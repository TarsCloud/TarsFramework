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

function kill_all()
{
  if [ $OS == 2 ]; then
    killall -9 $1
  elif [ $OS == 3 ]; then
    killall -q -9 $1
  else
    killall -9 -q $1
  fi
}

function netstat_port()
{
  if [ $OS == 2 ]; then
    netstat -anL
  elif [ $OS == 3 ]; then
    netstat -an -p TCP
  else
    netstat -lpn
  fi
}

#公共函数
function LOG_ERROR()
{
	local msg=$(date +%Y-%m-%d" "%H:%M:%S);

    msg="${msg} $@";

	echo -e "\033[31m $msg \033[0m";	
}

function LOG_WARNING()
{
	local msg=$(date +%Y-%m-%d" "%H:%M:%S);

    msg="${msg} $@";

	echo -e "\033[33m $msg \033[0m";	
}

function LOG_DEBUG()
{
	local msg=$(date +%Y-%m-%d" "%H:%M:%S);

    msg="${msg} $@";

 	echo -e "\033[40;37m $msg \033[0m";	
}

function LOG_INFO()
{
	local msg=$(date +%Y-%m-%d" "%H:%M:%S);
	
	for p in $@
	do
		msg=${msg}" "${p};
	done
	
	echo -e "\033[32m $msg \033[0m"  	
}

if [ $# -lt 8 ]; then
    echo "$0 MYSQL_IP MYSQL_PASSWORD  HOSTIP REBUILD(false[default]/true) SLAVE(false[default]/true) MYSQL_USER MYSQL_PORT INSTALL_PATH OVERWRITE(false[default]/true)";
    exit 1
fi

MYSQLIP=$1
PASS=$2
HOSTIP=$3
REBUILD=$4
SLAVE=$5
USER=$6
PORT=$7
INSTALL_PATH=$8
OVERWRITE=$9

if [ "${SLAVE}" != "true" ]; then
    SLAVE="false"
fi

if [ "$OVERWRITE" != "true" ]; then
    OVERWRITE="false"
fi

if [ "${SLAVE}" != "true" ]; then
    TARS="tarsAdminRegistry tarsregistry tarsconfig tarsnode tarsnotify tarsproperty tarsqueryproperty tarsquerystat tarsstat tarslog tarspatch"
else
    TARS="tarsregistry tarsconfig tarsnode tarsnotify tarsqueryproperty tarsquerystat"
fi

if [ $OS != 3 ]; then
    if [ "${INSTALL_PATH}" == "" ]; then
        INSTALL_PATH=/usr/local/app
    fi
else
    if [ "${INSTALL_PATH}" == "" ]; then
        INSTALL_PATH="c:/tars-install"
    fi
fi

TARS_PATH=${INSTALL_PATH}/tars
UPLOAD_PATH=$INSTALL_PATH
WEB_PATH=$INSTALL_PATH

mkdir -p ${TARS_PATH}

TARSALL="tarsregistry tarsAdminRegistry tarsconfig tarsnode tarslog tarsnotify  tarspatch  tarsproperty tarsqueryproperty tarsquerystat  tarsstat"

WORKDIR=$(cd $(dirname $0); pwd)

if [ $OS == 3 ]; then
    MYSQL_TOOL=${WORKDIR}/mysql-tool.exe
else
    MYSQL_TOOL=${WORKDIR}/mysql-tool
fi

TARS_USER=tarsAdmin
TARS_PASS=Tars@2019

LOG_INFO "====================================================================";
LOG_INFO "===**********************tars-install*****************************===";
LOG_INFO "====================================================================";

#输出配置信息
LOG_DEBUG "===>print config info >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
LOG_DEBUG "PARAMS:        "$*
LOG_DEBUG "OS:            "$OSNAME"="$OS
LOG_DEBUG "MYSQLIP:       "$MYSQLIP 
LOG_DEBUG "USER:          "$USER
LOG_DEBUG "PASS:          "$PASS
LOG_DEBUG "PORT:          "$PORT
LOG_DEBUG "HOSTIP:        "$HOSTIP
LOG_DEBUG "WORKDIR:       "$WORKDIR
LOG_DEBUG "SLAVE:         "${SLAVE}
LOG_DEBUG "REBUILD:       "${REBUILD}
LOG_DEBUG "TARS_USER:     "${TARS_USER}
LOG_DEBUG "TARS_PASS:     "${TARS_PASS}
LOG_DEBUG "INSTALL_PATH:  "${INSTALL_PATH}
LOG_DEBUG "TARS_PATH:     "${TARS_PATH}
LOG_DEBUG "UPLOAD_PATH:   "${UPLOAD_PATH}/patchs
LOG_DEBUG "WEB_PATH:      "${WEB_PATH}/web
LOG_DEBUG "===<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< print config info finish.\n";

################################################################################
#killall all tars-servers
for var in $TARSALL;
do
    kill_all ${var}
done
for var in $TARSALL;
do
    kill_all ${var}
done

################################################################################
#check port
function check_ports()
{
    LOG_DEBUG "check port if conflict"
    PORTS="18993 18997 18793 18797 18693 18697 18193 18197 18593 18597 18493 18497 18393 18397 18293 18297 12000 19385 19387 17897 17890 17891 3000 3001"
    for P in $PORTS;
    do
        NETINFO=$(netstat_port)

        RESULT=`echo ${NETINFO} | grep ${HOSTIP}:${P}`
        if [ "$RESULT" != "" ]; then
            LOG_ERROR ${HOSTIP}:${P}", port maybe conflict, please check!"
        fi

        RESULT=`echo ${NETINFO} | grep 127.0.0.1:${P}`
        if [ "$RESULT" != "" ]; then
            LOG_ERROR 127.0.0.1:${P}", port maybe conflict, please check!"
            # exit 1
        fi
    done
}

# check_ports

################################################################################

function check_mysql()
{
    while [ 1 ]
    do
        ${MYSQL_TOOL} --host=${MYSQLIP} --user="$1" --pass="$2" --port=${PORT} --check

        if [ $? == 0 ]; then
            LOG_INFO "mysql is alive"
            return
        fi

        LOG_ERROR "check mysql is not alive: ${MYSQL_TOOL} --host=${MYSQLIP} --user="$1" --pass="$2" --port=${PORT} --check, try again"

        sleep 3
    done
}

################################################################################
#check mysql

check_mysql ${USER} ${PASS}


function exec_mysql_has()
{
    #echo "${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --charset=utf8 --has=$1"
    ${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --charset=utf8 --has=$1

    ret=$?
    LOG_DEBUG "exec_mysql_has $1, ret: $ret"

    return $ret
}

function exec_mysql_script()
{
    ${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --charset=utf8 --sql="$1"

    ret=$?
    LOG_DEBUG "exec_mysql_script $1, ret code: $ret"  

    return $ret
}

function exec_mysql_sql()
{
    #echo "${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --charset=utf8 --db=$1 --file=$2"
    ${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --charset=utf8 --tars-path=${TARS_PATH} --db=$1 --file=$2

    ret=$?

    LOG_DEBUG "exec_mysql_sql $1 $2, ret code: $ret"  

    return $ret
}

function exec_mysql_template()
{
    #echo "${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --charset=utf8 --parent=$1 --template=$2 --profile=$3"
    ${MYSQL_TOOL} --host=${MYSQLIP} --user=${TARS_USER} --pass=${TARS_PASS} --port=${PORT} --charset=utf8 --db=db_tars --upload-path=${UPLOAD_PATH} --tars-path=${TARS_PATH} --parent=$1 --template=$2 --profile=$3 --overwrite=$OVERWRITE

    ret=$?

    LOG_DEBUG "exec_mysql_template $1 $2, ret code: $ret"  

    return $ret
}

################################################################################

function replace()
{
    SRC=$1
    DST=$2
    SCAN_FILE=$3

    FILES=`grep "${SRC}" -rl $SCAN_FILE`

    if [ "$FILES" == "" ]; then
        return
    fi

    for file in $FILES;
    do
        ${MYSQL_TOOL} --src="${SRC}" --dst="${DST}" --replace=$file 
    done
}

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

################################################################################
#replacePath sql/template
SQL_TMP=${WORKDIR}/sql.tmp

mkdir -p ${SQL_TMP}

cp -rf ${WORKDIR}/web/sql/*.sql ${WORKDIR}/framework/sql/

FRAMEWORK_VERSION=`cat FRAMEWORK_VERSION.txt`
if [ "${FRAMEWORK_VERSION}" != "" ]; then 
    replacePath "2.1.0" ${FRAMEWORK_VERSION} ${WORKDIR}/framework/sql/
fi

if [ -d ${WORKDIR}/web/demo/sql ]; then
  cp -rf ${WORKDIR}/web/demo/sql/*.sql ${WORKDIR}/framework/sql/
fi

cp -rf ${WORKDIR}/framework/sql/* ${SQL_TMP}

replacePath localip.tars.com $HOSTIP ${SQL_TMP}

cd ${SQL_TMP}

while [ 1 ]
do
    MYSQL_VER=`${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --version`

    if [ $? == 0 ]; then
        LOG_INFO "mysql is alive, version: $MYSQL_VER"
        break
    fi

    LOG_ERROR "check mysql version failed, try again later!"

    sleep 3
done


# LOG_INFO "mysql version is: $MYSQL_VER"

if [ "${SLAVE}" != "true" ]; then

    if [ "$REBUILD" == "true" ]; then
        exec_mysql_script "drop database if exists db_tars"
        exec_mysql_script "drop database if exists tars_stat"
        exec_mysql_script "drop database if exists tars_property"
        exec_mysql_script "drop database if exists db_tars_web"    
        exec_mysql_script "drop database if exists db_user_system"    
        exec_mysql_script "drop database if exists db_cache_web"
    fi

    MYSQL_GRANT="SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, RELOAD, PROCESS, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE USER"

    if [ `echo $MYSQL_VER|grep ^8.` ]; then
        exec_mysql_script "CREATE USER '${TARS_USER}'@'%' IDENTIFIED WITH mysql_native_password BY '${TARS_PASS}';"
        exec_mysql_script "GRANT ${MYSQL_GRANT} ON *.* TO '${TARS_USER}'@'%' WITH GRANT OPTION;"
        exec_mysql_script "CREATE USER '${TARS_USER}'@'localhost' IDENTIFIED WITH mysql_native_password BY '${TARS_PASS}';"
        exec_mysql_script "GRANT ${MYSQL_GRANT} ON *.* TO '${TARS_USER}'@'localhost' WITH GRANT OPTION;"
        exec_mysql_script "CREATE USER '${TARS_USER}'@'${HOSTIP}' IDENTIFIED WITH mysql_native_password BY '${TARS_PASS}';"
        exec_mysql_script "GRANT ${MYSQL_GRANT} ON *.* TO '${TARS_USER}'@'${HOSTIP}' WITH GRANT OPTION;"
    elif [ `echo $MYSQL_VER|grep ^5.` ]; then
        exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'%' identified by '${TARS_PASS}' with grant option;"
        if [ $? != 0 ]; then
            LOG_DEBUG "grant error, exit." 
            exit 1
        fi

        exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'localhost' identified by '${TARS_PASS}' with grant option;"
        exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'$HOSTIP' identified by '${TARS_PASS}' with grant option;"
        exec_mysql_script "flush privileges;"
    else
        exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'%' identified by '${TARS_PASS}' with grant option;"
	if [ $? != 0 ]; then
	    LOG_DEBUG "grant error, exit."
	    exit 1
	fi

	exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'localhost' identified by '${TARS_PASS}' with grant option;"
	exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'$HOSTIP' identified by '${TARS_PASS}' with grant option;"
	exec_mysql_script "flush privileges;"
    fi
fi

################################################################################
#check mysql

check_mysql ${TARS_USER} ${TARS_PASS}

################################################################################

function update_template()
{
    LOG_INFO "create t_profile_template";

    function read_dir()
    {
        for template_name in `ls template`
        do
            LOG_INFO "read_dir template:"$template_name

            parent_template="tars.default"
            if [ "$template_name" == "tars.springboot" ]; then
                parent_template="tars.tarsjava.default"
            elif [ "$template_name" == "tars.tarsAdminRegistry" ]; then
                parent_template="tars.framework-db"
            elif [ "$template_name" == "tars.tarsconfig" ]; then
                parent_template="tars.framework-db"
            elif [ "$template_name" == "tars.tarsnotify" ]; then
                parent_template="tars.framework-db"
            elif [ "$template_name" == "tars.tarsproperty" ]; then
                parent_template="tars.framework-db"
            elif [ "$template_name" == "tars.tarsqueryproperty" ]; then
                parent_template="tars.framework-db"
            elif [ "$template_name" == "tars.tarsstat" ]; then
                parent_template="tars.framework-db"
            elif [ "$template_name" == "tars.tarsquerystat" ]; then
                parent_template="tars.framework-db"
            elif [ "$template_name" == "tars.tarsregistry" ]; then
                parent_template="tars.framework-db"
            fi

            exec_mysql_template $parent_template $template_name template/${template_name} 

        done
    }

    read_dir
}

if [ "${SLAVE}" != "true" ]; then
    exec_mysql_has "db_tars"
    if [ $? != 0 ]; then

        LOG_INFO "no db_tars exists, begin build db_tars..."  

        LOG_INFO "create database db_tars";

        exec_mysql_script "create database db_tars"

        exec_mysql_sql db_tars db_tars.sql
        
        #only new to create tarslog, becouse tarslog may be transfer to other node
        LOG_INFO "create log servers";
        exec_mysql_sql db_tars tars_servers_logs.sql
    fi

    exec_mysql_has "db_tars_web"
    if [ $? != 0 ]; then

        LOG_INFO "no db_tars_web exists, begin build db_tars_web..."  

        exec_mysql_script "create database db_tars_web"

        exec_mysql_sql db_tars_web db_tars_web.sql
    fi

    exec_mysql_has "tars_stat"
    if [ $? != 0 ]; then

        LOG_INFO "no tars_stat exists, begin build tars_stat..."  

        exec_mysql_script "create database tars_stat"
    fi

    exec_mysql_has "tars_property"
    if [ $? != 0 ]; then

        LOG_INFO "no tars_property exists, begin build tars_property..."  

        exec_mysql_script "create database tars_property"
    fi

    exec_mysql_has "db_cache_web"
    if [ $? != 0 ]; then
        LOG_INFO "no db_cache_web exists, begin build db_cache_web..."
        exec_mysql_script "create database db_cache_web"

        exec_mysql_sql db_cache_web db_cache_web.sql
    fi

    #update template
    update_template

    exec_mysql_has "db_user_system"
    if [ $? != 0 ]; then
        LOG_INFO "db_user_system not exists, begin build db_user_system..."  

        exec_mysql_script "create database db_user_system"

        exec_mysql_sql db_user_system db_user_system.sql
    fi

    exec_mysql_has "db_base"
    if [ $? != 0 ]; then
        LOG_INFO "db_base not exists, begin build db_base..."  

        exec_mysql_script "create database db_base"

        exec_mysql_sql db_base db_base.sql
    fi

    #current master server info
    LOG_INFO "create master servers";

    exec_mysql_sql db_tars tars_servers_master.sql
else
    exec_mysql_has "db_tars"
    if [ $? != 0 ]; then
        LOG_ERROR "no db_tars exists, please install master first, install will exit"  
        exit 1
    fi
fi

#current server info
LOG_INFO "create inner servers";
exec_mysql_sql db_tars tars_servers.sql

#current node info
LOG_INFO "create node info";
exec_mysql_sql db_tars tars_node_init.sql

cd ${WORKDIR}

LOG_INFO "prepare framework-------------------------------------"

FRAMEWORK_TMP=${WORKDIR}/framework.tmp

mkdir -p ${FRAMEWORK_TMP}

for var in $TARSALL;
do
    cp -rf ${WORKDIR}/framework/servers/${var} ${FRAMEWORK_TMP}
    cp -rf ${WORKDIR}/framework/conf/${var} ${FRAMEWORK_TMP}

    if [ $OS != 3 ]; then
        cp -rf ${WORKDIR}/framework/util-linux/${var} ${FRAMEWORK_TMP}
    else
        cp -rf ${WORKDIR}/framework/util-win/${var} ${FRAMEWORK_TMP}
    fi

    replacePath TARS_PATH ${TARS_PATH} ${FRAMEWORK_TMP}/${var}/conf
    replacePath TARS_PATH ${TARS_PATH} ${FRAMEWORK_TMP}/${var}/util
done

if [ $OS == 3 ]; then
    cp -rf ${WORKDIR}/libmysql.dll ${FRAMEWORK_TMP}/tarsnode/data/lib/
    cp -rf ${WORKDIR}/busybox.exe ${FRAMEWORK_TMP}/tarsnode/util/

    cp -rf ${WORKDIR}/framework/util-win/*.bat ${FRAMEWORK_TMP}

    replace TARS_PATH ${TARS_PATH} "${FRAMEWORK_TMP}/*.bat"
    replace WEB_PATH ${WEB_PATH} "${FRAMEWORK_TMP}/*.bat"
else
    cp -rf ${WORKDIR}/framework/util-linux/*.sh ${FRAMEWORK_TMP}

    replace TARS_PATH ${TARS_PATH} "${FRAMEWORK_TMP}/*.sh"
    replace WEB_PATH ${WEB_PATH} "${FRAMEWORK_TMP}/*.sh"
fi

LOG_INFO "copy framework to install path"

if [ $OS == 3 ]; then
    if [ -f ${TARS_PATH}/tarsnode/util/stop.bat ]; then
        ${TARS_PATH}/tarsnode/util/stop.bat 
    fi
fi

sleep 1
for var in $TARS;
do

    if [ $OS == 3 ]; then
        #stop first , then copy in windows
        if [ -d ${TARS_PATH}/${var} ]; then
            ${TARS_PATH}/${var}/util/stop.bat 
            sleep 1
        fi
    fi

    # sleep 1
    LOG_INFO "cp -rf ${FRAMEWORK_TMP}/${var} ${TARS_PATH}"
    cp -rf ${FRAMEWORK_TMP}/${var} ${TARS_PATH}

done

sleep 3

if [ $OS == 3 ]; then
    cp -rf ${FRAMEWORK_TMP}/*.bat ${TARS_PATH}
else
    cp -rf ${FRAMEWORK_TMP}/*.sh ${TARS_PATH}
fi

################################################################################

LOG_INFO "tar framework tgz"

cd ${FRAMEWORK_TMP}

mkdir -p ${WEB_PATH}/web/files/

for var in $TARSALL;
do
    LOG_INFO "tar czf ${var}.tgz ${var}"
    tar czf ${var}.tgz ${var}
    cp -rf ${var}.tgz ${WEB_PATH}/web/files/
    rm -rf ${var}.tgz
done

cd ${WORKDIR}

LOG_INFO "copy tools to web files"

if [ $OS == 3 ]; then
    cp ${WORKDIR}/tools/install-win.sh ${WEB_PATH}/web/files/
    cp ${WORKDIR}/busybox.exe ${WEB_PATH}/web/files/
else
    cp ${WORKDIR}/tools/install.sh ${WEB_PATH}/web/files/
fi

replace TARS_PATH ${TARS_PATH} ${WEB_PATH}/web/files/*.sh

################################################################################

function update_conf() 
{
    for file in `ls $1`;
    do
       ${MYSQL_TOOL} --host=${MYSQLIP} --user="${TARS_USER}" --pass="${TARS_PASS}" --port=${PORT} --config=$1/$file --tars-path=${TARS_PATH} --upload-path=${UPLOAD_PATH} --hostip=${HOSTIP}
    done
}

function update_util()
{
    for file in `ls $1`;
    do
        replace localip.tars.com $HOSTIP $1/${file}
    done
}

#start server
for var in $TARS;
do
    if [ ! -d ${TARS_PATH}/${var} ]; then
        LOG_ERROR "${TARS_PATH}/${var} not exist."
        exit 1 
    fi

    LOG_DEBUG "update config: ${TARS_PATH}/${var}/conf"
    update_conf "${TARS_PATH}/${var}/conf"

    LOG_DEBUG "update util: ${TARS_PATH}/${var}/util"
    update_util "${TARS_PATH}/${var}/util"

    LOG_DEBUG "remove config: rm -rf ${TARS_PATH}/tarsnode/data/tars.${var}/conf/tars.${var}.config.conf"

    rm -rf ${TARS_PATH}/tarsnode/data/tars.${var}/conf/tars.${var}.config.conf

    if [ $OS == 3 ]; then
        LOG_DEBUG ${TARS_PATH}/${var}/util/start.bat
        ${TARS_PATH}/${var}/util/start.bat > /dev/null
    else
        LOG_DEBUG ${TARS_PATH}/${var}/util/start.sh
        ${TARS_PATH}/${var}/util/start.sh > /dev/null
    fi

done

################################################################################

LOG_INFO "begin install web"
${WORKDIR}/web-install.sh ${MYSQLIP} ${TARS_PASS} ${HOSTIP} ${REBUILD} ${SLAVE} ${TARS_USER}  ${PORT} ${TARS_PATH} ${WEB_PATH}

rm -rf ${SQL_TMP}
rm -rf ${FRAMEWORK_TMP}

