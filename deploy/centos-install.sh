#!/bin/sh

#sh centos-install.sh 192.168.7.152 Rancher@12345 eth0 false false
#sh centos-install.sh 192.168.7.152 Rancher@12345 eth0 true false

if (( $# < 5 ))
then
    echo "$0 MYSQL_IP MYSQL_PASSWORD INET REBUILD(true/false) SLAVE(false[default]/true)";
    exit -1
fi

MYSQLIP=$1
PORT=3306
USER=root
PASS=$2
INET=$3
REBUILD=$4
SLAVE=$5

if [ "$INET" == "" ]; then
    INET=(eth0)
fi

if [ "$REBUILD" != "true" ]; then
  REBUILD="false"
fi

if [ "$SLAVE" != "true" ]; then
    SLAVE="false"
fi

# MYSQLIP=192.168.7.152
# USER=root
# PASS=Rancher@12345
# PORT=3306
# INET=(enp3s0 eth0)

#########################################################################

NODE_VERSION="v12.13.0"

if [ "${SLAVE}" != "true" ]; then
    TARS=(tarsAdminRegistry tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)
else
    TARS=(tarsconfig  tarslog  tarsnode  tarsnotify tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)
fi

TARS_PATH=/usr/local/app/tars
MIRROR=http://mirrors.cloud.tencent.com

workdir=$(cd $(dirname $0); pwd)

cp centos7_base.repo /etc/yum.repos.d/
cp epel-7.repo /etc/yum.repos.d/
cp MariaDB.repo /etc/yum.repos.d/
yum makecache fast

yum install -y yum-utils wget epel-release psmisc MariaDB-client telnet net-tools

WHO=`whoami`
if [ "$WHO" != "root" ]; then
    echo "only root user can call $0"
    exit -1
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
    echo "HOSTIP is [$HOSTIP], not valid. HOSTIP must not be 127.0.0.1 or empty."
    exit -1
fi

if [ "${SLAVE}" != "true" ]; then

  if [ ! -d ${workdir}/web ]; then
      LOG_ERROR "no web exits, please copy TarsWeb to ${workdir}/web first."
      exit -1
  fi

  ################################################################################
  #download nodejs

  source ~/.bashrc

  CURRENT_NODE_VERSION=`node --version`

  if [ "${CURRENT_NODE_VERSION}" != "${NODE_VERSION}" ]; then

    sh nvm-install.sh

    export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release/

    export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion"; 

    source ~/.bashrc

    nvm install ${NODE_VERSION};
  fi

  ################################################################################
  #check node version
  CURRENT_NODE_VERSION=`node --version`

  if [ "${CURRENT_NODE_VERSION}" != "${NODE_VERSION}" ]; then
      echo "node is not valid, must be:${NODE_VERSION}"
      exit -1
  fi

  echo "install node success! Version is ${NODE_VERSION}"

  source ~/.bashrc
  npm config set registry ${MIRROR}/npm/;
  npm install -g npm pm2

fi

cd ${workdir}/web; npm prune;npm audit fix

################################################################################

cd ${workdir}

sh tars-install.sh ${MYSQLIP} ${PORT} ${USER} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE}

if [ "$1" == "check" ]; then
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
fi


