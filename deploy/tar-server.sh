#!/bin/bash

TARS=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat) 

cd ${TARS_INSTALL}/framework/servers; 

for var in ${TARS[@]} 
  do tar czf ${var}.tgz ${var} 
done