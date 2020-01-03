#!/bin/bash


#公共函数
function LOG_ERROR()
{
	if (( $# < 1 ))
	then
		echo -e "\033[33m usesage: LOG_ERROR msg \033[0m";
	fi
	
	local msg=$(date -d -0day +%Y-%m-%d" "%H:%M:%S);
	
	for p in $@
	do
		msg=${msg}" "${p};
	done
	
	echo -e "\033[31m $msg \033[0m";	
}

function LOG_WARNING()
{
	if (( $# < 1 ))
	then
		echo -e "\033[33m usesage: LOG_WARNING msg \033[0m";
	fi
	
	local msg=$(date -d -0day +%Y-%m-%d" "%H:%M:%S);
	
	for p in $@
	do
		msg=${msg}" "${p};
	done
	
	echo -e "\033[33m $msg \033[0m";	
}

function LOG_DEBUG()
{
	if (( $# < 1 ))
	then
		LOG_WARNING "Usage: LOG_DEBUG logmsg";
	fi
	
	local msg=$(date -d -0day +%Y-%m-%d" "%H:%M:%S);
	
	for p in $@
	do
		msg=${msg}" "${p};
	done
	
 	echo -e "\033[40;37m $msg \033[0m";	
}

function LOG_INFO()
{
	if (( $# < 1 ))
	then
		LOG_WARNING "Usage: LOG_INFO logmsg";
	fi
	
	local msg=$(date -d -0day +%Y-%m-%d" "%H:%M:%S);
	
	for p in $@
	do
		msg=${msg}" "${p};
	done
	
	echo -e "\033[32m $msg \033[0m"  	
}

if (( $# < 7 ))
then
    echo "$0 MYSQL_IP MYSQL_PORT MYSQL_USER MYSQL_PASSWORD HOSTIP REBUILD(false[default]/true) SLAVE(false[default]/true)";
    echo "you should not call this script directly, you should call centos-install.sh or ubuntu-intall.sh, or in docker by call docker-init.sh"
    exit 1
fi

MYSQLIP=$1
PORT=$2
USER=$3
PASS=$4
HOSTIP=$5
REBUILD=$6
SLAVE=$7

if [ "${SLAVE}" != "true" ]; then
    SLAVE="false"
fi

if [ "${SLAVE}" != "true" ]; then
    TARS=(tarsnotify tarsregistry tarsAdminRegistry tarspatch tarsconfig tarsnode tarslog  tarsproperty tarsqueryproperty tarsquerystat  tarsstat)
else
    TARS=(tarsnotify tarsregistry tarsconfig tarsnode tarsproperty tarsqueryproperty tarsquerystat  tarsstat)
fi

TARSALL=(tarsregistry tarsAdminRegistry tarsnode tarslog tarsconfig   tarsnotify  tarspatch  tarsproperty tarsqueryproperty tarsquerystat  tarsstat)
TARS_PATH=/usr/local/app/tars
mkdir -p ${TARS_PATH}

WORKDIR=$(cd $(dirname $0); pwd)

LOG_INFO "====================================================================";
LOG_INFO "===**********************tars-install*****************************===";
LOG_INFO "====================================================================";

#输出配置信息
LOG_DEBUG "===>print config info >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
LOG_DEBUG "PARAMS:        "$*
LOG_DEBUG "MYSQLIP:       "$MYSQLIP 
LOG_DEBUG "USER:          "$USER
LOG_DEBUG "PASS:          "$PASS
LOG_DEBUG "PORT:          "$PORT
LOG_DEBUG "HOSTIP:        "$HOSTIP
LOG_DEBUG "WORKDIR:       "$WORKDIR
LOG_DEBUG "SLAVE:         "${SLAVE}
LOG_DEBUG "REBUILD:       "${REBUILD}
LOG_DEBUG "===<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< print config info finish.\n";

################################################################################
#killall all tars-servers
for var in ${TARSALL[@]};
do
  killall -9 -q ${var}
done
for var in ${TARSALL[@]};
do
  killall -9 -q ${var}
done

################################################################################
#check port
PORTS=(18993 18793 18693 18193 18593 18493 18393 18293 12000 19385 17890 17891 3000 3001)
for P in ${PORTS[@]};
do
    RESULT=`netstat -lpn | grep ${HOSTIP}:${P} | grep tcp`
    if [ "$RESULT" != "" ]; then
        LOG_ERROR ${HOSTIP}:${P}" confict!!"
        exit 1
    fi
    RESULT=`netstat -lpn | grep 127.0.0.1:${P} | grep tcp`
    if [ "$RESULT" != "" ]; then
        LOG_ERROR 127.0.0.1:${P}" confict!!"
        exit 1
    fi
done


################################################################################
#check mysql
while [ 1 ]
do
    RESULT=`mysqladmin -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} ping`

    echo $RESULT | grep -q "alive"
    if [ $? == 0 ]; then
        LOG_INFO "mysql is alive"
        break
    fi

    LOG_ERROR "check mysql is not alive: mysqladmin -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} ping"

    sleep 3
done

function exec_mysql_script()
{
    # LOG_DEBUG "exec_mysql_script: $1"  
    mysql -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} --default-character-set=utf8 -e "$1"

    ret=$?
    LOG_DEBUG "exec_mysql_script $1, ret: $ret"  

    return $ret
}

function exec_mysql_sql()
{
    # LOG_DEBUG "exec_mysql_sql: $1 $2"  

    mysql -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} --default-character-set=utf8 -D$1 < $2

    ret=$?

    LOG_DEBUG "exec_mysql_sql $1 $2, ret: $ret"  

    return $ret
}

################################################################################
#check db_tars
cp -rf ${WORKDIR}/framework/sql ${WORKDIR}/sql.tmp

sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${WORKDIR}/sql.tmp/*`
sed -i "s/db.tars.com/${MYSQLIP}/g" `grep db.tars.com -rl ${WORKDIR}/sql.tmp/*`

if [ "$REBUILD" == "true" ]; then
    exec_mysql_script "drop database if exists db_tars"
    exec_mysql_script "drop database if exists tars_stat"
    exec_mysql_script "drop database if exists tars_property"
    exec_mysql_script "drop database if exists db_tars_web"    
    exec_mysql_script "drop database if exists db_user_system"    
fi

cd ${WORKDIR}/sql.tmp

MYSQL_VER=`mysql -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} -e "SELECT VERSION();"`
MYSQL_VER=`echo $MYSQL_VER | cut -d' ' -f2`

echo "mysql version is: $MYSQL_VER"

if [ `echo $MYSQL_VER|grep ^8.` ]; then
    exec_mysql_script "CREATE USER 'tars'@'%' IDENTIFIED WITH mysql_native_password BY 'tars2015';"
    exec_mysql_script "GRANT ALL ON *.* TO 'tars'@'%' WITH GRANT OPTION;"
    exec_mysql_script "CREATE USER 'tars'@'localhost' IDENTIFIED WITH mysql_native_password BY 'tars2015';"
    exec_mysql_script "GRANT ALL ON *.* TO 'tars'@'localhost' WITH GRANT OPTION;"
    exec_mysql_script "CREATE USER 'tars'@'${HOSTIP}' IDENTIFIED WITH mysql_native_password BY 'tars2015';"
    exec_mysql_script "GRANT ALL ON *.* TO 'tars'@'${HOSTIP}' WITH GRANT OPTION;"
fi

if [ `echo $MYSQL_VER|grep ^5.7` ]; then
    exec_mysql_script "set global validate_password_policy=LOW;"
fi

if [ `echo $MYSQL_VER|grep ^5.` ]; then
    exec_mysql_script "grant all on *.* to 'tars'@'%' identified by 'tars2015' with grant option;"
    if [ $? != 0 ]; then
        LOG_DEBUG "grant error, exit." 
        exit 1
    fi

    exec_mysql_script "grant all on *.* to 'tars'@'localhost' identified by 'tars2015' with grant option;"
    exec_mysql_script "grant all on *.* to 'tars'@'$HOSTIP' identified by 'tars2015' with grant option;"
    exec_mysql_script "flush privileges;"
fi

################################################################################
#check mysql
while [ 1 ]
do
    RESULT=`mysqladmin -h${MYSQLIP} -utars -ptars2015 -P${PORT} ping`

    echo $RESULT | grep -q "alive"
    if [ $? == 0 ]; then
        LOG_INFO "mysql auth succ"
        break
    fi

    LOG_ERROR "check mysql auth: mysqladmin -h${MYSQLIP} -utars -ptars2015 -P${PORT} ping"

    sleep 3
done

################################################################################
exec_mysql_script "use db_tars"
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

    LOG_INFO "create t_profile_template";

    sqlFile="tmp.sql"

    echo > $sqlFile
    echo "truncate t_profile_template;" >> $sqlFile

    function read_dir(){
        for template_name in $(ls template)
        do
            echo $template_name #在此处处理文件即可

            profile=$(cat template/${template_name} | sed "s/'/\\\'/g" ) 

            parent_template="tars.default"
            if [ "$template_name" == "tars.springboot" ]; then
                parent_template="tars.tarsjava.default"
            fi
            echo "replace into \`t_profile_template\` (\`template_name\`, \`parents_name\` , \`profile\`, \`posttime\`, \`lastuser\`) VALUES ('${template_name}','${parent_template}','${profile}', now(),'admin');" >> ${sqlFile};

        done
    }

    read_dir

    exec_mysql_sql db_tars $sqlFile;
fi

exec_mysql_script "use db_user_system"
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

rm -rf ${WORKDIR}/sql.tmp

################################################################################
#check framework

LOG_INFO "copy ${WORKDIR}/framework/servers/* to tars path:${TARS_PATH}";

cp -rf ${WORKDIR}/framework/servers/*.sh ${TARS_PATH}
for var in ${TARS[@]};
do
    cp -rf ${WORKDIR}/framework/servers/${var} ${TARS_PATH}
done

function update_conf() {

    LOG_INFO "update server config: [${TARS_PATH}/$1/conf/tars.$1.config.conf]";
    if [ "tarsnode" != "$1" ]; then
        sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
    else
        sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/registryAddress/tcp -h $HOSTIP -p 17890/g" `grep registryAddress -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/registryAddress/tcp -h $HOSTIP -p 17890/g" `grep registryAddress -rl ${TARS_PATH}/$1/util/execute.sh`
    fi
}

#update server config
for var in ${TARS[@]};
do
    update_conf ${var}
done

#start server
for var in ${TARS[@]};
do
   if [ ! -d ${TARS_PATH}/${var} ]; then
       LOG_ERROR "${TARS_PATH}/${var} not exist."
       exit 1 
   fi

    LOG_DEBUG ${TARS_PATH}/${var}/util/start.sh
    sh ${TARS_PATH}/${var}/util/start.sh > /dev/null

    usleep 100
done

################################################################################
#deploy & start web
if [ "$SLAVE" != "true" ]; then

    cd ${WORKDIR}
    LOG_INFO "copy web to web path:/usr/local/app/";

    rm -rf web/log
    cp -rf web /usr/local/app/
    # mkdir -p /usr/local/app/web/files/
    # LOG_INFO "copy *.tgz to web/files";

    # ls -R framework
    # cp framework/servers/*.tgz /usr/local/app/web/files/
    # cp tools/install.sh /usr/local/app/web/files/
    LOG_INFO "update web config";

    sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/config/webConf.js`
    sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl /usr/local/app/web/config/webConf.js`
    sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl /usr/local/app/web/config/tars.conf`

    sed -i "s/enableAuth: false/enableAuth: true/g" /usr/local/app/web/config/authConf.js
    sed -i "s/enableLogin: false/enableLogin: true/g" /usr/local/app/web/config/loginConf.js

    sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/demo/config/webConf.js`
    sed -i "s/enableLogin: false/enableLogin: true/g" /usr/local/app/web/demo/config/loginConf.js

    cd /usr/local/app/web; pm2 stop tars-node-web; npm run prd; 
    cd /usr/local/app/web/demo; pm2 stop tars-user-system; npm run prd

    LOG_INFO "INSTALL TARS SUCC: http://$HOSTIP:3000/ to open the tars web."
    LOG_INFO "If in Docker, please check you host ip and port."
    LOG_INFO "You can start tars web manual: cd /usr/local/app/web; npm run prd"
    LOG_INFO "You can install tarsnode to other machine in web(>=1.3.1)"
    LOG_INFO "==============================================================";
else
    LOG_INFO "Install slave($SLAVE) node success"
    LOG_INFO "==============================================================";
fi
