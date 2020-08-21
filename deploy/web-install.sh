#!/bin/bash

OSNAME=`uname`
OS=1

if [[ "$OSNAME" == "Darwin" ]]; then
    OS=2
elif [[ "$OSNAME" == "Windows_NT" ]]; then
    OS=3
else
    OS=1
fi


function LOG_INFO()
{
	local msg=$(date +%Y-%m-%d" "%H:%M:%S);

	for p in $@
	do
		msg=${msg}" "${p};
	done

	echo -e "\033[32m $msg \033[0m"
}

if [ $# -lt 9 ]; then
    echo "$0 MYSQL_IP MYSQL_PASSWORD  HOSTIP REBUILD(false[default]/true) SLAVE(false[default]/true) MYSQL_USER MYSQL_PORT TARS_PATH WEB_PATH";
    exit 1
fi

MYSQLIP=$1
PASS=$2
HOSTIP=$3
REBUILD=$4
SLAVE=$5
USER=$6
PORT=$7
TARS_PATH=$8
WEB_PATH=$9

if [ "${SLAVE}" != "true" ]; then
    SLAVE="false"
fi

WORKDIR=$(cd $(dirname $0); pwd)

if [ $OS == 3 ]; then
    MYSQL_TOOL=${WORKDIR}/mysql-tool.exe
else
    MYSQL_TOOL=${WORKDIR}/mysql-tool
fi

#输出配置信息
LOG_INFO "===>install web >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
LOG_INFO "PARAMS:        "$*
LOG_INFO "OS:            "$OSNAME"="$OS
LOG_INFO "MYSQLIP:       "$MYSQLIP 
LOG_INFO "USER:          "$USER
LOG_INFO "PASS:          "$PASS
LOG_INFO "PORT:          "$PORT
LOG_INFO "HOSTIP:        "$HOSTIP
LOG_INFO "WORKDIR:       "$WORKDIR
LOG_INFO "SLAVE:         "${SLAVE}
LOG_INFO "REBUILD:       "${REBUILD}
LOG_INFO "TARS_PATH:     "${TARS_PATH}
LOG_INFO "WEB_PATH:      "${WEB_PATH}
LOG_INFO "===<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< print config info finish.\n";

################################################################################

function replacePath()
{
    SRC=$1
    DST=$2
    SCAN_PATH=$3

    FILES=`grep "${SRC}" -rl $SCAN_PATH/*`

    if [ "$FILES" == "" ]; then
        return
    fi

    for file in $FILES;
    do
        ${MYSQL_TOOL} --src="${SRC}" --dst="${DST}" --replace=$file 
    done
}

# exit 0
function update_conf() 
{
    UPDATE_PATH=$1

    replacePath localip.tars.com $HOSTIP ${UPDATE_PATH}
    replacePath db.tars.com $MYSQLIP ${UPDATE_PATH}
    replacePath registry.tars.com $HOSTIP ${UPDATE_PATH}
    replacePath 3306 $PORT ${UPDATE_PATH}
    replacePath "user: 'tars'" "user: '${USER}'" ${UPDATE_PATH}
    replacePath "password: 'tars2015'" "password: '${PASS}'" ${UPDATE_PATH}
    replacePath "/usr/local/app" "$WEB_PATH" ${UPDATE_PATH}
    replacePath "enableAuth: false" "enableAuth: true" ${UPDATE_PATH}
    replacePath "enableLogin: false" "enableLogin: true" ${UPDATE_PATH}
}

################################################################################
#deploy & start web
if [ "$SLAVE" != "true" ]; then

    cd ${WORKDIR}

    if [ "${TARS_IN_DOCKER}" != "true" ]; then
      LOG_INFO "copy web to web path:${WEB_PATH}/";

       #delete old version: web/demo
      if [ ! -f web/demo/package.json ]; then
        LOG_INFO "update to new version, delete ${WEB_PATH}/web/demo";

        pm2 -s delete tars-user-system;

        rm -rf ${WEB_PATH}/web/demo

      fi

      cp -rf web ${WEB_PATH}
    fi
    rm -rf web/log

    LOG_INFO "update web config";

    update_conf ${WEB_PATH}/web/config

    if [ -f ${WEB_PATH}/web/demo/package.json ]; then
        LOG_INFO "update web/demo config";
        update_conf ${WEB_PATH}/web/demo/config
    fi

    LOG_INFO "start web";

    cd ${WEB_PATH}/web; pm2 -s stop tars-node-web ; pm2 -s delete tars-node-web; npm run prd; 

    if [ -f ${WEB_PATH}/web/demo/package.json ]; then
        cd ${WEB_PATH}/web/demo; pm2 -s stop tars-user-system;  pm2 -s delete tars-user-system; npm run prd
    fi

    LOG_INFO "INSTALL TARS SUCC: http://$HOSTIP:3000/ to open the tars web."
    LOG_INFO "If in Docker, please check you host ip and port."
    LOG_INFO "You can start tars web manual: cd ${WEB_PATH}/web; npm run prd"
    LOG_INFO "You can install tarsnode to other machine in web(>=1.3.1)"
    LOG_INFO "==============================================================";
else
    LOG_INFO "Install slave($SLAVE) node success"
    LOG_INFO "==============================================================";
fi
