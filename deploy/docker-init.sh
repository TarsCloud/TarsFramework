#!/bin/sh

#docker run -d -p3001:3000 -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxxx -eREBUILD=false -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh
#docker run -d --net=host -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxxx -eREBUILD=true -eINET=enp3s0 -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh
#docker run --net=host -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxxx -eREBUILD=true -eINET=enp3s0 -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh
#docker run --net=host -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxxxxx -eREBUILD=false -eINET=enp3s0 -v/data/log/app_log:/usr/local/app/tars/app_log -v/data/log/web_log:/usr/local/app/web/log -v/data/patchs:/usr/local/app/patchs tars-docker:v1 sh /root/tars-install/docker-init.sh

env

NODE_VERSION="v12.13.0"
MYSQLIP=`echo ${MYSQL_HOST}`
USER=`echo ${MYSQL_USER}`
PASS=`echo ${MYSQL_ROOT_PASSWORD}`
PORT=`echo ${MYSQL_PORT}`
REBUILD=`echo ${REBUILD}`
INET=`echo ${INET}`
#hostname存在, 则优先使用hostname
DOMAIN=`echo ${DOMAIN}`

if [ "$USER" == "" ]; then
    USER="root"
fi

if [ "$PORT" == "" ]; then
    PORT="3306"
fi

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
mkdir -p /data/tars/tarsnode-data

trap 'exit' SIGTERM SIGINT

./tars-install.sh ${MYSQLIP} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE} ${USER} ${PORT} ${TARS_PATH}
if [ $? != 0 ]; then
    echo  "tars-install.sh error"
    exit 1
fi

echo "install tars success. begin check server..."
if [ "$SLAVE" != "true" ]; then
  TARS=(tarsAdminRegistry  tarsnode  tarsregistry)
else
  TARS=(tarsnode tarsregistry)
fi

while [ 1 ]
do
    sh ${TARS_PATH}/tarsnode/util/check.sh
    sleep 3
done


