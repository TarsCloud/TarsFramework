该工程是Tars框架的基础服务，是承载TARS各个语言的服务运行的基础框架。


目录名称 |功能
----------------------|----------------
framework             |框架部署需要的配置和sql
tools                 |生成tarsnode压缩包的脚本
centos-install.sh     |centos7系统, 一键安装tar环境
ubuntu-install.sh     |ubuntu系统, 一键安装tar环境
docker.sh             |将整个tars环境生成一个基于centos7的脚本
tars-install.sh       |安装tars的脚本, 被centos-install.sh or docker-init.sh调用
Dockerfile            |生成Docker


