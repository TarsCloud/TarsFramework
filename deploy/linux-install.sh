#!/bin/bash

#./linux-install.sh 192.168.7.152 Rancher@12345 eth0 false false
#./inux-install.sh 192.168.7.152 Rancher@12345 eth0 true false

if (( $# < 5 ))
then
    echo "$0 MYSQL_IP MYSQL_PASSWORD INET REBUILD(true/false) SLAVE(false[default]/true)";
    exit 1
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

OS=`cat /etc/os-release`
if [[ "$OS" =~ "CentOS" ]]; then
  OS=1
elif [[ "$OS" =~ "Ubuntu" ]]; then
  OS=2
else
  echo "OS not support:"
  echo $OS
  exit 1
fi

function bash_rc()
{
  if [ $OS == 1 ]; then
    echo ".bashrc"
  else
    echo ".profile"
  fi
}

function exec_profile()
{
  if [ $OS == 1 ]; then
    source ~/.bashrc
  else
    source ~/.profile
  fi
}

function get_host_ip()
{
  if [ $OS == 1 ]; then
    IP=`ifconfig | grep $1 -A3 | grep inet | grep broad | awk '{print $2}'`
  else
    IP=`ifconfig | grep $1 -A3 | grep inet | awk -F':' '{print $2}' | awk '{print $1}'`
  fi
  echo "$IP"
}

now_user=`whoami`

if [ $now_user != "root" ]; then
  echo "User error, must be root user! Now user is:"$now_user;
  exit 1;
fi

if [ $OS == 1 ]; then
  cp centos7_base.repo /etc/yum.repos.d/
  cp epel-7.repo /etc/yum.repos.d/
  cp MariaDB.repo /etc/yum.repos.d/
  yum makecache fast

  yum install -y yum-utils psmisc MariaDB-client telnet net-tools wget unzip
else
  apt-get install -y psmisc mysql-client telnet net-tools wget unzip
fi

#获取主机hostip
for N in ${INET[@]};
do
    HOSTIP=$(get_host_ip $N)

    if [ "$HOSTIP" != "127.0.0.1" ] && [ "$HOSTIP" != "" ]; then
      break
    fi
done

if [ "$HOSTIP" == "127.0.0.1" ] || [ "$HOSTIP" == "" ]; then
    echo "HOSTIP is [$HOSTIP], not valid. HOSTIP must not be 127.0.0.1 or empty."
    exit 1
fi

if [ "${SLAVE}" != "true" ]; then

  if [ ! -d ${workdir}/web ]; then
      LOG_ERROR "no web exits, please copy TarsWeb to ${workdir}/web first."
      exit 1
  fi

  ################################################################################
  #download nodejs

  exec_profile

  CURRENT_NODE_VERSION=`node --version`

  if [ "${CURRENT_NODE_VERSION}" != "${NODE_VERSION}" ]; then

    export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release/

    rm -rf v0.35.1.zip
    wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;unzip v0.35.1.zip
    cp -rf nvm-0.35.1 $HOME/.nvm

    echo 'export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";' >> $HOME/$(bash_rc)

    exec_profile

    nvm install ${NODE_VERSION};
  fi

  ################################################################################
  #check node version
  CURRENT_NODE_VERSION=`node --version`

  if [ "${CURRENT_NODE_VERSION}" != "${NODE_VERSION}" ]; then
      echo "node is not valid, must be:${NODE_VERSION}"
      exit 1
  fi

  echo "install node success! Version is ${NODE_VERSION}"

  exec_profile
fi

npm config set registry ${MIRROR}/npm/; npm install -g npm pm2

#cd ${workdir}/web; npm prune;npm i --package-lock-only;npm audit fix
#cd ${workdir}/web; npm prune;npm i --package-lock-only;npm audit fix

################################################################################

cd ${workdir}

./tars-install.sh ${MYSQLIP} ${PORT} ${USER} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE}

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


