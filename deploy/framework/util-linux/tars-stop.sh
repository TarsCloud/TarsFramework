
TARS=(tarsAdminRegistry tarsconfig tarslog tarsnode tarsnotify tarspatch tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat)

cd TARS_PATH

for var in ${TARS[@]};
do
  echo "stop ${var}"
  TARS_PATH/${var}/util/stop.sh
done

if [ -d TARS_PATH/web ]; then
  pm2 stop -s tars-node-web;pm2 stop tars-user-system
fi
