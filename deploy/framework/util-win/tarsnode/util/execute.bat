

@echo off
setlocal enabledelayedexpansion

@echo off
set PATH=%PATH%;TARS_PATH\tarsnode\data\lib\;

@echo off
set SERVER_NAME=%1

@echo off
set COMMAND=%2

@echo off
if "!COMMAND!"=="stop" (
    @echo off
    taskkill /f /fi "imagename eq !SERVER_NAME!.exe" /im !SERVER_NAME!.exe /t
)

@echo off
if "!COMMAND!" == "start" (
    @echo off
    taskkill /f /fi "imagename eq !SERVER_NAME!.exe" /im !SERVER_NAME!.exe /t
    REM taskkill /im !SERVER_NAME!.exe /t /f

    set CONFIG=TARS_PATH\tarsnode\data\tars.!SERVER_NAME!\conf\tars.!SERVER_NAME!.config.conf

    if not exist !CONFIG! (

      start /b TARS_PATH\!SERVER_NAME!\bin\!SERVER_NAME!.exe --config=TARS_PATH\!SERVER_NAME!\conf\tars.!SERVER_NAME!.config.conf > nul

    ) else (

      echo "start !SERVER_NAME!.exe --config=!CONFIG!"
      start /b TARS_PATH\!SERVER_NAME!\bin\!SERVER_NAME!.exe --config=!CONFIG! > nul
    )

)

ENDLOCAL




