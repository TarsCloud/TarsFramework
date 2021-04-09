#!/bin/bash

if [ $# -lt 2  ]; then
    echo $0 version arm64/amd64
    echo "example: "$0 v2.4.15 arm64
    echo "example: "$0 v2.4.15 amd64
    exit 1      
fi              

SRC=`pwd`

if [ ! -d "deploy" ]; then
    echo "you must execute $0 in framework directory."
    exit 1
fi

if [ ! -d "web" ]; then
    echo "you must git clone https://github.com/TarsCloud/TarsWeb web in framework source directory"
    exit 1
fi


export DOCKER_CLI_EXPERIMENTAL=enabled 
docker buildx create --use --name tars-builder 
docker buildx inspect tars-builder --bootstrap
#docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64

docker run --rm --privileged tonistiigi/binfmt:qemu-v5.2.0

if [ "$2" == "amd64" ]; then
    docker buildx build . --file "deploy/Dockerfile" --tag tarscloud/framework:$1 --platform=linux/amd64 -o type=docker
elif [ "$2" == "arm64" ]; then
    docker buildx build . --file "deploy/Dockerfile" --tag tarscloud/framework:$1 --platform=linux/arm64  -o type=docker
else
    echo "example: "$0 v2.4.15 arm64
    echo "example: "$0 v2.4.15 amd64
fi




