#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/app/tars/tarsstat/bin/:/usr/local/app/tars/tarsnode/data/lib/

sh /usr/local/app/tars/tarsqueryproperty/util/execute.sh

#/usr/local/app/tars/tarsstat/bin/tarsstat --config=/usr/local/app/tars/tarsstat/conf/tars.tarsstat.config.conf  &
