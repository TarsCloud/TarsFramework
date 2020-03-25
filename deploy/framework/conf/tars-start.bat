
SETLOCAL

set TARS="tarsAdminRegistry,tarsconfig,tarslog,tarsnode,tarsnotify,tarspatch,tarsproperty,tarsqueryproperty,tarsquerystat,tarsregistry,tarsstat"

:loop

@echo off
for /f "Tokens=1,* Delims=," %%a in (%TARS%) do (
    echo "start %%a"

    call c:/tars-install\%%a\util\start.bat

    set TARS="%%b"

    goto :loop
)

call pm2 stop tars-node-web
cd TARS_PATH\web; 
call pm2 delete tars-node-web; 
call npm run prd;

call pm2 stop tars-user-system; 
cd TARS_PATH\web\demo; 
call pm2 delete tars-user-system; 
call npm run prd;

ENDLOCAL

