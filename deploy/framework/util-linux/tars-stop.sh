
#!/bin/bash

TARS="tarsAdminRegistry tarsconfig tarslog tarsnode tarsnotify tarspatch tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat"

cd TARS_PATH

for var in ${TARS};
do
  if [ -d ${var} ]; then
    echo "start ${var}"
    TARS_PATH/${var}/util/stop.sh
    sleep 1
  fi
done

if [ -f WEB_PATH/web/package.json ]; then
  pm2 stop -s tars-node-web
fi
