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

#######################################################
TARS_PATH=/usr/local/app/tars

source ~/.bashrc

WORKDIR=$(cd $(dirname $0); pwd)

mkdir -p ${TARS_PATH}

if [ "$SLAVE" != "true" ]; then
  if [ ! -d ${WORKDIR}/web ]; then
      echo "no web exits, please copy TarsWeb to ${WORKDIR}/web first."
      exit -1
  fi
fi

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
    exit -1
fi

cd ${WORKDIR}

# pwd

./tars-install.sh ${MYSQLIP} ${PORT} ${USER} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE}

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
    pid=`ps -ef | grep /usr/local/app/web/bin/www | grep -v grep | awk -F' ' '{print $2}'`
    if echo $pid | grep -q '[^0-9]'
    then
      echo "start tars-web"
      cd /usr/local/app/web/; npm run prd
    fi
  fi

  sleep 3

done


