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
    exit -1
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
    TARS=(tarsAdminRegistry tarslog tarspatch tarsconfig tarsnode tarsnotify tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat)
else
    TARS=(tarsconfig tarsnode tarsnotify tarsproperty tarsqueryproperty tarsquerystat  tarsregistry tarsstat)
fi

TARSALL=(tarsAdminRegistry tarslog tarsconfig tarsnode  tarsnotify  tarspatch  tarsproperty tarsqueryproperty tarsquerystat tarsregistry tarsstat)
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
PORTS=(18993 18793 18693 18193 18593 18493 18393 18293 12000 19385 17890 17891)
for P in ${PORTS[@]};
do
    RESULT=`netstat -lpn | grep ${HOSTIP}:${P} | grep tcp`
    if [ "$RESULT" != "" ]; then
        LOG_ERROR ${HOSTIP}:${P}" confict!!"
        exit -1
    fi
    RESULT=`netstat -lpn | grep 127.0.0.1:${P} | grep tcp`
    if [ "$RESULT" != "" ]; then
        LOG_ERROR 127.0.0.1:${P}" confict!!"
        exit -1
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
    LOG_DEBUG "exec_mysql_script: $1"  
    mysql -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} --default-character-set=utf8 -e "$1"

    return $?
}

function exec_mysql_sql()
{
    LOG_DEBUG "exec_mysql_sql: $1 $2"  

    mysql -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} --default-character-set=utf8 -D$1 < $2

    return $?
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
fi

cd ${WORKDIR}/sql.tmp

exec_mysql_script "use db_tars"
if [ $? != 0 ]; then

    LOG_DEBUG "no db_tars exists, begin build db_tars..."  
    LOG_DEBUG "flush mysql privileges";

    exec_mysql_script "grant all on *.* to 'tars'@'%' identified by 'tars2015' with grant option;flush privileges;"

    LOG_DEBUG "modify ip in sqls:${WORKDIR}/framework/sql";


    LOG_DEBUG "create database (db_tars, tars_stat, tars_property, db_tars_web)";

    exec_mysql_script "create database db_tars"
    exec_mysql_script "create database tars_stat"
    exec_mysql_script "create database tars_property"
    exec_mysql_script "create database db_tars_web"
    exec_mysql_sql db_tars db_tars.sql

    LOG_DEBUG "create t_profile_template";

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

LOG_DEBUG "copy ${WORKDIR}/framework/servers/* to tars path:${TARS_PATH}";

cp -rf ${WORKDIR}/framework/servers/* ${TARS_PATH}

function update_conf() {

    LOG_DEBUG "update server config: [${TARS_PATH}/$1/conf/tars.$1.config.conf]";
    if [ "tarsnode" != "$1" ]; then
        sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
    else
        sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/registryAddress/tcp -h $HOSTIP -p 17890/g" `grep registryAddress -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
        sed -i "s/registryAddress/tcp -h $HOSTIP -p 17890/g" `grep registryAddress -rl ${TARS_PATH}/$1/util/execute.sh`
    fi

#    if [ "tarsnode" == $1 ]; then
#        sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl ${TARS_PATH}/$1/util/execute.sh`
#    fi
}

#update server config
for var in ${TARS[@]};
do
    update_conf ${var}
done

for var in ${TARS[@]};
do
   if [ ! -d ${TARS_PATH}/${var} ]; then
       LOG_ERROR "${TARS_PATH}/${var} not exist."
       exit -1 
   fi

    LOG_DEBUG ${TARS_PATH}/${var}/util/start.sh
    sh ${TARS_PATH}/${var}/util/start.sh > /dev/null
done

################################################################################
#deploy web
if [ "$SLAVE" != "true" ]; then

    LOG_DEBUG "create tarsnode.tgz";

    cd ${WORKDIR}/framework/servers;
    tar czf tarsnode.tgz tarsnode
    cd ${WORKDIR}
    LOG_DEBUG "copy web to web path:/usr/local/app/";

    cp -rf web /usr/local/app/
    LOG_DEBUG "copy tarsnode.tgz to web/files";
    mkdir -p /usr/local/app/web/files/
    cp framework/servers/tarsnode.tgz /usr/local/app/web/files/
    cp tools/install.sh /usr/local/app/web/files/
    LOG_DEBUG "update web config";

    sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/config/webConf.js`
    sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl /usr/local/app/web/config/tars.conf`

    cd /usr/local/app/web; pm2 stop tars-node-web; npm run prd 

    LOG_INFO "INSTALL TARS SUCC: http://$HOSTIP:3000/ to open the tars web."
    LOG_INFO "If in Docker, please check you host ip and port."
    LOG_INFO "You can start tars web manual: cd /usr/local/app/web; npm run prd"
    LOG_INFO "If You want to install tarsnode in other machine, do this: "
    LOG_INFO "wget http://$HOSTIP:3000/install.sh"
    LOG_INFO "sh install.sh"
    LOG_INFO "==============================================================";
else
    LOG_INFO "Install slave($SLAVE) node success"
    LOG_INFO "==============================================================";
fi
