@echo off
call TARS_PATH/tarsnode/bin/tarsnode.exe --monitor --config=TARS_PATH/tarsnode/conf/tars.tarsnode.config.conf

@echo off
if %errorlevel% NEQ 0 (
    call TARS_PATH/tarsnode/util/start.bat
)

