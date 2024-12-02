#!/bin/bash

deploy_local() {
    echo "Building and stripping binary for local deployment..."
    bazel build  //...
    strip bazel-bin/src/server/server
    cp bazel-bin/src/server/server /usr/local/fstation/bin
    cp -r conf /usr/local/fstation
    systemctl restart fstation
    echo "Local deployment completed"
}

deploy_remote() {
    echo "Building and stripping binary for remote deployment..."
 
    ssh ubuntu@54.169.178.197 "sudo systemctl stop fstation"
    ssh ubuntu@54.169.178.197 "sudo rm /usr/local/fstation/bin/server"
    scp bazel-bin/src/server/server dev@54.169.178.197:/usr/local/fstation/bin
    
    ssh ubuntu@54.169.178.197 "sudo chmod +x /usr/local/fstation/bin/server"
    ssh ubuntu@54.169.178.197 "sudo systemctl restart fstation"
    echo "Remote deployment completed"
}

# Parse command line argument
DEPLOY_TYPE=${1:-"local"}  # Default to "remote" if no argument provided

case $DEPLOY_TYPE in
    "local")
        deploy_local
        ;;
    "remote")
        deploy_remote
        ;;
    "all")
        deploy_local
        deploy_remote
        ;;
    *)
        echo "Invalid deployment type. Use: local, remote, or all"
        exit 1
        ;;
esac



