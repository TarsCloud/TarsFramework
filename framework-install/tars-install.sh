#!/bin/sh


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

if (( $# < 5 ))
then
    echo "$0 MYSQL_IP MYSQL_PORT MYSQL_USER MYSQL_PASSWORD HOSTIP";
    echo "you should not call this script directly, you should call centos-install.sh or ubuntu-intall.sh, or in docker by call docker-init.sh"
    exit -1
fi

TARS=(tarsAdminRegistry tarsconfig  tarslog  tarsnode  tarsnotify  tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat  tarsregistry  tarsstat)

MYSQLIP=$1
PORT=$2
USER=$3
PASS=$4
HOSTIP=$5

TARS_PATH=/usr/local/app/tars
mkdir -p ${TARS_PATH}

workdir=$(cd $(dirname $0); pwd)

LOG_INFO "====================================================================";
LOG_INFO "===**********************tars-install*****************************===";
LOG_INFO "====================================================================";

#输出配置信息
LOG_DEBUG "===>print config info >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
LOG_DEBUG "MYSQLIP:       "$MYSQLIP 
LOG_DEBUG "USER:          "$USER
LOG_DEBUG "PASS:          "$PASS
LOG_DEBUG "PORT:          "$PORT
LOG_DEBUG "HOSTIP:        "$HOSTIP
LOG_DEBUG "workdir:       "$workdir
LOG_DEBUG "===<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< print config info finish.\n";

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

    LOG_ERROR "check mysql is not alive"

    sleep 3
done

function exec_mysql_script()
{
    mysql -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} -e "$1"

    if [ $? != 0 ]; then
        exit -1
    fi
}

function exec_mysql_sql()
{
    mysql -h${MYSQLIP} -u${USER} -p${PASS} -P${PORT} -D$1 < $2

    if [ $? != 0 ]; then
        exit -1
    fi
}

################################################################################
#check db_tars

RESULT=`exec_mysql_script "show databases like 'db_tars'"`

echo $RESULT | grep -q "db_tars"
if [ $? != 0 ]; then
    LOG_INFO "flush mysql privileges";

    exec_mysql_script "grant all on *.* to 'tars'@'%' identified by 'tars2015' with grant option;flush privileges;"

    LOG_INFO "modify ip in sqls:${workdir}/framework/sql";

    cp -rf ${workdir}/framework/sql ${workdir}/sql.tmp
    sed -i "s/192.168.2.131/$HOSTIP/g" `grep 192.168.2.131 -rl ${workdir}/sql.tmp/*`
    sed -i "s/db.tars.com/${MYSQLIP}/g" `grep db.tars.com -rl ${workdir}/sql.tmp/*`

    cd ${workdir}/sql.tmp

    LOG_INFO "create database (db_tars, tars_stat, tars_property, db_tars_web)";

    exec_mysql_script "create database db_tars"
    exec_mysql_script "create database tars_stat"
    exec_mysql_script "create database tars_property"
    exec_mysql_script "create database db_tars_web"
    exec_mysql_sql db_tars db_tars.sql

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

    LOG_INFO "create inner servers";

    exec_mysql_sql db_tars tars_servers.sql

    cd ${workdir}
    rm -rf sql.tmp
fi

################################################################################
#check framework

LOG_INFO "copy ${workdir}/framework/deploy/* to tars path:${TARS_PATH}";

cp -rf ${workdir}/framework/deploy/* ${TARS_PATH}

function update_conf() {

    LOG_INFO "update server ${TARS_PATH}/$1/conf/tars.$1.config.conf";

    sed -i "s/localip.tars.com/$HOSTIP/g" `grep localip.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
    sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`
    sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl ${TARS_PATH}/$1/conf/tars.$1.config.conf`

    if [ "tarsnode" == $1 ]; then
        sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl ${TARS_PATH}/$1/util/execute.sh`
    fi
}

#update server config
for var in ${TARS[@]};
do
    update_conf ${var}
done

for var in ${TARS[@]};
do
   if [ ! -d ${TARS_PATH}/${var} ]; then
       echo "${TARS_PATH}/${var} not exist."
       exit -1 
   fi
done

sh ${workdir}/tars.sh start

################################################################################
#deploy web

LOG_INFO "copy web to web path:/usr/local/app/";

cp -rf web /usr/local/app/

LOG_INFO "update web config";

sed -i "s/db.tars.com/$MYSQLIP/g" `grep db.tars.com -rl /usr/local/app/web/config/webConf.js`
sed -i "s/registry.tars.com/$HOSTIP/g" `grep registry.tars.com -rl /usr/local/app/web/config/tars.conf`

cd /usr/local/app/web; npm run prd

LOG_INFO "INSTALL TARS SUCC: http://$HOSTIP:3000/ to open the tars web."
LOG_INFO "If in Docker, please check you host ip and port."
LOG_INFO "You can start tars web manual: cd /usr/local/app/web; npm run prd"
LOG_INFO "==============================================================";
