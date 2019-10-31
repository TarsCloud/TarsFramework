#! /bin/bash

cd tmp;
wget http://web.tars.com:3000/install/tarsnode_install.tgz 
tar -zxvf tarsnode_install.tgz
if [ ! -d "/usr/local/app" ]
then
    echo "create tars base path: "
    mkdir -p /data/app
    ln -s /data/app /usr/local/app
fi

mkdir -p /usr/local/app/tars
mv ./tarsnode_install/tarsnode /usr/local/app/tars/

localIp=`ifconfig | grep inet | grep broadcast | awk '{print $2}'`
echo "local ip is:"${localIp}
if [ "${localIp}" == "" ]
then
    echo "get localip fail!"
    exit 1
fi

sed -i "s/localip.tars.com/${localIp}/g" /usr/local/app/tars/tarsnode/conf/tars.tarsnode.config.conf 
/usr/local/app/tars/tarsnode/util/start.sh

echo "install tarsnode finish, result is:"
ps -aux | grep bin/tarsnode
