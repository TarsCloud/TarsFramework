#!/bin/bash

TARS="tarsAdminRegistry tarsregistry tarsnotify tarsconfig tarsnode tarslog tarspatch tarsproperty tarsqueryproperty tarsquerystat  tarsstat"

cd TARS_PATH

for var in ${TARS};
do
  if [ -d ${var} ]; then
    echo "start ${var}"
    TARS_PATH/${var}/util/start.sh
    sleep 1
  fi
done

if [ -f WEB_PATH/web/package.json ]; then
  pm2 stop -s tars-node-web; cd WEB_PATH/web/; npm run prd;
fi
