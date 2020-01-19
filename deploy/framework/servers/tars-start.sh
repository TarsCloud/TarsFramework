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

pm2 stop tars-node-web; cd /usr/local/app/web/; npm run prd;

pm2 stop tars-user-system; cd /usr/local/app/web/demo; npm run prd;
