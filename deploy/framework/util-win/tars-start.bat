@echo off
SETLOCAL

@echo off
set TARS="tarsAdminRegistry,tarsconfig,tarslog,tarsnode,tarsnotify,tarspatch,tarsproperty,tarsqueryproperty,tarsquerystat,tarsregistry,tarsstat"

:loop

@echo off
for /f "Tokens=1,* Delims=," %%a in (%TARS%) do (
    echo "start %%a"

    call TARS_PATH\%%a\util\start.bat

    set TARS="%%b"

    goto :loop
)

if exist WEB_PATH\web (
    call pm2 stop -s tars-node-web
    cd WEB_PATH\web
    call npm run prd
)

if exist WEB_PATH\web\demo\package.json (
    call pm2 stop -s tars-user-system
    cd WEB_PATH\web\demo
    call npm run prd
)

@echo off
ENDLOCAL

