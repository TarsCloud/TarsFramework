#!/bin/bash

cd TARS_PATH/tarsnode/util

bin="TARS_PATH/tarsnode/bin/tarsnode"

$bin --monitor --config=TARS_PATH/tarsnode/conf/tars.tarsnode.config.conf

ex=$?

if [ $ex -ne 0 ]; then
#    echo "monitor:"$ex", restart tarsnode"
    sh TARS_PATH/tarsnode/util/start.sh
fi

