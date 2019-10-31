#!/bin/sh

bin="/usr/local/app/tars/tarsnotify/bin/tarsnotify"

PID=`ps -eopid,cmd | grep "$bin"| grep "tarsnotify" |  grep -v "grep"|grep -v "sh" |awk '{print $1}'`

echo $PID

if [ "$PID" != "" ]; then
        kill -9 $PID
            echo "kill -9 $PID"
        fi

