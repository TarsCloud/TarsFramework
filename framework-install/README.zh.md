该工程是Tars框架的基础服务，是承载TARS各个语言的服务运行的基础框架。


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
### 启动docker:
- 启动docker: docker run -e MYSQL_HOST=xxx.xxx.xxx.xxx -e MYSQL_ROOT_PASSWORD=xxx -e MYSQL_PORT=3306 tars-docker:v1 sh /root/tars-install/docker-init.sh
- 启动docker, 和host主机同一个网络(注意端口不能冲突了), INET必须要是host机IP对应的网卡, 且重建DB: docker run -d --net=host -e MYSQL_HOST=xxx.xxx.xxx.xxx -e MYSQL_ROOT_PASSWORD=xxx -e MYSQL_PORT=3306 tars-docker:v1 -eINET=eth0 -eREBUILD=true sh /root/tars-install/docker-init.sh
### 多活的docker:
docker可以部署在不同host机子上多活, 但是一台为主, 为主的docker上有(tarsAdminRegistry, tarspatch, web)
- 主: docker run -d --net=host -e MYSQL_HOST=xxx.xxx.xxx.xxx -e MYSQL_ROOT_PASSWORD=xxx -e MYSQL_PORT=3306 tars-docker:v1 -eINET=eth0 -v/data/tars/patchs:/usr/local/app/patchs  sh -v/data/tars/web_log:/usr/local/tars/web/log -v/data/tars/app_log/:/usr/local/app/tars/app_log /root/tars-install/docker-init.sh
- 备(可以多台): docker run -d --net=host -e MYSQL_HOST=xxx.xxx.xxx.xxx -e MYSQL_ROOT_PASSWORD=xxx -e MYSQL_PORT=3306 tars-docker:v1 -eINET=eth0 -eSLAVE=true -v/data/tars/app_log/:/usr/local/app/tars/app_log sh /root/tars-install/docker-init.sh
其他节点上tarsnode的配置文件, 配置registry时可以指定多个 host

