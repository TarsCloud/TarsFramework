#!/bin/sh
export PATH=${PATH}:/usr/local/app/tars/bin:/usr/local/jdk/bin;
bin="/usr/local/app/tars/tarsnode/bin/tarsnode"

OS=`uname`

if [[ "$OS" =~ "Darwin" ]]; then
    OS=1
else
    OS=0
fi

if [[ $OS == 1 ]]; then
PID=`ps -eopid,comm | grep "$bin"| grep "tarsnode" |  grep -v "grep" |grep -v "sh"|awk '{print $1}'`
else
PID=`ps -eopid,cmd | grep "$bin"| grep "tarsnode" |  grep -v "grep" |grep -v "sh" |awk '{print $1}'`
fi

if [ "$PID" == "" ]; then
  sh /usr/local/app/tars/tarsnode/util/start.sh
fi
