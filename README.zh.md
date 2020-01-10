[Click to View English](README.md)

[查看Tars整体介绍文文档](https://tarscloud.github.io/TarsDocs)


该工程是Tars框架的基础服务，是承载TARS各个语言的服务运行的基础框架。


目录名称 |功能
----------------------|----------------
protocol              |定义各个基础服务定义的通信接口文件
RegistryServer        |名字服务路由
NodeServer            |管理服务
AdminRegistryServer   |与前台进行交互的接入管理服务
PatchServer           |发布服务
ConfigServer          |配置服务
LogServer             |日志服务
StatServer            |模调数据统计服务
PropertyServer        |属性统计服务
NotifyServer          |异常上报统计服务
deploy                |核心基础服务的模版配置和工具脚本
tarscpp               |Tars RPC框架C++语言的源代实现