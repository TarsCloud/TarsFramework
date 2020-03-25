cd TARS_PATH/tarsnode/util

bin="TARS_PATH/tarsnode/bin/tarsnode"

$bin --monitor --config=TARS_PATH/tarsnode/conf/tars.tarsnode.config.conf

ex=$?

if $ex == 0 (
    TARS_PATH/tarsnode/util/start.bat
)
