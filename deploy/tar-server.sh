#!/bin/bash

mkdir -p /data/tars/app_log
mkdir -p /data/tars/web_log
mkdir -p /data/tars/demo_log
mkdir -p /data/tars/patchs
mkdir -p /data/tars/tarsnode-data

mkdir -p /usr/local/app/tars/
mkdir -p /usr/local/app/web/
mkdir -p /usr/local/app/web/demo/
mkdir -p /usr/local/app/tars/tarsnode

ln -s /data/tars/app_log /usr/local/app/tars/app_log 
ln -s /data/tars/web_log /usr/local/app/web/log 
ln -s /data/tars/demo_log /usr/local/app/web/demo/log 
ln -s /data/tars/patchs /usr/local/app/patchs 
ln -s /data/tars/tarsnode-data /usr/local/app/tars/tarsnode/data

TARS=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat) 

strip ${TARS_INSTALL}/framework/servers/tars*/bin/tars*
chmod a+x ${TARS_INSTALL}/framework/servers/tars*/util/*.sh

cd ${TARS_INSTALL}/framework/servers; 

for var in ${TARS[@]} 
  do tar czf ${var}.tgz ${var} 
done

cp -rf ${TARS_INSTALL}/web/sql/*.sql ${TARS_INSTALL}/framework/sql/
cp -rf ${TARS_INSTALL}/web/demo/sql/*.sql ${TARS_INSTALL}/framework/sql/
 
mkdir -p ${TARS_INSTALL}/web/files
cp -rf ${TARS_INSTALL}/framework/servers/*.tgz ${TARS_INSTALL}/web/files/
rm -rf ${TARS_INSTALL}/framework/servers/*.tgz

cp ${TARS_INSTALL}/tools/install.sh ${TARS_INSTALL}/web/files/
