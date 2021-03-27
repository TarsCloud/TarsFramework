#!/bin/bash

if [ $# -lt 2  ]; then
    echo $0 version dockerfile
    exit        
fi              

SRC=`pwd`

# cd deploy

docker build . -t tars.build -f $2

# docker run -v $SRC:/data tars.build bash -c "cd /data && rm -rf build-tmp; && cd bulid-tmp && cmake .. && make -j4; && make install"
# docker run tars.build bash -c "cd /usr/local/tars/cpp/deploy && sh docker.sh $1"


