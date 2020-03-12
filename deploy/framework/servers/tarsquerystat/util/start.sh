#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/app/tars/tarsquerystat/bin/:/usr/local/app/tars/tarsnode/data/lib/

sh /usr/local/app/tars/tarsqueryproperty/util/execute.sh

#/usr/local/app/tars/tarsquerystat/bin/tarsquerystat --config=/usr/local/app/tars/tarsquerystat/conf/tars.tarsquerystat.config.conf  &
