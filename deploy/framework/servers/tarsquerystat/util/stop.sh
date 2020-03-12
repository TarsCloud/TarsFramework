#!/bin/sh

bin="/usr/local/app/tars/tarsquerystat/bin/tarsquerystat"

if [[ "$OS" =~ "Darwin" ]]; then
    OS=1
else
    OS=0
fi

if [[ $OS == 1 ]]; then
PID=`ps -eopid,comm | grep "$bin"| grep "tarsquerystat" |  grep -v "grep" |awk '{print $1}'`
else
PID=`ps -eopid,cmd | grep "$bin"| grep "tarsquerystat" |  grep -v "grep" |awk '{print $1}'`
fi

#PID=`ps -eopid,cmd | grep "$bin"| grep "tarsquerystat" |  grep -v "grep"|grep -v "sh" |awk '{print $1}'`

if [ "$PID" != "" ]; then
    kill -9 $PID
    echo "kill -9 $PID"
fi

