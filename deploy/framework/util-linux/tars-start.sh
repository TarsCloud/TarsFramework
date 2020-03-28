TARS=(tarsAdminRegistry tarsregistry tarsnotify tarsconfig tarsnode tarslog tarspatch tarsproperty tarsqueryproperty tarsquerystat  tarsstat)

cd TARS_PATH

for var in ${TARS[@]};
do
  if [ -d ${var} ]; then
    echo "start ${var}"
    TARS_PATH/${var}/util/start.sh
    sleep 1
  fi
done

pm2 stop -s tars-node-web; cd /usr/local/app/web/; npm run prd;

pm2 stop -s tars-user-system; cd /usr/local/app/web/demo; npm run prd;
