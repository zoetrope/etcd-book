#!/bin/bash -e

# get veth interface for a given container
# https://stackoverflow.com/a/28613516

# Get the process ID for the container named ${1}:
pid=$(docker inspect -f '{{.State.Pid}}' "${1}")

# Make the container's network namespace available to the ip-netns command:
mkdir -p /var/run/netns
ln -sf /proc/$pid/ns/net "/var/run/netns/${1}"

# Get the interface index of the container's eth0:
index=$(ip netns exec "${1}" ip link show eth0 | head -n1 | sed s/:.*//)
# Increment the index to determine the veth index, which we assume is
# always one greater than the container's index:
let index=index+1

# Write the name of the veth interface to stdout:
ip link show | grep "^${index}:" | sed "s/${index}: \(.*\):.*/\1/" | cut -d '@' -f1

# Clean up the netns symlink, since we don't need it anymore
rm -f "/var/run/netns/${1}"
