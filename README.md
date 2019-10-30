[点我查看中文版](README.zh.md)

This project is the basic service of the Tars framework and is the basic framework for the operation of the services that carry the TARS language.


Directory |Features
----------------------|----------------
framework             |框架部署需要的配置和sql
tools                 |生成tarsnode压缩包的脚本
centos-install.sh     |centos7系统, 一键安装tar环境(主控只在一台机器上)
docker.sh             |将整个tars环境生成一个基于centos7的脚本
tars-install.sh       |安装tars的脚本, 被centos-install.sh or docker-init.sh调用
Dockerfile            |生成Docker
*                     |其他部署文件, 各种mirror
