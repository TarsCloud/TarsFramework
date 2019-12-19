#!/bin/bash

TARS=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat) 

cd ${TARS_INSTALL}/framework/servers; 

for var in ${TARS[@]} 
  do tar czf ${var}.tgz ${var} 
done

mkdir -p ${TARS_INSTALL}/web/files
cp -rf ${TARS_INSTALL}/framework/servers/*.tgz ${TARS_INSTALL}/web/files/
rm -rf ${TARS_INSTALL}/framework/servers/*.tgz