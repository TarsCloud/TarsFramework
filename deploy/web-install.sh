#!/bin/bash

OS=`uname`

if [[ "$OS" =~ "Darwin" ]]; then
    OS=2
else
    OS=1
fi

function LOG_INFO()
{
	if (( $# < 1 ))
	then
		LOG_WARNING "Usage: LOG_INFO logmsg";
	fi

	local msg=$(date +%Y-%m-%d" "%H:%M:%S);

	for p in $@
	do
		msg=${msg}" "${p};
	done

	echo -e "\033[32m $msg \033[0m"
}

if (( $# < 5 ))
then
    echo "$0 MYSQL_IP HOSTIP SLAVE(false[default]/true) MYSQL_PORT TARS_PATH";
    exit 1
fi


MYSQLIP=$1
HOSTIP=$2
SLAVE=$3
PORT=$4
TARS_PATH=$5

if [ "${SLAVE}" != "true" ]; then
    SLAVE="false"
fi

WORKDIR=$(cd $(dirname $0); pwd)

################################################################################
#deploy & start web
if [ "$SLAVE" != "true" ]; then

    cd ${TARS_PATH}

    mkdir -p ${TARS_PATH}/web/files/

    TARS=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)

    for var in ${TARS[@]};
    do
        echo "tar czf ${var}.tgz ${var}"
        tar czf ${var}.tgz ${var}
        cp -rf ${var}.tgz ${TARS_PATH}/web/files/
        rm -rf ${var}.tgz
    done

    cp ${WORKDIR}/tools/install.sh ${TARS_PATH}/web/files/

    cd ${WORKDIR}
    LOG_INFO "copy web to web path:/usr/local/app/";

    rm -rf web/log
    cp -rf web /usr/local/app/

    LOG_INFO "update web config";

    if [ $OS == 2 ]; then
        sed -i "" "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/config/webConf.js`
        sed -i "" "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl /usr/local/app/web/config/webConf.js`
        sed -i "" "s/3306/$PORT/g" `grep 3306 -rl /usr/local/app/web/config/webConf.js`
        sed -i "" "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl /usr/local/app/web/config/tars.conf`

        sed -i "" "s/enableAuth: false/enableAuth: true/g" /usr/local/app/web/config/authConf.js
        sed -i "" "s/enableLogin: false/enableLogin: true/g" /usr/local/app/web/config/loginConf.js

        sed -i "" "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/demo/config/webConf.js`
        sed -i "" "s/3306/$PORT/g" `grep 3306 -rl /usr/local/app/web/demo/config/webConf.js`

        sed -i "" "s/enableLogin: false/enableLogin: true/g" /usr/local/app/web/demo/config/loginConf.js
    else
        sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/config/webConf.js`
        sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl /usr/local/app/web/config/webConf.js`
        sed -i "s/3306/$PORT/g" `grep 3306 -rl /usr/local/app/web/config/webConf.js`
        sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl /usr/local/app/web/config/tars.conf`

        sed -i "s/enableAuth: false/enableAuth: true/g" /usr/local/app/web/config/authConf.js
        sed -i "s/enableLogin: false/enableLogin: true/g" /usr/local/app/web/config/loginConf.js

        sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/demo/config/webConf.js`
        sed -i "s/3306/$PORT/g" `grep 3306 -rl /usr/local/app/web/demo/config/webConf.js`

        sed -i "s/enableLogin: false/enableLogin: true/g" /usr/local/app/web/demo/config/loginConf.js
    fi

    LOG_INFO "start web";

    cd /usr/local/app/web; pm2 stop tars-node-web; pm2 delete tars-node-web; npm run prd; 
    cd /usr/local/app/web/demo; pm2 stop tars-user-system;  pm2 delete tars-user-system; npm run prd

    LOG_INFO "INSTALL TARS SUCC: http://$HOSTIP:3000/ to open the tars web."
    LOG_INFO "If in Docker, please check you host ip and port."
    LOG_INFO "You can start tars web manual: cd /usr/local/app/web; npm run prd"
    LOG_INFO "You can install tarsnode to other machine in web(>=1.3.1)"
    LOG_INFO "==============================================================";
else
    LOG_INFO "Install slave($SLAVE) node success"
    LOG_INFO "==============================================================";
fi
