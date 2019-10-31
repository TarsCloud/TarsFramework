#!/bin/sh

bin="/usr/local/app/tars/tarsquerystat/bin/tarsquerystat"

PID=`ps -eopid,cmd | grep "$bin"| grep "tarsquerystat" |  grep -v "grep"|grep -v "sh" |awk '{print $1}'`

echo $PID

if [ "$PID" != "" ]; then
        kill -9 $PID
            echo "kill -9 $PID"
        fi

