#!/bin/sh

bin="/usr/local/app/tars/tarsconfig/bin/tarsconfig"

OS=`uname`

if [[ "$OS" =~ "Darwin" ]]; then
    OS=1
else
    OS=0
fi

if [[ $OS == 1 ]]; then
PID=`ps -eopid,comm | grep "$bin"| grep "tarsconfig" |  grep -v "grep" |awk '{print $1}'`
else
PID=`ps -eopid,cmd | grep "$bin"| grep "tarsconfig" |  grep -v "grep" |awk '{print $1}'`
fi

#PID=`ps -eopid,cmd | grep "$bin"| grep "tarsconfig" |  grep -v "grep"|grep -v "sh" |awk '{print $1}'`

echo $PID

if [ "$PID" != "" ]; then
        kill -9 $PID
            echo "kill -9 $PID"
        fi

