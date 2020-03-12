#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/app/tarslog/bin/:/usr/local/app/tars/tarsnode/data/lib/

sh /usr/local/app/tars/tarslog/util/execute.sh

#/usr/local/app/tars/tarslog/bin/tarslog --config=/usr/local/app/tars/tarslog/conf/tars.tarslog.config.conf  &
