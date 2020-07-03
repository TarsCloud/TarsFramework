#!/bin/bash


OS=`uname`

if [ "$OS" = "Darwin" ]; then
    OS=1
else
    OS=0
fi

SERVER_NAME=tarsregistry

bin="TARS_PATH/${SERVER_NAME}/bin/${SERVER_NAME}"


if [ $OS -eq 1 ]; then
    PID=`ps -eopid,comm | grep "$bin"| grep "${SERVER_NAME}" |  grep -v "grep" |awk '{print $1}'`
else
    PID=`ps -eopid,cmd | grep "$bin"| grep "${SERVER_NAME}" |  grep -v "grep" |awk '{print $1}'`
fi

if [ "$PID" == "" ]; then
    sh TARS_PATH/tarsregistry/util/start.sh
fi

