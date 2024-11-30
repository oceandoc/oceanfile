#!/bin/bash

bazel build --config=gcc_aarch64_linux_gnu //...
/zfs/www/files/gcc14.2.0-aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-strip bazel-bin/src/server/server
#scp -r conf dev@54.169.178.197:/usr/local/fstation
ssh ubuntu@54.169.178.197 "sudo systemctl stop fstation"
ssh ubuntu@54.169.178.197 "sudo rm /usr/local/fstation/bin/server"
scp bazel-bin/src/server/server dev@54.169.178.197:/usr/local/fstation/bin
ssh ubuntu@54.169.178.197 "sudo chmod +x /usr/local/fstation/bin/server"
ssh ubuntu@54.169.178.197 "sudo systemctl restart fstation"



