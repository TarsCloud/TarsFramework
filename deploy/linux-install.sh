#!/bin/bash

#./linux-install.sh 192.168.7.152 Rancher@12345 eth0 false false
#./inux-install.sh 192.168.7.152 Rancher@12345 eth0 true false

if (( $# < 7 ))
then
    echo $#
    echo "$0 MYSQL_IP MYSQL_PASSWORD INET REBUILD(true/false) SLAVE(false[default]/true) MYSQL_USER MYSQL_PORT";
    exit 1
fi

MYSQLIP=$1
PASS=$2
INET=$3
REBUILD=$4
SLAVE=$5
USER=$6
PORT=$7

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

#########################################################################

NODE_VERSION="v12.13.0"

TARS_PATH=/usr/local/app/tars
MIRROR=http://mirrors.cloud.tencent.com

export TARS_INSTALL=$(cd $(dirname $0); pwd)

OS=`uname`

if [[ "$OS" =~ "Darwin" ]]; then
    OS=3
else
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
fi

function bash_rc()
{
  if [ $OS == 3 ]; then
    echo ".bash_profile"
  elif [ $OS == 1 ]; then
    echo ".bashrc"
  else
    echo ".profile"
  fi
}

function exec_profile()
{
  if [ $OS == 3 ]; then
    source ~/.bash_profile
  elif [ $OS == 1 ]; then
    source ~/.bashrc
  else
    source ~/.profile
  fi
}

function get_host_ip()
{
  if [ $OS == 1 ]; then
    IP=`ifconfig | grep $1 -A3 | grep inet | grep broad | awk '{print $2}'`
  elif [ $OS == 2 ]; then
    IP=`ifconfig | sed 's/addr//g' | grep $1 -A3 | grep "inet " | awk -F'[ :]+' '{print $3}'`
  elif [ $OS == 3 ]; then
    IP=`ifconfig | grep $1 -A4 | grep "inet " | awk '{print $2}'`
  fi
  echo "$IP"
}

if [ $OS != 3 ]; then

    now_user=`whoami`

    if [ $now_user != "root" ]; then
      echo "User error, must be root user! Now user is:"$now_user;
      exit 1;
    fi

    if [ $OS == 1 ]; then

      cp centos7_base.repo /etc/yum.repos.d/
      yum makecache fast

      yum install -y yum-utils psmisc mysql telnet net-tools wget unzip
    else
      apt-get update
      apt-get install -y psmisc mysql-client telnet net-tools wget unzip
    fi

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

  if [ ! -d ${TARS_INSTALL}/web ]; then
      echo "no web exits, please copy TarsWeb to ${TARS_INSTALL}/web first:"
      echo "cd ${TARS_INSTALL}; git clone https://github.com/TarsCloud/TarsWeb.git web"
      exit 1
  fi

  if [ ! -d ${TARS_INSTALL}/web/demo ]; then
      echo "web not the newest version, please update to the newest version."
      exit 1
  fi

  ################################################################################
  #download nodejs

  exec_profile

  CURRENT_NODE_VERSION=`node --version`

  export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release/

  if [ "${CURRENT_NODE_VERSION}" != "${NODE_VERSION}" ]; then

    rm -rf v0.35.1.zip
    #centos8 need chmod a+x
    chmod a+x /usr/bin/unzip
    wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;/usr/bin/unzip v0.35.1.zip
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

cp -rf ${TARS_INSTALL}/web/sql/*.sql ${TARS_INSTALL}/framework/sql/
cp -rf ${TARS_INSTALL}/web/demo/sql/*.sql ${TARS_INSTALL}/framework/sql/

# strip ${TARS_INSTALL}/framework/servers/tars*/bin/tars*
chmod a+x ${TARS_INSTALL}/framework/servers/tars*/util/*.sh

TARS=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)

cd ${TARS_INSTALL}/framework/servers;
for var in ${TARS[@]};
do
  echo "tar czf ${var}.tgz ${var}"
  tar czf ${var}.tgz ${var}
done

mkdir -p ${TARS_INSTALL}/web/files/
cp -rf ${TARS_INSTALL}/framework/servers/*.tgz ${TARS_INSTALL}/web/files/
rm -rf ${TARS_INSTALL}/framework/servers/*.tgz
cp ${TARS_INSTALL}/tools/install.sh ${TARS_INSTALL}/web/files/

################################################################################

cd ${TARS_INSTALL}

./tars-install.sh ${MYSQLIP} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE} ${USER}  ${PORT} ${TARS_PATH}

exec_profile
