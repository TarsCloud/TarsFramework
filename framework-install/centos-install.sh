#!/bin/sh

NODE_VERSION="v12.13.0"
MYSQLIP=192.168.7.152
USER=root
PASS=Rancher@12345
PORT=3306
INET=(enp3s0 eth0)
HOSTIP=""

#######################################################
INSTALL_TMP=/tmp/tars-install
TARS_PATH=/usr/local/app/tars

workdir=$(cd $(dirname $0); pwd)

#if [ 1 == 2 ]; then

cp centos7_base.repo /etc/yum.repos.d/
cp MariaDB.repo /etc/yum.repos.d/
yum makecache fast

yum install -y yum-utils wget epel-release mariadb-libs psmisc MariaDB-client telnet net-tools make curl 

wget -qO- https://raw.github.com/creationix/nvm/master/install.sh | sh

mkdir -p ${INSTALL_TMP}
cp -rf web ${INSTALL_TMP}/web

#fi

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

if [ ! -d ${workdir}/web ]; then
    LOG_ERROR "no web exits, please copy TarsWeb to ${workdir}/web first."
    exit -1
fi

################################################################################
#download nodejs

export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion"; export NVM_NODEJS_ORG_MIRROR=https://mirrors.cloud.tencent.com/nodejs-release/;nvm install ${NODE_VERSION};npm config set registry http://mirrors.cloud.tencent.com/npm/;npm install -g npm;cd ${INSTALL_TMP}/web;npm install

################################################################################
#check node version
NODE_VERSION=`node --version`

if [ NODE_VERSION != "${NODE_VERSION}" ]; then
    echo "node is not valid, must be:${NODE_VERSION}"
    exit -1
fi

echo "install node success! Version is ${NODE_VERSION}"

################################################################################
cp -rf framework ${INSTALL_TMP}/framework
cp -rf tools ${INSTALL_TMP}/tools
cp -rf tars.sh tars-install.sh ${INSTALL_TMP}

export PATH=${HOME}/.nvm/versions/node/${NODE_VERSION}/bin/:$PATH
# export npm=mirrors.cloud.tencent.com

sh ${workdir}/tars.sh stop

cd ${workdir}

pwd

sh tars-install.sh ${MYSQLIP} ${PORT} ${USER} ${PASS} ${HOSTIP}

TARS=(tarsAdminRegistry tarsconfig  tarslog  tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)

for var in ${TARS[@]};
do
   if [ ! -d ${TARS_PATH}/${var} ]; then
       echo "${TARS_PATH}/${var} not exist."
       exit -1 
   fi
done

sh ${workdir}/tars.sh start

if [ "$1" == "check" ]; then
  echo "begin check server..."
  TARS=(tarsAdminRegistry  tarsnode  tarsregistry)

  while [ 1 ]
  do
  #  echo "check server..."

    for var in ${TARS[@]};
    do
      sh ${TARS_PATH}/${var}/util/check.sh
    done

    pid=`ps -ef | grep tars-node-web | grep -v grep | awk -F' ' '{print $2}'`
    if echo $pid | grep -q '[^0-9]'
    then
      echo "start tars-web"
      cd /usr/local/app/web/; nohup npm run start &
    fi

    sleep 3

  done
fi


