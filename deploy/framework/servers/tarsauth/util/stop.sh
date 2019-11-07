#!/bin/sh

bin="/usr/local/app/tars/tarsauth/bin/tarsauth"

PID=`ps -eopid,cmd | grep "$bin"| grep "tarsauth" |  grep -v "grep"|grep -v "sh" |awk '{print $1}'`

echo $PID

if [ "$PID" != "" ]; then
        kill -9 $PID
            echo "kill -9 $PID"
        fi

