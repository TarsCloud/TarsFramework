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
    echo "$0 MYSQL_IP MYSQL_PASSWORD  HOSTIP REBUILD(false[default]/true) SLAVE(false[default]/true) MYSQL_USER MYSQL_PORT TARS_PATH";
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

if [ "${SLAVE}" != "true" ]; then
    SLAVE="false"
fi

if [ "${SLAVE}" != "true" ]; then
    TARS="tarsregistry tarsAdminRegistry tarsconfig tarsnode tarsnotify tarsproperty tarsqueryproperty tarsquerystat tarsstat tarslog tarspatch"
else
    TARS="tarsregistry tarsconfig tarsnode tarsnotify tarsproperty tarsqueryproperty tarsquerystat tarsstat"
fi

if [ "${TARS_PATH}" == "" ]; then
    TARS_PATH=/usr/local/app/tars
fi

if [ $OS == 3 ]; then
    UPLOAD_PATH=$TARS_PATH
else
    UPLOAD_PATH=$TARS_PATH/..
fi

if [ $OS == 3 ]; then
    WEB_PATH=$TARS_PATH
else
    WEB_PATH=$TARS_PATH/..
fi

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
LOG_DEBUG "TARS_PATH:     "${TARS_PATH}
LOG_DEBUG "WEB_PATH:      "${WEB_PATH}
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
    PORTS="18993 18793 18693 18193 18593 18493 18393 18293 12000 19385 17890 17891 3000 3001"
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
    ${MYSQL_TOOL} --host=${MYSQLIP} --user="$1" --pass="$2" --port=${PORT} --check

    if [ $? == 0 ]; then
        LOG_INFO "mysql is alive"
        return
    fi

    LOG_ERROR "check mysql is not alive: ${MYSQL_TOOL} --host=${MYSQLIP} --user="$1" --pass="$2" --port=${PORT} --check"

    exit 1
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
    ${MYSQL_TOOL} --host=${MYSQLIP} --user=${TARS_USER} --pass=${TARS_PASS} --port=${PORT} --charset=utf8 --db=db_tars --parent=$1 --template=$2 --profile=$3

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
        # LOG_DEBUG "replace $1 $2 $file"
        ${MYSQL_TOOL} --src="${SRC}" --dst="${DST}" --replace=$file 
        # if [ $OS == 2 ]; then
        #     sed -i "" 's#${SRC}#${DST}#g' $file 
        # else
        #     sed -i 's#${SRC}#${DST}#g' $file
        # fi
    done

    # if [ $OS == 2 ]; then
    #     sed -i "" "s#${SRC}#${DST}#g" $SCAN_FILE 
    # else
    #     sed -i "s#${SRC}#${DST}#g" $SCAN_FILE
    # fi
}

function replacePath()
{
    SRC=$1
    DST=$2
    SCAN_PATH=$3

    # LOG_DEBUG "replacePath $1 $2 $SCAN_PATH"

    FILES=`grep "${SRC}" -rl $SCAN_PATH/*`

    # LOG_DEBUG "replacePath $1 $2 $SCAN_PATH"
    # LOG_DEBUG "path:$SCAN_PATH"
    # LOG_DEBUG "file:$FILES"

    if [ "$FILES" == "" ]; then
        return
    fi

    for file in $FILES;
    do
        # LOG_DEBUG "replacePath $1 $2 $var"
        ${MYSQL_TOOL} --src="${SRC}" --dst="${DST}" --replace=$file 
        # if [ $OS == 2 ]; then
        #     sed -i "" 's#${SRC}#${DST}#g' $file 
        # else
        #     sed -i 's#${SRC}#${DST}#g' $file
        # fi
    done
}

################################################################################
#replacePath sql/template
SQL_TMP=${WORKDIR}/sql.tmp

mkdir -p ${SQL_TMP}

cp -rf ${WORKDIR}/web/sql/*.sql ${WORKDIR}/framework/sql/
cp -rf ${WORKDIR}/web/demo/sql/*.sql ${WORKDIR}/framework/sql/

cp -rf ${WORKDIR}/framework/sql/* ${SQL_TMP}

replacePath localip.tars.com $HOSTIP ${SQL_TMP}
replacePath db.tars.com $MYSQLIP ${SQL_TMP}
replacePath 3306 $PORT ${SQL_TMP}
replacePath "dbuser=tars" "dbuser=${TARS_USER}" ${SQL_TMP}
replacePath "dbpass=tars2015" "dbpass=${TARS_PASS}" ${SQL_TMP}
replacePath TARS_PATH ${TARS_PATH} ${SQL_TMP}
replacePath UPLOAD_PATH ${UPLOAD_PATH} ${SQL_TMP}

# exit 0
# if [ $OS == 2 ]; then
#     #mac
#     sed -i "" "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${WORKDIR}/sql.tmp/*`
#     sed -i "" "s/db.tars.com/${MYSQLIP}/g" `grep db.tars.com -rl ${WORKDIR}/sql.tmp/*`
#     sed -i "" "s/3306/${PORT}/g" `grep 3306 -rl ${WORKDIR}/sql.tmp/*`
# else
#     sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${WORKDIR}/sql.tmp/*`
#     sed -i "s/db.tars.com/${MYSQLIP}/g" `grep db.tars.com -rl ${WORKDIR}/sql.tmp/*`
#     sed -i "s/3306/${PORT}/g" `grep 3306 -rl ${WORKDIR}/sql.tmp/*`
# fi

if [ "$REBUILD" == "true" ]; then
    exec_mysql_script "drop database if exists db_tars"
    exec_mysql_script "drop database if exists tars_stat"
    exec_mysql_script "drop database if exists tars_property"
    exec_mysql_script "drop database if exists db_tars_web"    
    exec_mysql_script "drop database if exists db_user_system"    
    exec_mysql_script "drop database if exists db_cache_web"
fi

cd ${SQL_TMP}

MYSQL_VER=`${MYSQL_TOOL} --host=${MYSQLIP} --user=${USER} --pass=${PASS} --port=${PORT} --version`

MYSQL_GRANT="SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, RELOAD, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE"

LOG_INFO "mysql version is: $MYSQL_VER"

if [ `echo $MYSQL_VER|grep ^8.` ]; then
    exec_mysql_script "CREATE USER '${TARS_USER}'@'%' IDENTIFIED WITH mysql_native_password BY '${TARS_PASS}';"
    exec_mysql_script "GRANT ${MYSQL_GRANT} ON *.* TO '${TARS_USER}'@'%' WITH GRANT OPTION;"
    exec_mysql_script "CREATE USER '${TARS_USER}'@'localhost' IDENTIFIED WITH mysql_native_password BY '${TARS_PASS}';"
    exec_mysql_script "GRANT ${MYSQL_GRANT} ON *.* TO '${TARS_USER}'@'localhost' WITH GRANT OPTION;"
    exec_mysql_script "CREATE USER '${TARS_USER}'@'${HOSTIP}' IDENTIFIED WITH mysql_native_password BY '${TARS_PASS}';"
    exec_mysql_script "GRANT ${MYSQL_GRANT} ON *.* TO '${TARS_USER}'@'${HOSTIP}' WITH GRANT OPTION;"
fi

# if [ `echo $MYSQL_VER|grep ^5.7` ]; then
#     exec_mysql_script "set global validate_password_policy=LOW;"
# fi

if [ `echo $MYSQL_VER|grep ^5.` ]; then
    exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'%' identified by '${TARS_PASS}' with grant option;"
    if [ $? != 0 ]; then
        LOG_DEBUG "grant error, exit." 
        exit 1
    fi

    exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'localhost' identified by '${TARS_PASS}' with grant option;"
    exec_mysql_script "grant ${MYSQL_GRANT} on *.* to '${TARS_USER}'@'$HOSTIP' identified by '${TARS_PASS}' with grant option;"
    exec_mysql_script "flush privileges;"
fi

################################################################################
#check mysql

check_mysql ${TARS_USER} ${TARS_PASS}

################################################################################
exec_mysql_has "db_tars"
if [ $? != 0 ]; then

    LOG_INFO "no db_tars exists, begin build db_tars..."  

    LOG_INFO "modify ip in sqls:${WORKDIR}/framework/sql";

    LOG_INFO "create database (db_tars, tars_stat, tars_property, db_tars_web)";

    exec_mysql_script "create database db_tars"
    exec_mysql_script "create database tars_stat"
    exec_mysql_script "create database tars_property"
    exec_mysql_script "create database db_tars_web"

    exec_mysql_sql db_tars db_tars.sql
    exec_mysql_sql db_tars_web db_tars_web.sql
fi

exec_mysql_has "db_cache_web"
if [ $? != 0 ]; then
    LOG_INFO "no db_cache_web exists, begin build db_cache_web..."
    exec_mysql_script "create database db_cache_web"

    exec_mysql_sql db_cache_web db_cache_web.sql
fi

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

update_template

exec_mysql_has "db_user_system"
if [ $? != 0 ]; then
    LOG_INFO "db_user_system not exists, begin build db_user_system..."  

    exec_mysql_script "create database db_user_system"

    exec_mysql_sql db_user_system db_user_system.sql
fi

if [ "$SLAVE" != "true" ]; then
    #current master server info
    LOG_INFO "create master servers";

    exec_mysql_sql db_tars tars_servers_master.sql
fi

#current server info
LOG_INFO "create inner servers";
exec_mysql_sql db_tars tars_servers.sql

#current node info
LOG_INFO "create node info";
exec_mysql_sql db_tars tars_node_init.sql

cd ${WORKDIR}

# rm -rf ${SQL_TMP}

################################################################################
# function update_path() {

#     cd $2 

#     LOG_INFO "update install path: [$2/conf/tars.$1.config.conf]";

#     if [ $OS == 2 ]; then
#         #mac
#         sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl conf/tars.$1.config.conf`
#         sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/execute.sh`
#         sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/start.sh`
#         sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/stop.sh`
#         if [ -f util/check.sh ]; then
#             sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/check.sh`
#         fi
#     elif [ $OS == 3 ]; then
#         #windows
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl conf/tars.$1.config.conf`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/execute.bat`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/start.bat`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/stop.bat`
#         if [ -f util/check.bat ]; then
#             sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/check.bat`
#         fi
#     else
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl conf/tars.$1.config.conf`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/execute.sh`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/start.sh`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/stop.sh`
#         if [ -f util/check.sh ]; then
#             sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl util/check.sh`
#         fi
#     fi

# }

# function update_script() {
#     if [ $OS == 2 ]; then
#         sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/check.sh`
#         sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/tars-start.sh`
#         sed -i "" "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/tars-stop.sh`
#     elif [ $OS == 3 ]; then
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/check.bat`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/tars-start.bat`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/tars-stop.bat`
#     else
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/check.sh`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/tars-start.sh`
#         sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${TARS_PATH}/tars-stop.sh`
#     fi
# }
################################################################################
#check framework

# LOG_INFO "copy ${WORKDIR}/framework/conf/ to tars path:${TARS_PATH}";

# if [ $OS != 3 ]; then
#     LOG_INFO "copy ${WORKDIR}/framework/util-linux/ to tars path:${TARS_PATH}";

#     cp -rf ${WORKDIR}/framework/util-linux/*.sh ${TARS_PATH}
#     chmod a+x ${TARS_PATH}/*.sh

# else
#     LOG_INFO "copy ${WORKDIR}/framework/util-win/ to tars path:${TARS_PATH}";

#     cp -rf ${WORKDIR}/framework/util-win/*.bat ${TARS_PATH}

#     # cp -rf ${WORKDIR}/busybox.exe ${TARS_PATH}

#     #for tarsnode use
#     cp -rf ${WORKDIR}/busybox.exe ${TARS_PATH}/tarsnode/util/
# fi

LOG_INFO "prepare framework-------------------------------------"

FRAMEWORK_TMP=${WORKDIR}/framework.tmp

# rm -rf ${WORKDIR}/framework-tmp
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

    replacePath TARS_PATH ${TARS_PATH} ${FRAMEWORK_TMP}/${var}/util
done

if [ $OS == 3 ]; then
    cp -rf ${WORKDIR}/libmysql.dll ${FRAMEWORK_TMP}/tarsnode/data/lib/
    cp -rf ${WORKDIR}/busybox.exe ${FRAMEWORK_TMP}/tarsnode/util/

    cp -rf ${WORKDIR}/framework/util-win/*.bat ${FRAMEWORK_TMP}

    replace TARS_PATH ${TARS_PATH} "${FRAMEWORK_TMP}/*.bat"
else
    cp -rf ${WORKDIR}/framework/util-linux/*.sh ${FRAMEWORK_TMP}

    replace TARS_PATH ${TARS_PATH} "${FRAMEWORK_TMP}/*.sh"
fi


LOG_INFO "copy framework to install path"

for var in $TARS;
do

    # update_path ${var} "${WORKDIR}/framework-tmp/${var}"

    # if [ $OS == 3 ]; then
    #     #stop first , then copy in windows
    #     if [ -d ${TARS_PATH}/${var} ]; then
    #         ${TARS_PATH}/${var}/util/stop.bat 
    #     fi
    # fi

    cp -rf ${FRAMEWORK_TMP}/${var} ${TARS_PATH}

    # replacePath TARS_PATH ${TARS_PATH} ${TARS_PATH}/${var}/util

done

if [ $OS == 3 ]; then
    cp -rf ${FRAMEWORK_TMP}/*.bat ${TARS_PATH}
else
    cp -rf  "${FRAMEWORK_TMP}/*.sh" ${TARS_PATH}
fi

# replace TARS_PATH "${TARS_PATH}/*.bat"

#exit 0

#update script
# update_script

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
    # sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${WEB_PATH}/web/files/install-win.sh`
else
    cp ${WORKDIR}/tools/install.sh ${WEB_PATH}/web/files/
    # sed -i "s#TARS_PATH#${TARS_PATH}#g" `grep TARS_PATH -rl ${WEB_PATH}/web/files/install.sh`
fi

replace TARS_PATH ${TARS_PATH} ${WEB_PATH}/web/files/*.sh

# rm -rf ${FRAMEWORK_TMP}

################################################################################

function update_conf() 
{
    UPDATE_PATH=$1

    replacePath localip.tars.com $HOSTIP ${UPDATE_PATH}
    replacePath db.tars.com $MYSQLIP ${UPDATE_PATH}
    replacePath registry.tars.com $HOSTIP ${UPDATE_PATH}
    replacePath 3306 $PORT ${UPDATE_PATH}
    replacePath "dbuser=tars" "dbuser=${TARS_USER}" ${UPDATE_PATH}
    replacePath "dbpass=tars2015" "dbpass=${TARS_PASS}" ${UPDATE_PATH}
    replacePath registryAddress "tcp -h $HOSTIP -p 17890" ${UPDATE_PATH}

    # cd $2 
    # LOG_INFO "update server config: [conf/tars.$1.config.conf]";

    # if [ $OS == 2 ]; then
    #     #mac
    #     sed -i "" "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "" "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "" "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "" "s/dbuser=tars/dbuser=${TARS_USER}/g" `grep dbuser=tars -rl conf/tars.$1.config.conf`
    #     sed -i "" "s/dbpass=tars2015/dbpass=${TARS_PASS}/g" `grep dbpass=tars2015 -rl conf/tars.$1.config.conf`
    #     sed -i "" "s/registryAddress/tcp -h $HOSTIP -p 17890/g" `grep registryAddress -rl conf/tars.$1.config.conf`
    #     sed -i "" "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl util/execute.sh`
    # elif [ $OS == 3 ]; then
    #     sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "s/3306/$PORT/g" `grep 3306 -rl conf/tars.$1.config.conf`
    #     sed -i "s/dbuser=tars/dbuser=${TARS_USER}/g" `grep dbuser=tars -rl conf/tars.$1.config.conf`
    #     sed -i "s/dbpass=tars2015/dbpass=${TARS_PASS}/g" `grep dbpass=tars2015 -rl conf/tars.$1.config.conf`        
    #     sed -i "s/registryAddress/tcp -h $HOSTIP -p 17890/g" `grep registryAddress -rl conf/tars.$1.config.conf`
    #     sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl util/execute.bat`
    # else
    #     sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl conf/tars.$1.config.conf`
    #     sed -i "s/3306/$PORT/g" `grep 3306 -rl conf/tars.$1.config.conf`
    #     sed -i "s/dbuser=tars/dbuser=${TARS_USER}/g" `grep dbuser=tars -rl conf/tars.$1.config.conf`
    #     sed -i "s/dbpass=tars2015/dbpass=${TARS_PASS}/g" `grep dbpass=tars2015 -rl conf/tars.$1.config.conf`   
    #     sed -i "s/registryAddress/tcp -h $HOSTIP -p 17890/g" `grep registryAddress -rl conf/tars.$1.config.conf`
    #     sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl util/execute.sh`
    # fi
}


# replacePath localip.tars.com $HOSTIP ${TARS_PATH}
# replacePath db.tars.com $HOSTIP ${TARS_PATH}
# replacePath registry.tars.com $HOSTIP ${TARS_PATH}
# replacePath 3306 $HOSTIP ${TARS_PATH}
# replacePath dbuser=tars $HOSTIP ${TARS_PATH}
# replacePath dbpass=tars2015 $HOSTIP ${TARS_PATH}
# replacePath registryAddress $HOSTIP ${TARS_PATH}

#start server
for var in $TARS;
do
    if [ ! -d ${TARS_PATH}/${var} ]; then
        LOG_ERROR "${TARS_PATH}/${var} not exist."
        exit 1 
    fi

    # LOG_DEBUG "update config: ${TARS_PATH}/${var}/conf"
    update_conf "${TARS_PATH}/${var}/conf"

    # LOG_DEBUG "---------------------------------------------"
    update_conf "${TARS_PATH}/${var}/util"

    # update_conf ${var} ${TARS_PATH}/${var}

    LOG_DEBUG "remove old config: rm -rf ${TARS_PATH}/tarsnode/data/tars.${var}/conf/tars.${var}.config.conf"

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

