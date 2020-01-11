#!/bin/sh

#docker run -d -p3001:3000 -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxxx -eREBUILD=false -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh
#docker run -d --net=host -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxxx -eREBUILD=true -eINET=enp3s0 -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh
#docker run --net=host -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxxx -eREBUILD=true -eINET=enp3s0 -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh
#docker run --net=host -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxx -eREBUILD=false -eINET=enp3s0 -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh

env

NODE_VERSION="v12.13.0"
MYSQLIP=`echo ${MYSQL_HOST}`
USER=root
PASS=`echo ${MYSQL_ROOT_PASSWORD}`
PORT=3306
REBUILD=`echo ${REBUILD}`
INET=`echo ${INET}`
#hostname存在, 则优先使用hostname
DOMAIN=`echo ${DOMAIN}`

if [ "$INET" == "" ]; then
   INET=(eth0)
fi

if [ "$REBUILD" != "true" ]; then
  REBUILD="false"
fi

if [ "$SLAVE" != "true" ]; then
    SLAVE="false"
fi

HOSTIP=""

if [ "$DOMAIN" != "" ]; then
  HOSTIP=$DOMAIN
else 

  #获取主机hostip
  for IP in ${INET[@]};
  do
      HOSTIP=`ifconfig | grep ${IP} -A3 | grep inet | grep broad | awk '{print $2}'    `
      echo $HOSTIP $IP
      if [ "$HOSTIP" != "127.0.0.1" ] && [ "$HOSTIP" != "" ]; then
        break
      fi
  done

  if [ "$HOSTIP" == "127.0.0.1" ] || [ "$HOSTIP" == "" ]; then
      echo "HOSTIP:[$HOSTIP], not valid. HOSTIP must not be 127.0.0.1 or empty."
      exit 1
  fi
fi

#######################################################

WORKDIR=$(cd $(dirname $0); pwd)

#######################################################
TARS_PATH=/usr/local/app/tars

mkdir -p ${TARS_PATH}

source ~/.bashrc

if [ "$SLAVE" != "true" ]; then
  if [ ! -d ${WORKDIR}/web ]; then
      echo "no web exits, please copy TarsWeb to ${WORKDIR}/web first."
      exit 1
  fi
fi

cd ${WORKDIR}

mkdir -p /data/tars/app_log
mkdir -p /data/tars/web_log
mkdir -p /data/tars/demo_log
mkdir -p /data/tars/patchs

./tars-install.sh ${MYSQLIP} ${PORT} ${USER} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE}
if [ $? != 0 ]; then
    echo  "tars-install.sh error"
    exit 1
fi

echo "begin check server..."
if [ "$SLAVE" != "true" ]; then
  TARS=(tarsAdminRegistry  tarsnode  tarsregistry)
else
  TARS=(tarsnode tarsregistry)
fi

while [ 1 ]
do

  for var in ${TARS[@]};
  do
    sh ${TARS_PATH}/${var}/util/check.sh
  done

  if [ "$SLAVE" != "true" ]; then
    pm2 ping tars-node-web; pm2 ping tars-user-system  
  fi

  sleep 3

done


