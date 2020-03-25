
TARS=(tarsAdminRegistry tarsconfig tarslog tarsnode tarsnotify tarspatch tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat)

cd TARS_PATH

for var in ${TARS[@]};
do
  echo "stop ${var}"
  TARS_PATH/${var}/util/stop.sh
done

pm2 stop tars-node-web;pm2 stop tars-user-system
