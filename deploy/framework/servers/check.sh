#!/bin/bash

echo "begin check server..."

TARS_PATH=/usr/local/app/tars

sh ${TARS_PATH}/tarsnode/util/check.sh

#if [ "$1" == "master" ]; then
#  TARS=(tarsAdminRegistry  tarsnode  tarsregistry)
#else
#  TARS=(tarsnode tarsregistry)
#fi
#
#for var in ${TARS[@]};
#do
#  sh ${TARS_PATH}/${var}/util/check.sh
#done
#
#if [ "$1" == "master" ]; then
#  pm2 ping tars-node-web; pm2 ping tars-user-system
#fi


