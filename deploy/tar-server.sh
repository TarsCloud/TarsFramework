#!/bin/bash

TARS=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat) 

cd ${TARS_INSTALL}/framework/servers; 

for var in ${TARS[@]} 
  do tar czf ${var}.tgz ${var} 
done

cp -rf ${TARS_INSTALL}/web/sql/*.sql ${TARS_INSTALL}/framework/sql/
cp -rf ${TARS_INSTALL}/web/demo/sql/*.sql ${TARS_INSTALL}/framework/sql/
 
mkdir -p ${TARS_INSTALL}/web/files
cp -rf ${TARS_INSTALL}/framework/servers/*.tgz ${TARS_INSTALL}/web/files/
rm -rf ${TARS_INSTALL}/framework/servers/*.tgz