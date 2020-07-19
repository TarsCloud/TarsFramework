
#!/bin/bash

TARS="tarsAdminRegistry tarsconfig tarslog tarsnode tarsnotify tarspatch tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat"

cd TARS_PATH

for var in ${TARS};
do
  echo "stop ${var}"
  TARS_PATH/${var}/util/stop.sh
done

if [ -d WEB_PATH/web ]; then
  pm2 stop -s tars-node-web
fi

if [ -f WEB_PATH/web/demo/package.json ]; then
  pm2 stop -s tars-user-system
fi


