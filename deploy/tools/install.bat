#! /bin/bash

echo "----------------------------------------------------------------------"
echo "begin check dependency tools"
now_user=`whoami`

if [ $now_user != "root" ]; then
  echo "User error, must be root user! Now user is:"$now_user;
  exit 1;
fi

RESULT=`ifconfig --version`
echo $RESULT | grep -q "tools"
if [ $? == 0 ]; then
    echo "please install ifconfig first.(yum install net-tools or apt-get install net-tools)"
    exit 1
fi

RESULT=`nc --version`
echo $RESULT | grep -q "Ncat"
if [ $? == 0 ]; then
    echo "please install nc first.(yum install nc or apt-get install nc)"
    exit 1
fi

echo "all dependency satisfied."
echo "----------------------------------------------------------------------"

cd /tmp;
while [ 1 ]
do
  echo ""
  read -p "Please input your tars web url (like this: http://172.17.0.4:3000):" weburl

  echo "weburl:"$weburl
  echo "download tarsnode from ${weburl}/files/tarsnode.tgz:"

  rm -rf tarsnode.tgz
  wget ${weburl}/files/tarsnode.tgz 

  if [ ! -f tarsnode.tgz ]; then
    echo "download tarsnode.tgz failed, please input tars web again!"
    continue
  fi
  
  break
done

tar zxf tarsnode.tgz

if [ ! -d "/usr/local/app" ]
then
    echo "create tars base path: "
    mkdir -p /data/app
    ln -s /data/app /usr/local/app
fi

mkdir -p /usr/local/app/tars

if [ -d "/usr/local/app/tars/tarsnode" ]; then
   read -p "tarsnode exist, overwrite?(Y/N):" Y
   if [ "$Y" != "Y" ] && [ "$Y" != "y" ]; then
       echo "exit"
       exit 0
   fi
fi

cp -rf tarsnode /usr/local/app/tars/

while [ 1 ]
do
  echo ""
  read -p "Please input local machine ip:" localIp

  RESULT=`ifconfig | grep ${localIp}`

  if [ "${RESULT}" == "" ]; then
    echo "get local ip fail, please input again!"
  else
    break
  fi
done

echo "local machine ip:$localIp succ"

registryAddress=""

while [ 1 ]
do
    echo ""
    read -p "Please input tars registry address(like this: 172.168.1.2 17890):" ip port

    if [ "$ip" == "" ] || [ "$port" == "" ]; then
        echo "address invalid, must like this: 172.168.1.2 17890, please input again"
        continue
    fi

    nc -w 10 -z $ip $port > /dev/null 2>&1  
    if [ $? != 0 ]; then  
      echo "tars registry:"$ip $port" can not connect, please input again."
      continue
    fi  

    echo "tars registry [$ip:$port] connect succ"

    if [ "${registryAddress}" != "" ]; then
      registryAddress="$registryAddress:tcp -h $ip -p $port"
    else
      registryAddress="tcp -h $ip -p $port"
    fi

    echo ""
    read -p "Add anthor registry?(Y/N):" Y

    if [ "$Y" != "Y" ] && [ "$Y" != "y" ]; then
        break;
    fi

done

echo "all tars registry:" $registryAddress

echo "update tarsnode conf"

sed -i "s/localip.tars.com/${localIp}/g" /usr/local/app/tars/tarsnode/conf/tars.tarsnode.config.conf 

sed -i "s/registryAddress/${registryAddress}/g" /usr/local/app/tars/tarsnode/conf/tars.tarsnode.config.conf

sed -i "s/registryAddress/${registryAddress}/g" /usr/local/app/tars/tarsnode/util/execute.sh

echo "install tarsnode succ, start tarsnode"

sh /usr/local/app/tars/tarsnode/util/start.sh

ps -aux | grep bin/tarsnode
