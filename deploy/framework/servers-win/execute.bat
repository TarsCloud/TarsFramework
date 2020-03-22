
@echo off
setlocal enabledelayedexpansion

@echo off
set SERVER_NAME=%1

@echo off
set COMMAND=%2

@echo off
if "!COMMAND!"=="stop" (
    @echo off
    taskkill /im !SERVER_NAME!.exe /t /f
)

@echo off
if "!COMMAND!" == "start" (
    @echo off
    taskkill /im !SERVER_NAME!.exe /t /f

    set CONFIG=c:\tars-install\tarsnode\data\tars.!SERVER_NAME!\conf\tars.!SERVER_NAME!.config.conf

    if not exist !CONFIG! (

      start /b c:\tars-install\!SERVER_NAME!\bin\!SERVER_NAME!.exe --config=c:\tars-install\!SERVER_NAME!\conf\tars.!SERVER_NAME!.config.conf > nul

    ) else (

      echo "start !SERVER_NAME!.exe --config=!CONFIG!"
      start /b c:\tars-install\!SERVER_NAME!\bin\!SERVER_NAME!.exe --config=!CONFIG! > nul
    )

)

ENDLOCAL
