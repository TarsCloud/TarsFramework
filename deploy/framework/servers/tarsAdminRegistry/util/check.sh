#!/bin/sh
#ulimit -a
bin="/usr/local/app/tars/tarsAdminRegistry/bin/tarsAdminRegistry"

PID=`ps -eopid,cmd | grep "$bin"| grep "tarsAdminRegistry" |  grep -v "grep" |awk '{print $1}'`

#echo $PID

#ulimit -c unlimited

if [ "$PID" == "" ]; then
  CONFIG=/usr/local/app/tars/tarsnode/data/tars.tarsAdminRegistry/conf/tars.tarsAdminRegistry.config.conf

  if [ ! -f $CONFIG ]; then
    CONFIG=/usr/local/app/tars/tarsAdminRegistry/conf/tars.tarsAdminRegistry.config.conf	
  fi

  $bin  --config=$CONFIG &

fi

