

## v2.4.15 2021.08.19
### en
- update tarscpp to v2.4.21
- support gracefull patch
- tarspatch add upload interface
- framework support install in INET:127.0.0.1
- tarsnode create local port by rand
- fix node version compare
- fix TARS_NOTIFY_XXXX not show in web
- tarsnode template add cmd_white_list_ip
- deploy Dockerfile add psmisc
- update tarsregistry, support auto upgrade sql add flow_state
### cn
- 升级tarscpp to v2.4.21, 必须依赖这个版本才能编译通过
- tarsAdminRegistry&tarregistry支持了无损发布, 还需要等web支持
- tarspatch 增加了 upload 接口, 给将来部署到k8s中的web平台使用
- framework 支持安装到127.0.0.1上了, 方便调试
- tarsnode 随机创建local端口了
- 修复安装是node版本比较
- 修复 TARS_NOTIFY_XXXX 在web上不显示的问题
- tarsnode 模板增加 cmd_white_list_ip
- 制作docker的Dockerfile 增加 psmisc,支持killall命令
- tarsregistry 启动自动升级sql, 添加flow_state字段
## v2.4.14 2021.04.16
### en
- update tarscpp to v2.4.18, to avoid connection crash bug
### cn
- 升级tarscpp到v2.4.18版本, 以避免连接crash的问题
## v2.4.13 2021.04.02
### en
- Fix docker production, switch to ubuntu:20.04 , using buildx to make arm64/amd64 image
### cn
- 完善docker制作, 切换到ubuntu:20.04版本, 同时使用buildx同时制作arm64/amd64镜像

## v2.4.12 2021.03.29
### en
- perfect docker.sh Script and compilation are also execute in docker
- Making docker supporting arm64
- Support dynamic load balancing
- Modify the version of the service and the Framework version in SQL when the framework is installed
- tarsAdimnRegistry supports obtaining the interface of the Framework version for web use
- Fix nodetimeoutinterval minimum
- Try to fix node with certain probability on Windows platform_ The ID cannot be obtained normally
- Fix the bug that tarsnode occasionally fails to update the service status of registry
### cn
- 完善docker.sh脚本, 编译也放在docker里面进行
- 支持arm64的docker制作
- 支持动态负载均衡
- framework安装时修改sql中服务的版本和framework版本匹配
- tarsAdminRegistry支持获取framework版本的接口, 以便web使用
- 修复nodeTimeoutInterval最小值的问题
- 尝试修复在windows平台上一定概率NODE_ID无法正常获取的问题
- 修复tarsnode更新registry的服务状态偶尔失败的bug

## v2.4.11 2021.01.07
### en
- fix tarsnode, not registerNode when nodeName is empty
- fix tars-install.sh create database bug in k8s
- tars.default add activating-timeout
- fix tarsnode start scripts all success bug
- stat & property delele old history db data

### cn
- 修复tarsnode, 当nodeName为空时, 不要调用registerNode 
- 修复 tars-install.sh 创建 database 的问题
- tars.default 添加 activating-timeout 参数
- 修复tarsnode启动脚本的时候, 总是判断成功的问题
- stat & property 定时删除老的历史数据, 缺省保留31天 

## v2.4.10 2020.11.09
### en
- fix tarsregisty not return inactive ip of tarsAdminRegistry
- fix tarsnotify deadcycle bug
- fix compiler bug, in tarscpp v2.4.14 version
- add tarsnode start timeout config
- fix the error of tarspatch reading file in Windows
- Filter some files and not submit them to GIT

### cn
- 修改tarsregisty不返回inactive的tarsAdminRegistry ip
- 修改tarsnotify可能的死循环问题
- 修改编译问题, 支持到v2.4.14版本的tarscpp
- tarsnode添加启动超时配置
- 修复windows下tarspatch 读取文件错误
- 过滤某些文件, 不提交到git上 

## v2.4.9 2020.10.11
### en
- Fix Windows compilation errors 
- The NPM of nodejs in the installation script supports the selection of Tencent cloud / aliyun
### cn
- 修复windows编译错误
- 安装脚本中nodejs的npm支持tencent cloud/aliyun的选择

## v2.4.8 2020.09.23
### en
- tarsregistry sync internel code
### cn
- 主控同步内部代码

## v2.4.7 2020.09.16
### en
- fix the deployment script, support the overwrite mode, and cover the configuration and template files
- tarsAdminRegistry supports only historical publishing records (the latest 200 by default), and the configuration can be modified in the template
- tarsAdminRegistry supports patch timeout configuration, which can be modified in the template
- Support parameters during deployment, covering configuration files and templates
- When installing framework slave, if db_tars has not been created, exit
  
### cn
- 完善部署脚本, 支持overwrite模式, 覆盖配置和模板文件 
- tarsAdminRegistry支持保存历史发布记录(每个服务缺省200条), 可以在模板中修改配置
- tarsAdminRegistry发布支持超时配置, 可以在模板中修改配置
- 部署时支持参数, 覆盖配置文件和模板
- 安装framework slave时, 如果db_tars还没有创建, 则退出

## v2.4.6 2020.09.02
### en
- fix node monitor, check keepalive
- fix java class path
- add tarsregistry log
- fix build docker bug, copy tars2case to docker
### cn
- 修改tarnode的监控逻辑, 增加keepalive的监控
- 修改tarsnode java path
- 增加tarsregistry日志
- 修复build docker的bug, copy tars2case到容器中

## v2.4.5 2020.08.21
### en
- fix tarsnode write config file empty when disk is full 
- AdminRegistry support patch time > 1 min 
- fix windows install script
- Reduce tarsstat & tarsproperty shm memory consumption

### cn
- 继续修复tarsnode写配置文件可能为空的bug(硬盘满的情况下)
- tarsAdminRegistry支持发布超大文件(发布超过一分钟), 可配置超时时间
- 修复windows install script
- 减小tarsstat & tarsproperty 共享内存大小

## v2.4.4 2020.07.19
### en
- fix tarsnode save config bug(when disk is full)
- update deploy script support web(v2.4.7)
- fix tarstat & tarspropery only install in master framework
- tarsAdminRegistry support tarsgo gracefull deploy(web>=v2.4.7)
### cn
- 修改tarsnode当硬盘满或者掉电时, 写配置失败的bug
- 更新部署脚本, 支持web(v2.4.7)
- 修改安装脚本, tarstat & tarspropery只安装在主节点上
- tarsAdminRegistry 支持tarsgo优雅发布(需要新版本web >=v2.4.7以上支持)

## v2.4.3 2020.07.03
### en:
- tarsregistry add monitor.sh
- tarsnode get real ip of registry & admin
- update framework not install tarslog
- update framework not replace template
- create db_base for gateway when install framework
- fix tarsnode server state error

### cn:
- tarsregistry 增加监控脚本, docker内使用
- tarsnode 获取registry & admin的ip, 存在代理的情况下
- 升级framework时, 不再安装tarslog(因为tarslog通常会部署在其他机器)
- 升级framework时不在替换template
- 安装framework时创建网关需要的db_base
- 修复tarsnode server state error


## v2.4.2 2020.06.10
### en:
- speed tars docker start
- clear t_server_notifys records
- support dcache deploy 
- fix tarsnode start server timeout limit
### cn:
- 加速docker启动速度
- 清除t_server_notifys记录
- 支持dcache部署优化
- 修复tarsnode启动服务超时太短的bug


## v2.4.1 2020.06.02
### en:
- fix registry db protect bug
- deploy support centos 6
- deploy tars, when mysql is not alive, Cycle detection
- registry default close day log
### cn:
- 修复registry db protect的功能
- 部署支持centos 6
- 部署时, mysql如果不连通, 循环检测
- registry默认关闭按天日志

## v2.4.0 2020.05.06
### en:
- fix windows tarsnode start script bug
- fix tarsnode patch server, backup conf bug
- tarsnode onUpdateConfig add tars_ping
- fix udp server bug
- fix install framework, where OS has USER env, overload.profile,USER env invalid bug
### cn:
- 修复windows tarsnode启动脚本的bug
- 修复tarsnode中发布应用, 备份的逻辑
- tarsnode onUpdateConfig add tars_ping
- fix udp server bug
- fix 安装过程中操作系统有USER的环境变量，当你重载.profile,USER会失效的bug

## v2.3.0 2020.04.24
### en:
- fix deploy script, Deployment script, the preparation work of actual installation is centralized for subsequent maintenance
- fix stat & property, report in shm key & size to Web
- All scripts are modified to bash mode, compatible with all platforms
- fix windows tars-start.bat & tars-stop.bat, web not exists then not start web
- fix tars.cpp.default template, add co & netthread paramter
- Adjust environment variable write file to /etc/profile (mac/linux)
- fix CommandStart.h, php tars_start.sh, remove space bug
- fix tarsnode, doMonScript bug
- fix start Java server bug, keep alive pid == 0
- Repair the deployment script and delete the unnecessary SQL permission
- fix windows start java bug
- fix tarsnotify eType default value bug

### cn:
- 完善部署脚本, 实际安装的准备工作集中到一起, 便于后续维护
- 修改stat & property, 增加共享内存key上报到shm key & size to web
- 所有脚本全部修改为bash模式, 兼容各平台
- 修复windows tars-start.bat & tars-stop.bat, web 不存在则不启动web
- 调整tars.cpp.default模板, 增加协程和网络参数
- 调整环境变量写入文件到/etc/profile (mac/linux)
- 修复CommandStart.h, php tars_start.sh多了一个空格的bug
- 修复tarsnode中启动doMonScript的bug
- 修复启动Java服务keep alive中pid偶尔为0的问题
- 修复部署脚本, 删除不需要的sql权限
- 修复windows上java服务发布的bug
- 修复tarsnotify中eType default value的bug

## v2.2.0 2020.03.31
### en
- framework support deploy on windows
- update tarsnode, tarsnode can start all framework server
- fix epoll bug in windows
- tlinux OS install support
- fix create token bug in web
- add tarsstat & tarsproperty default memory
- Web improvement experience
### cn
- 框架全面支持windows安装
- 更新tarsnode, 框架服务都 挂了, 启动tarsnode可以完成所有服务的拉起
- 修复epoll在windows下的bug
- tlinux OS 安装支持
- 修复web 创建token
- 增加t arsstat & tarsproperty 缺省的内存大小
- web部分界面做了优化

## v2.1.2 2020.03.28
### en:
- Add MySQL tool to support deployment, no longer need to rely on MySQL client during installation
- Installation path can be specified during installation
- During installation, you can specify the user password of MySQL created by the framework
- Support the automatic deployment of windows
- depend on tarscpp v2.1.2
### cn:
- 增加mysql-tool支持部署, 不需要依赖mysql客户端
- 安装时可以指定安装路径
- 安装时可以指定框架新建的mysql的用户密码
- 支持windows的自动化部署
- 依赖tarscpp v2.1.2

## v2.1.1 2020.03.23
## en:
- check.sh does not replace IP when modifying the framework installation
- fix deploy adminRegsitry bug in sql
- fix tars-tools.cmake
- fix problems of MySQL(5.5) compatibility
- add querystat &queryproperty error return msg
- fix start & stop script of tars-server
- fix update framework bug
### cn:
- 修复服务权重不生效的bug
- 修改安装时check.sh没有替换ip的bug
- 修复adaminRegsitry的部署sql的问题
- 修复tars-tools.cmake的问题
- fix 5.5 mysql兼容性的问题
- 增加querystat &queryproperty返回错误信息
- 完善tars组件服务的start & stop脚本
- 修改环境从1.9版本升级上来的问题

## v2.1.0 2020.03.14
### en:
- fix tarsAdminRegistry bug
- remove rapidjson depends
- querystat & query property protocol change to tars
- tarsnode timing update template configuration, auto update registry locator
- Adjust template configuration, DB configuration is centralized to one
file
### cn:
- 修改tarsAdminRegistry bug
- 删除rapidjson依赖
- 修改querystat & query property 协议到tars
- tarsnode定时更tarsnode配置到本地, 当主控切换地址后, tarsnode可以跟随自动变动
- 调整模板继承方式, 将框架db配置集中到一个模板, 方便管理

## v2.0.1 2020.03.03
- support mac & linux
- depends tarscpp v2.0.0
