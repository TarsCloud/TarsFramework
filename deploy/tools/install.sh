#! /bin/bash

WIN_TARSPATH=c:/tars-install/tars

if [ $# -lt 3 ]; then
    echo $#
    echo "$0 WEB_HOST(http://172.17.0.4:3000) LOCAL_IP(xx.xx.xx.xx) REGISTRY_ADDRESS(xx.xx.xx.xx:port)";
    exit 1
fi

weburl=$1
localIp=$2
registry=$3

echo "----------------------------------------------------------------------"
echo "===>print config info >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
echo "PARAMS:           "$*
echo "WEB_HOST:         "$weburl
echo "LOCAL_IP:         "$localIp 
echo "REGISTRY_ADDRESS: "$registry
echo "===<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< print config info finish.\n";

cd /tmp;
echo "weburl:"$weburl
echo "download tarsnode from ${weburl}/files/tarsnode.tgz:"

rm -rf tarsnode.tgz
wget ${weburl}/files/tarsnode.tgz 

if [ ! -f tarsnode.tgz ]; then
  echo "download tarsnode.tgz failed, please check web host !"
  exit 1
fi

tar zxf tarsnode.tgz

mkdir -p ${WIN_TARSPATH} 

if [ -d "${WIN_TARSPATH}/tarsnode" ]; then
  ${WIN_TARSPATH}/tarsnode/util/stop.sh
  sleep 1
fi

cp -rf tarsnode ${WIN_TARSPATH} 

ip=`echo ${registry} | awk -F':' '{print $1}'`
port=`echo ${registry} | awk -F':' '{print $2}'`

if [ "$ip" == "" ] || [ "$port" == "" ]; then
  echo "address invalid, must like this: 172.168.1.2:17890, please check!"
  exit 1
fi

registryAddress="tcp -h $ip -p $port"

echo "all tars registry:" $registryAddress

echo "update tarsnode conf"

sed -i "s/localip.tars.com/${localIp}/g" ${WIN_TARSPATH}/tarsnode/conf/tars.tarsnode.config.conf 

sed -i "s/registryAddress/${registryAddress}/g" ${WIN_TARSPATH}/tarsnode/conf/tars.tarsnode.config.conf

sed -i "s/registryAddress/${registryAddress}/g" ${WIN_TARSPATH}/tarsnode/util/*.sh

echo "install tarsnode succ"

${WIN_TARSPATH}/tarsnode/util/start.sh

echo "start tarsnode finish"