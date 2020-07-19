#!/bin/bash

TARS="tarsAdminRegistry tarsconfig tarslog tarsnode tarsnotify tarspatch tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat"

TARS_PATH=/usr/local/app/tars/
for var in ${TARS};
do
  echo "stop ${var}"
  ${TARS_PATH}/${var}/util/stop.sh
done

pm2 stop tars-node-web;

if [ -f /usr/local/app/web/demo/package.json ]; then
  pm2 stop tars-user-system
fi

