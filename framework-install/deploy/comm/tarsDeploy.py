#!/usr/bin/python
# encoding: utf-8
import tarsLog
import time
import datetime
import shutil
import os
import sys
import stat
import MySQLdb
from tarsUtil import *

log = tarsLog.getLogger()
tarsDeploy = "/usr/local/app/tars"
tarsDeployFrameBasicServerList = ["tarsregistry", "tarsnode", "tarsAdminRegistry", "tarspatch","tarsconfig"]
tarsDeployFrameCommServerList = ["tarsnotify", "tarsstat", "tarsproperty", "tarsquerystat", "tarsqueryproperty", "tarslog"]
baseDir = getBaseDir()
localIp= getIpAddress(getCommProperties("network.ifname"))

def do():
    log.infoPrint("node ip:{}".format(localIp)) 
    log.infoPrint("init nodejs env start ...") 
    initNodeJsEnv()
    log.infoPrint("initDB success ") 
    log.infoPrint("initDB start ...")
    initDB()
    log.infoPrint("initDB success ")
    initNodeDB()
    log.infoPrint("initNodeDB success ")
    log.infoPrint("deploy framework Servers start ...")
    deployFrameServer()
    log.infoPrint("deploy framework Servers success ")
    copyTarsNodeTgz()
    log.infoPrint("copyTarsNodeTgz success") 
    log.infoPrint("deploy web start ... ")
    deployWeb()
    log.infoPrint("deploy web success")
    return

def getDBDir():
    dbDir = baseDir+"/framework/sql/"
    return dbDir

def deployFrameServer():
    for server in tarsDeployFrameBasicServerList:
        srcDir = "{}/framework/build/deploy/{}".format(baseDir,server)
        confDir = "{}/framework/deploy/{}".format(baseDir,server)
        dstDir = "/usr/local/app/tars/{}".format(server)
        log.infoPrint(" deploy {} start srcDir is {} , confDir is {} , dstDir is {}  ".format(server,srcDir,confDir,dstDir))
        copytree(srcDir,dstDir)
        copytree(confDir,dstDir)
        updateConf(server)
        #os.chmod(dstDir+"/util/start.sh",stat.S_IXGRP)
#        doCmd("chmod +x {}/util/*.sh".format(dstDir))
#        doCmd(dstDir+"/util/start.sh".format(server))
        log.infoPrint(" deploy {}  sucess".format(server))

    for server in tarsDeployFrameCommServerList:
        srcDir = "{}/framework/build/deploy/{}".format(baseDir,server)
        confDir = "{}/framework/deploy/{}".format(baseDir, server)
        dstDir = "/usr/local/app/tars/{}".format(server)
        dstBinDir = "/usr/local/app/tars/{}/bin/".format(server)
        log.infoPrint(" deploy {} start srcDir is {} , confDir is {} , dstDir is {}  ".format(server, srcDir, confDir, dstDir))
        if not os.path.exists(dstBinDir):
            os.makedirs(dstBinDir)
        shutil.copy(srcDir+"/"+server,dstBinDir+"/"+server)
        copytree(confDir,dstDir)
        updateConf(server)
        #os.chmod(dstDir+"/util/start.sh",stat.S_IXGRP)
#        doCmd("chmod +x {}/util/*.sh".format(dstDir))
#        doCmd(dstDir+"/util/start.sh".format(server))
    return

def updateConf(server):
    mysqlHost = getCommProperties("mysql.host")

    replaceConf("/usr/local/app/tars/{}/conf/tars.{}.config.conf".format(server,server),"localip.tars.com",localIp)
    replaceConf("/usr/local/app/tars/{}/conf/tars.{}.config.conf".format(server, server), "192.168.2.131", localIp)
    replaceConf("/usr/local/app/tars/{}/conf/tars.{}.config.conf".format(server, server), "192.168.2.129", localIp)
    replaceConf("/usr/local/app/tars/{}/conf/tars.{}.config.conf".format(server, server), "db.tars.com", mysqlHost)
    replaceConf("/usr/local/app/tars/{}/conf/tars.{}.config.conf".format(server, server), "registry.tars.com", localIp)
    if "tarsnode" == server:
        replaceConf("/usr/local/app/tars/{}/util/execute.sh".format(server, server), "registry.tars.com", localIp)
    return

def deployWeb():
    mysqlHost = getCommProperties("mysql.host")

    doCmd("cp -rf {}/web /usr/local/app/".format(baseDir))

    replaceConf("/usr/local/app/web/config/tars.conf", "registry.tars.com", localIp)
    replaceConf("/usr/local/app/web/config/webConf.js", "db.tars.com", mysqlHost)

    os.system("cd /usr/local/app/web; nohup npm run start")
#    doCmd("cd /usr/local/app/web; npm run prd")
    return

def copyTarsNodeTgz():
    doCmd("rm -rf {}/tarsnode_install/".format(baseDir))
    doCmd("mkdir {}/tarsnode_install/".format(baseDir))
    server="tarsnode"
    srcDir = "{}/framework/build/deploy/tarsnode".format(baseDir)
    confDir = "{}/framework/deploy/tarsnode".format(baseDir)
    dstDir = "{}/tarsnode_install/tarsnode".format(baseDir)
    copytree(srcDir,dstDir)
    copytree(confDir,dstDir)
    replaceConf("{}/tarsnode_install/tarsnode/util/execute.sh".format(baseDir), "registry.tars.com", localIp)
    replaceConf("{}/tarsnode_install/tarsnode/conf/tars.tarsnode.config.conf".format(baseDir), "registry.tars.com", localIp)
    #os.chmod(dstDir+"/util/start.sh",stat.S_IXGRP)
    doCmd("chmod +x {}/util/*.sh".format(dstDir))
    doCmd("cd {}; tar -zcvf tarsnode_install.tgz ./tarsnode_install/; cd -".format(baseDir))
    tgzDstDir = "/usr/local/app/web/files/install/"
    doCmd("mkdir -p {}; mv {}/tarsnode_install.tgz {}".format(tgzDstDir, baseDir, tgzDstDir))
    doCmd("cp -f {}/tools/install.sh {}".format(baseDir, tgzDstDir))
    replaceConf("{}/install.sh".format(tgzDstDir), "web.tars.com", localIp)
    doCmd("rm -rf {}/tarsnode_install/")
    log.infoPrint("copy tarsnode install files for wget success!")
    return

def initTemplate():
    dbDir=getDBDir()
    mysqlHost = getCommProperties("mysql.host")
    mysqlPort = getCommProperties("mysql.port")

    template_path = "{}/template".format(dbDir)
    dir_or_files = os.listdir(template_path)
    for file_path in dir_or_files:
        log.infoPrint("insert template {}/{}".format(template_path, file_path))
        
        parent_template = "tars.default"
        if file_path == "tars.springboot":
            parent_template = "tars.tarsjava.default" 

        with open( os.path.join(template_path, file_path), 'r') as f:
            doCmd("mysql -h{} -P{} -utars -ptars2015 -Ddb_tars -e 'replace into `t_profile_template` (`template_name`, `parents_name` , `profile`, `posttime`, `lastuser`) VALUES (\"{}\",\"{}\",\"{}\", now(),\"admin\");'".format(mysqlHost,mysqlPort, file_path, parent_template, f.read())); 

    return

def initDB():
    dbDir=getDBDir()
    mysqlHost = getCommProperties("mysql.host")
    mysqlPort = getCommProperties("mysql.port")
    mysqlRootPassWord = getCommProperties("mysql.root.password")
    replaceConfDir(dbDir, "192.168.2.131", localIp)
    replaceConfDir(dbDir, "db.tars.com", mysqlHost)

    while True:
      result = doCmdNoException("mysqladmin -h{} -P{} -uroot -p{} ping".format(mysqlHost,mysqlPort,mysqlRootPassWord))
      if result['output'].find("alive") > 0:
        break;
      log.info("check mysql is alive..")
      time.sleep(1)

    result = doCmd("mysql -h{} -P{} -uroot -p{} -e \"show databases like 'db_tars'\"".format(mysqlHost,mysqlPort,mysqlRootPassWord))
    if result['output'].find("db_tars") < 0:

      log.info(" dbDir is{} , mysqlHost is {} , mysqlPort is {} mysqlRootPassWord is {} ,localIp is {} ".format(dbDir,mysqlHost,mysqlPort,mysqlRootPassWord,localIp))
      doCmd("mysql -h{} -P{} -uroot -p{}  -e \"grant all on *.* to 'tars'@'%' identified by 'tars2015' with grant option;flush privileges;\"".format(mysqlHost, mysqlPort, mysqlRootPassWord))
      log.info(" the mysqlHost is {} , mysqlPort is {},  mysqlRootPassWord is {}".format(mysqlHost,mysqlPort,mysqlRootPassWord))

      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists db_tars;create database db_tars'".format(mysqlHost,mysqlPort))
      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists tars_stat;create database tars_stat'".format(mysqlHost,mysqlPort))
      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists tars_property;create database tars_property'".format(mysqlHost,mysqlPort))
      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists db_tars_web;create database db_tars_web'".format(mysqlHost,mysqlPort))
      doCmd("mysql -h{} -P{} -utars -ptars2015 db_tars < {}/db_tars.sql".format(mysqlHost,mysqlPort,dbDir))
      doCmd("mysql -h{} -P{} -utars -ptars2015 db_tars -e 'truncate t_profile_template; truncate t_server_conf;truncate t_adapter_conf;truncate t_node_info;truncate t_registry_info '".format(mysqlHost,mysqlPort))
      initTemplate()

    return

def initNodeDB():
    dbDir=getDBDir()
    mysqlHost = getCommProperties("mysql.host")
    mysqlPort = getCommProperties("mysql.port")

    replaceConfDir(dbDir, "192.168.2.131", localIp)
    replaceConfDir(dbDir, "db.tars.com", mysqlHost)

    doCmd("mysql -h{} -P{}  -utars -ptars2015 db_tars < {}/tars_servers.sql".format(mysqlHost,mysqlPort,dbDir))

def cleardb():
    mysqlHost = getCommProperties("mysql.host")
    mysqlPort = getCommProperties("mysql.port")

    result = doCmd("mysql -h{} -P{} -utars -ptars2015 -e \"show databases like 'db_tars'\"".format(mysqlHost,mysqlPort))
    if result['output'].find("db_tars") > 0: 
      doCmd("mkdir -p {}/backup-sql".format(baseDir))
      doCmd("mysqldump -h{} -P{} -utars -ptars2015 db_tars > {}/backup-sql/db_tars-{}.sql".format(mysqlHost,mysqlPort,baseDir,datetime.datetime.now().strftime('%Y%m%d%H%M%S')))
      doCmd("mysqldump -h{} -P{} -utars -ptars2015 db_tars_web > {}/backup-sql/db_tars_web-{}.sql".format(mysqlHost,mysqlPort,baseDir,datetime.datetime.now().strftime('%Y%m%d%H%M%S')))
      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists db_tars;'".format(mysqlHost,mysqlPort))
      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists tars_stat;'".format(mysqlHost,mysqlPort))
      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists tars_property;'".format(mysqlHost,mysqlPort))
      doCmd("mysql -h{} -P{} -utars -ptars2015 -e 'drop database if exists db_tars_web;'".format(mysqlHost,mysqlPort))

def initNodeJsEnv():
 
    result = doCmdIgnoreException("node --version")
    if result["status"] != 0:
        log.infoPrint("node is not valid!")
        exit(1)
    else:
        log.infoPrint("install node success! Version  is {}".format(result["output"]))

if __name__ == '__main__':
    do()
    pass
