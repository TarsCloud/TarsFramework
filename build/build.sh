# Copyright (c) 2020
# Linux Foundation Projects
#       Link: https://www.linuxfoundation.org/projects/
# TARS Foundation Projects
#       Link: https://github.com/TarsCloud
# All rights reserved.

#!/bin/sh

ARGS=$1

if [ $# -lt 1 ]; then
    ARGS="help"
fi

BASEPATH=$(cd `dirname $0`; pwd)

case $ARGS in
    prepare)
        cd ..; git submodule update --init --recursive
        ;;
    all)
        cd $BASEPATH;  cmake ..;  make
        ;;
    cleanall)
        cd $BASEPATH; make clean; ls | grep -v build.sh | grep -v README.md | xargs rm -rf
        ;;
    install)
        cd $BASEPATH; make install
        ;;
    help|*)
        echo "Usage:"
        echo "$0 help:     view help info."
        echo "$0 prepare:  download dependent project."
        echo "$0 all:      build all target"
        echo "$0 install:  install framework"
        echo "$0 cleanall: remove all temp files"
        ;;
esac
