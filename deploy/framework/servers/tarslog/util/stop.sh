#!/bin/sh

bin="/usr/local/app/tars/tarslog/bin/tarslog"

PID=`ps -eopid,cmd | grep "$bin"| grep "tarslog" |  grep -v "grep"|grep -v "sh" |awk '{print $1}'`

echo $PID

if [ "$PID" != "" ]; then
        kill -9 $PID
            echo "kill -9 $PID"
        fi

