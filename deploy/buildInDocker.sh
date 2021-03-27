#!/bin/bash

if [ $# -lt 2 ]; then
    echo $0 docker version
    exit
fi

INDOCKER=$1
SRC=`pwd`

docker run -it -v $SRC:/data $INDOCKER
docker exec $DOCKER bash -c "cd /data; mkdir -p build && cd build; "

