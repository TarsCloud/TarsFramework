[点我查看中文版](README.zh.md)

This project is the basic service of the Tars framework and is the basic framework for the operation of the services that carry the TARS language.


目录名称 |功能
----------------------|----------------
framework             |框架部署需要的配置和sql
tools                 |生成tarsnode压缩包的脚本
centos-install.sh     |centos7系统, 一键安装tar环境(主控只在一台机器上)
docker.sh             |将整个tars环境生成一个基于centos7的脚本
tars-install.sh       |安装tars的脚本, 被centos-install.sh or docker-init.sh调用
Dockerfile            |生成Docker
*                     |其他部署文件, 各种mirror

# 目录
> * [安装前置条件](#chapter-0)
> * [centos原生环境安装](#chapter-1)
> * [ubuntu原生环境安装](#chapter-2)
> * [制作docker方式安装](#chapter-3)

# 1. <a id="chapter-0"></a>安装前置条件
- 完成TarsFramework的编译, 参见[Install.zh.md](Install.zh.md)。
- 搭建好mysql, 并确定root用户的用户密码
- 完成在编译后, 在build目录中: make install

# 2. <a id="chapter-1"></a>centos原生环境安装
- cd /usr/local/tars/cpp/framework-install
- sh centos-install.sh MYSQL_IP MYSQL_PORT MYSQL_USER MYSQL_PASSWORD HOSTIP

# 3. <a id="chapter-2"></a>ubuntu原生环境安装
- cd /usr/local/tars/cpp/framework-install
- sh ubuntu-install.sh MYSQL_IP MYSQL_PORT MYSQL_USER MYSQL_PASSWORD HOSTIP

# 4. <a id="chapter-3"></a>制作docker方式安装
- cd /usr/local/tars/cpp/framework-install
- sh docker.sh build v1
- docker run -e MYSQL_HOST=192.168.7.152 -e MYSQL_ROOT_PASSWORD=xxx -e MYSQL_PORT=3306 tars-docker:v1 sh /root/tars-install/docker-init.sh