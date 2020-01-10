TARS=(tarsAdminRegistry tarsconfig tarslog tarsnode tarsnotify tarspatch tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat)

TARS_PATH=/usr/local/app/tars/

cd ${TARS_PATH}

for var in ${TARS[@]};
do
  if [ -d ${var} ]; then
    echo "start ${var}"
    ${TARS_PATH}/${var}/util/start.sh
  fi
done

pm2 start tars-node-web;pm2 start tars-user-system