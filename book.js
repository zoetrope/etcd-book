const path = require("path")
const fs = require("fs")
module.exports = {
    "root": "./docs",
    "title": "etcd programming with Go",
    "plugins": [
        "include-codeblock"
    ],
    "pluginsConfig": {
        "include-codeblock": {
            "template": path.join(__dirname,"template.hbs")
        }
    }
};
