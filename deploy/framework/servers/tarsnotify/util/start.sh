#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/app/tars/tarsnotify/bin/:/usr/local/app/tars/tarsnode/data/lib/

sh /usr/local/app/tars/tarsnotify/util/execute.sh

#/usr/local/app/tars/tarsnotify/bin/tarsnotify --config=/usr/local/app/tars/tarsnotify/conf/tars.tarsnotify.config.conf  &
