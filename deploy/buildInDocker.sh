#!/bin/bash

if [ $# -lt 2  ]; then
    echo $0 version dockerfile
    echo "example: "$0 v2.4.15 deploy/build.Dockerfile
    echo "example: "$0 v2.4.15 deploy/arm64.build.Dockerfile
    exit        
fi              

SRC=`pwd`

# cd deploy

docker build . -t tars.build -f $2

# docker run -v $SRC:/data tars.build bash -c "cd /data ; mkdir -p build-tmp; cd build-tmp ; pwd;  cmake .. ; make -j4; make install"
# docker run tars.build bash -c "cd /usr/local/tars/cpp/deploy ; sh docker.sh $1"


