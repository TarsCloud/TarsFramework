#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/app/tarsauth/bin/:/usr/local/app/tars/tarsnode/data/lib/

/usr/local/app/tars/tarsauth/bin/tarsauth --config=/usr/local/app/tars/tarsauth/conf/tars.tarsauth.config.conf  &
