#!/bin/bash

if [ $# -lt 2 ]; then
    exit -1
fi

SERVER_NAME=$1
COMMAND=$2

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:TARS_PATH/${SERVER_NAME}/bin/:TARS_PATH/tarsnode/data/lib/

bin="TARS_PATH/${SERVER_NAME}/bin/${SERVER_NAME}"

chmod a+x $bin

OS=`uname`

if [ "$OS" = "Darwin" ]; then
    OS=1
else
    OS=0
fi

if [ $OS -eq 1 ]; then
    PID=`ps -eopid,comm | grep "$bin"| grep "${SERVER_NAME}" |  grep -v "grep" |awk '{print $1}'`
else
    PID=`ps -eopid,cmd | grep "$bin"| grep "${SERVER_NAME}" |  grep -v "grep" |awk '{print $1}'`
fi

if [ "${COMMAND}" = "stop" ]; then

    if [ "$PID" != "" ]; then
        kill -9 $PID
        echo "kill -9 $PID"
    fi
fi

if [ "${COMMAND}" = "start" ]; then

    if [ "$PID" != "" ]; then
        kill -9 $PID
        echo "kill -9 $PID"
    fi

    ulimit -c unlimited

    CONFIG=TARS_PATH/tarsnode/data/tars.${SERVER_NAME}/conf/tars.${SERVER_NAME}.config.conf

    if [ ! -f $CONFIG ]; then
        CONFIG=TARS_PATH/${SERVER_NAME}/conf/tars.${SERVER_NAME}.config.conf
    fi

    if [ "${SERVER_NAME}" = "tarsnode" ]; then
        echo "start $bin --nodeid=localip.tars.com --config=$CONFIG"
        $bin --nodeid="localip.tars.com" --config=$CONFIG > /dev/null &
    else
        echo "start $bin --config=$CONFIG"
        $bin --config=$CONFIG > /dev/null &
    fi

fi

