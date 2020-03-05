#!/bin/sh
# ulimit -a
bin="/usr/local/app/tars/tarsAdminRegistry/bin/tarsAdminRegistry"

chmod a+x $bin

OS=`uname`

if [[ "$OS" =~ "Darwin" ]]; then
    OS=1
else
    OS=0
fi

if [[ $OS == 1 ]]; then
PID=`ps -eopid,comm | grep "$bin"| grep "tarsAdminRegistry" |  grep -v "grep" |awk '{print $1}'`
else
PID=`ps -eopid,cmd | grep "$bin"| grep "tarsAdminRegistry" |  grep -v "grep" |awk '{print $1}'`
fi

echo $PID

if [ "$PID" != "" ]; then
        kill -9 $PID
        echo "kill -9 $PID"
fi
ulimit -c unlimited

CONFIG=/usr/local/app/tars/tarsnode/data/tars.tarsAdminRegistry/conf/tars.tarsAdminRegistry.config.conf

if [ ! -f $CONFIG ]; then
	CONFIG=/usr/local/app/tars/tarsAdminRegistry/conf/tars.tarsAdminRegistry.config.conf	
fi

$bin  --config=$CONFIG &

