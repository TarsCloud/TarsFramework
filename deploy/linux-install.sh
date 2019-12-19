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

TARS_PATH=/usr/local/app/tars
MIRROR=http://mirrors.cloud.tencent.com

WORKDIR=$(cd $(dirname $0); pwd)

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

  yum install -y yum-utils psmisc MariaDB-client telnet net-tools wget unzip gcc gcc-c++
else
  apt-get install -y psmisc mysql-client telnet net-tools wget unzip gcc gcc-c++
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

  if [ ! -d ${WORKDIR}/web ]; then
      echo "no web exits, please copy TarsWeb to ${WORKDIR}/web first:"
      echo "cd ${WORKDIR}; git clone https://github.com/TarsCloud/TarsWeb.git web"
      exit 1
  fi

  if [ ! -d ${WORKDIR}/web/demo ]; then
      echo "web not the newest version, please update to the newest version."
      exit 1
  fi

  cp -rf ${WORKDIR}/web/sql/*.sql ${WORKDIR}/framework/sql/
  cp -rf ${WORKDIR}/web/demo/sql/*.sql ${WORKDIR}/framework/sql/
 
  ################################################################################
  #download nodejs

  exec_profile

  CURRENT_NODE_VERSION=`node --version`

  export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release/

  if [ "${CURRENT_NODE_VERSION}" != "${NODE_VERSION}" ]; then

    rm -rf v0.35.1.zip
    wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;unzip v0.35.1.zip
    rm -rf $HOME/.nvm; rm -rf $HOME/.npm; cp -rf nvm-0.35.1 $HOME/.nvm; rm -rf nvm-0.35.1;

    echo 'export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";' >> $HOME/$(bash_rc)

    exec_profile

    nvm install ${NODE_VERSION};
  fi

  ################################################################################
  #check node version
  CURRENT_NODE_VERSION=`node --version`

  if [ "${CURRENT_NODE_VERSION}" != "${NODE_VERSION}" ]; then
      echo "node is not valid, must be:${NODE_VERSION}, please remove your node first."
      exit 1
  fi

  echo "install node success! Version is ${NODE_VERSION}"

  exec_profile

  cd web; npm install;
  cd demo; npm install;
fi

npm config set registry ${MIRROR}/npm/; npm install -g npm pm2

################################################################################

TARS=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)

cd ${WORKDIR}/framework/servers;
for var in ${TARS[@]};
do
  echo "tar czf ${var}.tgz ${var}"
  tar czf ${var}.tgz ${var}
done

################################################################################

cd ${WORKDIR}

./tars-install.sh ${MYSQLIP} ${PORT} ${USER} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE}

exec_profile
