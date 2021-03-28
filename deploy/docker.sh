#!/bin/bash

if [ $# -lt 2  ]; then
    echo $0 version dockerfile
    echo "example: "$0 v2.4.15 deploy/x64.build.Dockerfile
    echo "example: "$0 v2.4.15 deploy/arm64.build.Dockerfile
    exit 1      
fi              

SRC=`pwd`

if [ ! -d "deploy" ]; then
    echo "you must execute $0 in framework directory."
    exit 1
fi

docker build . -t tarscloud/framework:$1 -f $2



