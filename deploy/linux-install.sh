#!/bin/bash

#./linux-install.sh 192.168.7.152 Rancher@12345 eth0 false false
#./linux-install.sh 192.168.7.152 Rancher@12345 eth0 true false

if (( $# < 7 ))
then
    echo $#
    echo "$0 MYSQL_IP MYSQL_PASSWORD INET REBUILD(true/false) SLAVE(false[default]/true) MYSQL_USER MYSQL_PORT OVERWRITE(false/true[default]) NODE_MIRROR(tencentcloud[default]/aliyun)";
    exit 1
fi

MYSQLIP=$1
PASS=$2
INET=$3
REBUILD=$4
SLAVE=$5
DBUSER=$6
PORT=$7
OVERWRITE=$8
MIRROR=$9

if [ "$DBUSER" = "" ]; then
    DBUSER="root"
fi

if [ "$PORT" = "" ]; then
    PORT="3306"
fi

if [ "$INET" = "" ]; then
    INET=(eth0)
fi

if [ "$REBUILD" != "true" ]; then
  REBUILD="false"
fi

if [ "$SLAVE" != "true" ]; then
    SLAVE="false"
fi

if [ "$OVERWRITE" != "true" ]; then
    OVERWRITE="false"
fi

if [ "$MIRROR" = "" ]; then
    MIRROR="tencentcloud"
fi
#########################################################################

NODE_VERSION="v12.13.0"

INSTALL_PATH=/usr/local/app

if [ "$MIRROR" = "tencentcloud" ]; then
    NODEJS_MIRROR=http://mirrors.cloud.tencent.com/nodejs-release/
    NPM_MIRROR=http://mirrors.cloud.tencent.com/npm/
    rm -rf centos7_base.repo
    curl -O http://mirrors.cloud.tencent.com/repo/centos7_base.repo
elif [ "$MIRROR" = "aliyun" ]; then
    NODEJS_MIRROR=http://npm.taobao.org/mirrors/node/
    NPM_MIRROR=https://registry.npm.taobao.org
    rm -rf centos7_base.repo
    curl -O centos7_base.repo http://mirrors.aliyun.com/repo/Centos-7.repo
else
    echo "Mirror not support, please add it to this script manually."
    exit 1
fi


export TARS_INSTALL=$(cd $(dirname $0); pwd)

OS=`uname`

if [[ "$OS" =~ "Darwin" ]]; then
    OS=3
else
    OS=`cat /etc/redhat-release`
    if [[ "$OS" =~ "CentOS release 6" ]]; then
      OS=1
      NODE_VERSION="v10.20.1"
    else
      OS=`cat /etc/os-release`
      if [[ "$OS" =~ "CentOS" ]] || [[ "$OS" =~ "Tencent tlinux" ]]; then
        OS=1
      elif [[ "$OS" =~ "Ubuntu" ]]; then
        OS=2
      else
        echo "OS not support:"
        echo $OS
        exit 1
      fi
    fi
fi

function exec_profile()
{
  source /etc/profile
  source ~/.bashrc
}

function get_host_ip()
{
  if [ $OS == 1 ]; then
    IP=`ifconfig | grep $1 -A 1 | tail -1 | awk '{print $2}' | cut -d ':' -f 2`
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

      yum install -y yum-utils psmisc telnet net-tools wget unzip
    else
      apt-get update
      apt-get install -y psmisc telnet net-tools wget unzip
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

#if [ "$HOSTIP" == "127.0.0.1" ] || [ "$HOSTIP" == "" ]; then
#    echo "HOSTIP is [$HOSTIP], not valid. HOSTIP must not be 127.0.0.1 or empty."
#    exit 1
#fi

function version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }

if [ "${SLAVE}" != "true" ]; then

  if [ ! -d ${TARS_INSTALL}/web ]; then
      echo "no web exits, please copy TarsWeb to ${TARS_INSTALL}/web first:"
      echo "cd ${TARS_INSTALL}; git clone https://github.com/TarsCloud/TarsWeb.git web"
      exit 1
  fi

  # if [ ! -d ${TARS_INSTALL}/web/demo ]; then
  #     echo "web not the newest version, please update to the newest version."
  #     exit 1
  # fi

  ################################################################################
  #download nodejs

  exec_profile

  CURRENT_NODE_SUCC=`node -e "console.log('succ')"`
  CURRENT_NODE_VERSION=`node --version`

  export NVM_NODEJS_ORG_MIRROR=${NODEJS_MIRROR}

  if [[ "${CURRENT_NODE_SUCC}" != "succ" ]] || version_lt "${CURRENT_NODE_VERSION}" "${NODE_VERSION}" ; then

    rm -rf v0.35.1.zip
    #centos8 need chmod a+x
    chmod a+x /usr/bin/unzip
    wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip --no-check-certificate;/usr/bin/unzip v0.35.1.zip

    NVM_HOME=$HOME

    rm -rf $NVM_HOME/.nvm; rm -rf $NVM_HOME/.npm; cp -rf nvm-0.35.1 $NVM_HOME/.nvm; rm -rf nvm-0.35.1;

    NVM_DIR=$NVM_HOME/.nvm;
    echo "export NVM_DIR=$NVM_DIR; [ -s $NVM_DIR/nvm.sh ] && \. $NVM_DIR/nvm.sh; [ -s $NVM_DIR/bash_completion ] && \. $NVM_DIR/bash_completion;" >> /etc/profile

    exec_profile

    nvm install ${NODE_VERSION};
  fi

  ################################################################################
  #check node version
  CURRENT_NODE_VERSION=`node --version`

  if [[ "${CURRENT_NODE_VERSION}" < "${NODE_VERSION}" ]]; then
      echo "node is not valid, must be after version:${NODE_VERSION}, please remove your node first."
      exit 1
  fi

  echo "install node success! Version is ${CURRENT_NODE_VERSION}"

  exec_profile

  cd web; npm install;

  if [ -f demo/package.json ]; then
    cd demo; npm install;
  fi

fi

npm config set registry ${NPM_MIRROR}; npm install -g npm pm2

################################################################################

cd ${TARS_INSTALL}

./tars-install.sh ${MYSQLIP} ${PASS} ${HOSTIP} ${REBUILD} ${SLAVE} ${DBUSER}  ${PORT} ${INSTALL_PATH} ${OVERWRITE}


################################################################################

exec_profile

################################################################################
