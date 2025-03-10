#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 repo_name"
    exit 1
fi

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DIR="../$1"


generate() {
    echo "Now copy '$TARGET_DIR'..."
    cp -r "$SOURCE_DIR" "$TARGET_DIR"
    echo "Copied $SOURCE_DIR to $TARGET_DIR"
    cd $TARGET_DIR
    rm -rf .git
    rm -rf bazel-*
    sed -i "s/oceandoc/$1/g" `grep 'oceandoc' -rl .`
}

if [ -d "$TARGET_DIR" ]; then
    echo "Directory '$TARGET_DIR' exists."
    read -p "Do you want to delete it? (yes/no): " choice
    if [ "$choice" = "yes" ]; then
        rm -r "$TARGET_DIR"
        echo "Directory deleted."
        generate $1
    else
        echo "No operation"
    fi
else
    generate $1
fi

