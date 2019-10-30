#!/usr/bin/python
# encoding: utf-8

import ConfigParser
import commands
import tarsLog
import socket
import fcntl
import struct
import os
import platform
from distutils.dir_util import copy_tree

log = tarsLog.getLogger()
platformStr = platform.platform()

def getIpAddress(ifname):
#    hostname = socket.gethostname()
#    return hostname
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', ifname[:15])
    )[20:24])

def getLocalIp():
    return getIpAddress(getCommProperties("network.ifname"))

def isUbuntu():
    return platformStr.lower().find("ubuntu") > -1

def isCentos():
    return platformStr.lower().find("centos") > -1

def isSuse():
    return platformStr.lower().find("suse") > -1

def getBaseDir():
    cwd = os.getcwd()
    #!!!!!! 本地虚拟机磁盘映射获取路径特殊，这里特殊处理， 其他地方不需要以下处理！！！
#    cwd = cwd.replace("/mnt/hgfs/", "/home/shevqko/")
    log.info("  os.getcwd() is {}".format(cwd))
    #path = os.path.abspath(os.path.join(os.getcwd(), ".."))
    path = os.path.abspath(os.path.join(cwd, ".."))
    log.info("  path is {}".format(path))
    return path

#def getCurrDir():
#    cwd = os.getcwd()
    #!!!!!! 本地虚拟机磁盘映射获取路径特殊，这里特殊处理， 其他地方不需要以下处理！！！
#    cwd = cwd.replace("/mnt/hgfs/", "/home/shevqko/") 
#    return cwd
    
def copytree(src, dst):
    copy_tree(src,dst)
    return

def doCmd(cmd):
    log.info(" execute cmd  start ,cmd : {}".format(cmd))
    result = dict()
    (status, output) = commands.getstatusoutput(cmd)
    result["status"] = status
    result["output"] = output
    log.info(" execute cmd  end ,cmd : {},status :{} , output: {}".format(cmd,status,output))
    if (0 != status):
        raise Exception("execute cmd  error ,cmd : {}, status is {} ,output is {}".format(cmd,status, output))
    return result

def doCmdNoException(cmd):
    log.info(" execute cmd  start ,cmd : {}".format(cmd))
    result = dict()
    (status, output) = commands.getstatusoutput(cmd)
    result["status"] = status
    result["output"] = output
    log.info(" execute cmd  end ,cmd : {},status :{} , output: {}".format(cmd,status,output))
    return result

def doCmdIgnoreException(cmd):
    log.info(" execute cmd  start ,cmd : {}".format(cmd))
    result = dict()
    (status, output) = commands.getstatusoutput(cmd)
    result["status"] = status
    result["output"] = output
    log.info(" execute cmd  end ,cmd : {},status :{} , output: {}".format(cmd, status, output))
    return result

def getCommProperties(paramsKey):
    baseDir = getBaseDir()
    cf = ConfigParser.ConfigParser()
    propertiesDir =baseDir+"/deploy/comm.properties"
#    print "propertiesDir:", propertiesDir
    cf.read(propertiesDir)
#    log.info(" commProperties is {} ".format(propertiesDir))
    cf.sections()
    
    value = os.getenv(paramsKey,'') 

    if value == '':
       value = cf.get('tarscomm', paramsKey)

    return value

def replaceConf(fileName,oldStr,newStr):
    if not os.path.isfile(fileName):
        print "{} is not a file ".format(fileName)
        return
    oldData =""
    with open(fileName, "r") as f:
        for line in f:
            if oldStr in line:
                line = line.replace(oldStr, newStr)
            oldData += line
    with open(fileName, "w") as f:
        f.write(oldData)
    return

def replaceConfDir(filePath,oldStr,newStr):
    if not os.path.isdir(filePath):
        print "{} is not a dir ".format(filePath)
        return
    for root, dirs, files in os.walk(filePath):
        for file in files:
            replaceConf(os.path.join(root,file),oldStr,newStr)
    return



if __name__ == '__main__':
    print(getIpAddress("eth0"))
    pass
