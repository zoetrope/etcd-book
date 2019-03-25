package main

import (
	"context"
	"fmt"
	"log"
	"path"
	"strings"

	vault "github.com/hashicorp/vault/api"
)

// CA Keys in Vault
const (
	CAEtcdServer = "ca-etcd-server"
	CAEtcdPeer   = "ca-etcd-peer"
	CAEtcdClient = "ca-etcd-client"
)

// addRole adds a role to CA if not exists.
func addRole(client *vault.Client, ca, role string, data map[string]interface{}) error {
	l := client.Logical()
	rpath := path.Join(ca, "roles", role)
	secret, err := l.Read(rpath)
	if err != nil {
		return err
	}
	if secret != nil {
		// already exists
		return nil
	}

	_, err = l.Write(rpath, data)
	return err
}

// IssueServerCert issues TLS server certificates.
func IssueServerCert(ctx context.Context, client *vault.Client, hostname, address string) (crt, key string, err error) {
	return issueCertificate(client, CAEtcdServer, "system",
		map[string]interface{}{
			"ttl":            "87600h",
			"max_ttl":        "87600h",
			"client_flag":    "false",
			"allow_any_name": "true",
		},
		map[string]interface{}{
			"common_name": hostname,
			"alt_names":   "localhost",
			"ip_sans":     "127.0.0.1," + address,
		})
}

// IssuePeerCert issues TLS certificates for mutual peer authentication.
func IssuePeerCert(ctx context.Context, client *vault.Client, hostname, address string) (crt, key string, err error) {
	return issueCertificate(client, CAEtcdPeer, "system",
		map[string]interface{}{
			"ttl":            "87600h",
			"max_ttl":        "87600h",
			"allow_any_name": "true",
		},
		map[string]interface{}{
			"common_name":          hostname,
			"ip_sans":              "127.0.0.1," + address,
			"exclude_cn_from_sans": "true",
		})
}

// IssueRoot issues certificate for root user.
func IssueRootCert(ctx context.Context, client *vault.Client) (cert, key string, err error) {
	return issueCertificate(client, CAEtcdClient, "admin",
		map[string]interface{}{
			"ttl":            "2h",
			"max_ttl":        "24h",
			"server_flag":    "false",
			"allow_any_name": "true",
		},
		map[string]interface{}{
			"common_name":          "root",
			"exclude_cn_from_sans": "true",
			"ttl":                  "1h",
		})
}

// IssueEtcdClientCertificate issues TLS client certificate for a target role.
func IssueEtcdClientCert(client *vault.Client, role, commonName, ttl string) (cert, key string, err error) {
	return issueCertificate(client, CAEtcdClient, role,
		map[string]interface{}{
			"ttl":            "87600h",
			"max_ttl":        "87600h",
			"server_flag":    "false",
			"allow_any_name": "true",
		},
		map[string]interface{}{
			"common_name":          commonName,
			"exclude_cn_from_sans": "true",
			"ttl":                  ttl,
		})
}

func issueCertificate(client *vault.Client, ca, role string, roleOpts, certOpts map[string]interface{}) (crt, key string, err error) {
	err = addRole(client, ca, role, roleOpts)
	if err != nil {
		return "", "", err
	}

	secret, err := client.Logical().Write(path.Join(ca, "issue", role), certOpts)
	if err != nil {
		return "", "", err
	}
	crt = secret.Data["certificate"].(string)
	key = secret.Data["private_key"].(string)
	return crt, key, err
}

func createCA(client *vault.Client, ca string) (string, error) {
	secret, err := client.Logical().Read(ca + "/cert/ca")
	if err != nil {
		return "", err
	}
	if secret == nil {
		p := ca + "/root/generate/internal"
		secret, err = client.Logical().Write(p, map[string]interface{}{
			"common_name": ca,
			"ttl":         "876000h",
			"format":      "pem",
		})
		if err != nil {
			return "", err
		}
	}
	cert := secret.Data["certificate"]
	return cert.(string), nil
}

func trailingSlash(p string) string {
	p = strings.TrimSpace(p)
	for len(p) > 0 && p[len(p)-1] != '/' {
		p = p + "/"
	}
	return p
}

func enableSecrets(client *vault.Client, ca string) error {
	mounts, err := client.Sys().ListMounts()
	if err != nil {
		return err
	}
	mountPath := trailingSlash(ca)
	if _, ok := mounts[mountPath]; ok {
		return nil
	}
	mountInput := &vault.MountInput{
		Type: "pki",
		Config: vault.MountConfigInput{
			DefaultLeaseTTL: "87600h",
			MaxLeaseTTL:     "876000h",
		},
	}
	return client.Sys().Mount(mountPath, mountInput)
}

func main() {
	client, err := vault.NewClient(&vault.Config{
		Address: "http://127.0.0.1:8200",
	})
	client.SetToken("myroot")
	if err != nil {
		log.Fatal(err)
	}
	for _, ca := range []string{CAEtcdServer, CAEtcdPeer, CAEtcdClient} {
		err = enableSecrets(client, ca)
		if err != nil {
			log.Fatal(err)
		}
		cert, err := createCA(client, ca)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Println(cert)
	}

	c, k, err := IssueServerCert(context.TODO(), client, "etcd", "192.168.0.1")
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("------------------------------------")
	fmt.Println(c)
	fmt.Println("------------------------------------")
	fmt.Println(k)
}
