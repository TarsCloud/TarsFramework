#!/bin/sh

#docker run -d -p3001:3000 -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=Rancher@12345 -e MYSQL_PORT=3306 tars-docker:v1 sh /root/tars-install/docker-init.sh

TARS=(tarsAdminRegistry tarsconfig  tarslog  tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)

NODE_VERSION="v12.13.0"
MYSQLIP=`echo ${MYSQL_HOST}`
USER=root
PASS=`echo ${MYSQL_ROOT_PASSWORD}`
PORT=`echo ${MYSQL_PORT}`
if [ "$PORT" == "" ]; then
  PORT="3306"
fi

INET=(eth0)
HOSTIP=""

#######################################################
INSTALL_TMP=/tmp/tars-install
TARS_PATH=/usr/local/app/tars

source ~/.bashrc

workdir=$(cd $(dirname $0); pwd)

mkdir -p ${INSTALL_TMP}
mkdir -p ${TARS_PATH}

if [ ! -d ${workdir}/web ]; then
    echo "no web exits, please copy TarsWeb to ${workdir}/web first."
    exit -1
fi

cp -rf ${workdir}/web ${INSTALL_TMP}/web

#获取主机hostip
for IP in ${INET[@]};
do
    HOSTIP=`ifconfig | grep ${IP} -A3 | grep inet | grep broad | awk '{print $2}'    `
    echo $HOSTIP $IP
    if [ "$HOSTIP" != "127.0.0.1" ] && [ "$HOSTIP" != "" ]; then
      break
    fi
done

WHO=`whoami`
if [ "$WHO" != "root" ]; then
    echo "only root user can call $0"
    exit -1
fi

if [ "$HOSTIP" == "127.0.0.1" ] || [ "$HOSTIP" == "" ]; then
    echo "HOSTIP is $HOSTIP, not valid. HOSTIP must not be 127.0.0.1 or empty."
    exit -1
fi

sh ${workdir}/tars.sh stop

cd ${workdir}

pwd

sh tars-install.sh ${MYSQLIP} ${PORT} ${USER} ${PASS} ${HOSTIP}

echo "begin check server..."
TARS=(tarsAdminRegistry  tarsnode  tarsregistry)

while [ 1 ]
do
#  echo "check server..."

  for var in ${TARS[@]};
  do
    sh ${TARS_PATH}/${var}/util/check.sh
  done

  pid=`ps -ef | grep /usr/local/app/web/bin/www | grep -v grep | awk -F' ' '{print $2}'`
  if echo $pid | grep -q '[^0-9]'
  then
    echo "start tars-web"
    cd /usr/local/app/web/; nohup npm run start &
  fi

  sleep 3

done


