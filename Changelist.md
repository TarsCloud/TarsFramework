# v3.0.5 20210328

### en
- Add: tarscpp use v3.0.6
- Add: tarsAdminRegistry add batchPatch to support dcache source compiler and publish
- Add: deploy script support database table upgrade
- Add: server template add volumes/ports in container mode.
- Add: tarsnode/tarsregistry template add container socket(/var/run/docker.sock) config
- Add: tarsnode/tarsregistry use tc_docker as docker api
- Fix: tarsAdminRegistry support dcache publish
- Fix: tarsnode start server return code bug
- Fix: tarsnode load config parseFile(profile) bug
- Fix: hostIp(host.docker.internal) in docker on mac platform
- Fix: tarsnode & tarsregistry load docker registry & image bug
- Fix: tarsnode load and create config bug
### cn
- 添加: tarscpp 使用到 v3.0.6
- 添加: tarsAdminRegistry 还原了 batchPatch 以支持dcache的编译以及发布
- 添加: 部署脚本支持了数据库表字段的升级
- 添加: 容器模式下, 服务模板支持了volumes/ports的配置(/tars/application/container/volumes or /tars/application/container/ports)
- 添加: tarsnode/tarsregistry使用 tc_docker 作为访问docker的api
- 添加: tarsnode/tarsregistry模板添加了容器的unit socket的配置(默认/var/run/docker.sock)
- 修复: tarsnode 启动服务时返回值的bug
- 修复: tarsnode 加载解析私有模板的bug
- 修复: 容器模式下, mac平台的hostIp使用host.docker.internal 
- 修复: tarsnode & tarsregistry 加载仓库和容器信息的bug
- 修复: tarsnode加载生成配置的bug(新增的,没有重新打tag)

# v3.0.4 20210225

### en

- Remove the support of rapidjson in logserver and optimize the analysis logic of call chain
- Add table t* docker* registry & t* base* Image to support containerized operation
- Support container operation, that is, release tgz package. The service can run in the specified container, which needs to cooperate with tarsweb v2 Version above 4.25 runs
- Tarsnode for the cpp/go version, if there is only one (xxxxServer) executable program within the directory, start the program (this ensures that the service exe that does not need to be published is the same as the configured service name)
- Fix the dead loop problem of tarsnotify when DB down is hung

### cn

- LogServer 中去掉 rapidjson 的支持, 优化调用链的分析逻辑
- 添加表 t_docker_registry & t_base_image, 以支持容器化运行
- 支持容器化运行, 即发布 tgz 包, 服务能运行在指定的容器中, 需要配合 tarsweb v2.4.25 以上版本运行
- tarsnode 对于 cpp/go 版本, 如果目录下只有一个带 Server 结尾的可执行程序, 则启动该程序(这样保证不需要发布的服务 exe 和配置的服务名相同)
- 修复 tarsnotify 在 db down 的挂的情况下, 死循环的问题

# v3.0.3 20211130

### en

- remove tarsnode useless log
- fix tarsnode use old config for tars framework server
- fix tarsnode, keep tars server local port not change
- update dockerfile node upgrade to 16.13.0

### cn

- 去掉 tarsnode 中不用的日志
- 修复 tarsnode 对 tars 框架服务, 使用旧配置文件
- 修复 tarsnode 保持 tars 服务的 local port 不要改变
- dockerfile 中 nodejs 版本升级到 v16.13.0

# v3.0.2 20211118

### en

- fix config path to read elegant_wait_second

### cn

- 修复读取无损发布配置的问题

# v3.0.1 20211020

### en

- update tarscpp to v3.0.1
- tarslog support trace
- fix windows compiler bug

### cn

- 升级 tarscpp to v3.0.1
- tarslog 支持调用链记录和统计
- 修复 windows 编译 bug

## v3.0.0 20210911

### en

- update tarscpp to v3.0.0

### cn

- 升级 tarscpp to v3.0.0

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

- 升级 tarscpp to v2.4.21, 必须依赖这个版本才能编译通过
- tarsAdminRegistry&tarregistry 支持了无损发布, 还需要等 web 支持
- tarspatch 增加了 upload 接口, 给将来部署到 k8s 中的 web 平台使用
- framework 支持安装到 127.0.0.1 上了, 方便调试
- tarsnode 随机创建 local 端口了
- 修复安装是 node 版本比较
- 修复 TARS_NOTIFY_XXXX 在 web 上不显示的问题
- tarsnode 模板增加 cmd_white_list_ip
- 制作 docker 的 Dockerfile 增加 psmisc,支持 killall 命令
- tarsregistry 启动自动升级 sql, 添加 flow_state 字段

## v2.4.14 2021.04.16

### en

- update tarscpp to v2.4.18, to avoid connection crash bug

### cn

- 升级 tarscpp 到 v2.4.18 版本, 以避免连接 crash 的问题

## v2.4.13 2021.04.02

### en

- Fix docker production, switch to ubuntu:20.04 , using buildx to make arm64/amd64 image

### cn

- 完善 docker 制作, 切换到 ubuntu:20.04 版本, 同时使用 buildx 同时制作 arm64/amd64 镜像

## v2.4.12 2021.03.29

### en

- perfect docker.sh Script and compilation are also execute in docker
- Making docker supporting arm64
- Support dynamic load balancing
- Modify the version of the service and the Framework version in SQL when the framework is installed
- tarsAdimnRegistry supports obtaining the interface of the Framework version for web use
- Fix nodetimeoutinterval minimum
- Try to fix node with certain probability on Windows platform\_ The ID cannot be obtained normally
- Fix the bug that tarsnode occasionally fails to update the service status of registry

### cn

- 完善 docker.sh 脚本, 编译也放在 docker 里面进行
- 支持 arm64 的 docker 制作
- 支持动态负载均衡
- framework 安装时修改 sql 中服务的版本和 framework 版本匹配
- tarsAdminRegistry 支持获取 framework 版本的接口, 以便 web 使用
- 修复 nodeTimeoutInterval 最小值的问题
- 尝试修复在 windows 平台上一定概率 NODE_ID 无法正常获取的问题
- 修复 tarsnode 更新 registry 的服务状态偶尔失败的 bug

## v2.4.11 2021.01.07

### en

- fix tarsnode, not registerNode when nodeName is empty
- fix tars-install.sh create database bug in k8s
- tars.default add activating-timeout
- fix tarsnode start scripts all success bug
- stat & property delele old history db data

### cn

- 修复 tarsnode, 当 nodeName 为空时, 不要调用 registerNode
- 修复 tars-install.sh 创建 database 的问题
- tars.default 添加 activating-timeout 参数
- 修复 tarsnode 启动脚本的时候, 总是判断成功的问题
- stat & property 定时删除老的历史数据, 缺省保留 31 天

## v2.4.10 2020.11.09

### en

- fix tarsregisty not return inactive ip of tarsAdminRegistry
- fix tarsnotify deadcycle bug
- fix compiler bug, in tarscpp v2.4.14 version
- add tarsnode start timeout config
- fix the error of tarspatch reading file in Windows
- Filter some files and not submit them to GIT

### cn

- 修改 tarsregisty 不返回 inactive 的 tarsAdminRegistry ip
- 修改 tarsnotify 可能的死循环问题
- 修改编译问题, 支持到 v2.4.14 版本的 tarscpp
- tarsnode 添加启动超时配置
- 修复 windows 下 tarspatch 读取文件错误
- 过滤某些文件, 不提交到 git 上

## v2.4.9 2020.10.11

### en

- Fix Windows compilation errors
- The NPM of nodejs in the installation script supports the selection of Tencent cloud / aliyun

### cn

- 修复 windows 编译错误
- 安装脚本中 nodejs 的 npm 支持 tencent cloud/aliyun 的选择

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

- 完善部署脚本, 支持 overwrite 模式, 覆盖配置和模板文件
- tarsAdminRegistry 支持保存历史发布记录(每个服务缺省 200 条), 可以在模板中修改配置
- tarsAdminRegistry 发布支持超时配置, 可以在模板中修改配置
- 部署时支持参数, 覆盖配置文件和模板
- 安装 framework slave 时, 如果 db_tars 还没有创建, 则退出

## v2.4.6 2020.09.02

### en

- fix node monitor, check keepalive
- fix java class path
- add tarsregistry log
- fix build docker bug, copy tars2case to docker

### cn

- 修改 tarnode 的监控逻辑, 增加 keepalive 的监控
- 修改 tarsnode java path
- 增加 tarsregistry 日志
- 修复 build docker 的 bug, copy tars2case 到容器中

## v2.4.5 2020.08.21

### en

- fix tarsnode write config file empty when disk is full
- AdminRegistry support patch time > 1 min
- fix windows install script
- Reduce tarsstat & tarsproperty shm memory consumption

### cn

- 继续修复 tarsnode 写配置文件可能为空的 bug(硬盘满的情况下)
- tarsAdminRegistry 支持发布超大文件(发布超过一分钟), 可配置超时时间
- 修复 windows install script
- 减小 tarsstat & tarsproperty 共享内存大小

## v2.4.4 2020.07.19

### en

- fix tarsnode save config bug(when disk is full)
- update deploy script support web(v2.4.7)
- fix tarstat & tarspropery only install in master framework
- tarsAdminRegistry support tarsgo gracefull deploy(web>=v2.4.7)

### cn

- 修改 tarsnode 当硬盘满或者掉电时, 写配置失败的 bug
- 更新部署脚本, 支持 web(v2.4.7)
- 修改安装脚本, tarstat & tarspropery 只安装在主节点上
- tarsAdminRegistry 支持 tarsgo 优雅发布(需要新版本 web >=v2.4.7 以上支持)

## v2.4.3 2020.07.03

### en:

- tarsregistry add monitor.sh
- tarsnode get real ip of registry & admin
- update framework not install tarslog
- update framework not replace template
- create db_base for gateway when install framework
- fix tarsnode server state error

### cn:

- tarsregistry 增加监控脚本, docker 内使用
- tarsnode 获取 registry & admin 的 ip, 存在代理的情况下
- 升级 framework 时, 不再安装 tarslog(因为 tarslog 通常会部署在其他机器)
- 升级 framework 时不在替换 template
- 安装 framework 时创建网关需要的 db_base
- 修复 tarsnode server state error

## v2.4.2 2020.06.10

### en:

- speed tars docker start
- clear t_server_notifys records
- support dcache deploy
- fix tarsnode start server timeout limit

### cn:

- 加速 docker 启动速度
- 清除 t_server_notifys 记录
- 支持 dcache 部署优化
- 修复 tarsnode 启动服务超时太短的 bug

## v2.4.1 2020.06.02

### en:

- fix registry db protect bug
- deploy support centos 6
- deploy tars, when mysql is not alive, Cycle detection
- registry default close day log

### cn:

- 修复 registry db protect 的功能
- 部署支持 centos 6
- 部署时, mysql 如果不连通, 循环检测
- registry 默认关闭按天日志

## v2.4.0 2020.05.06

### en:

- fix windows tarsnode start script bug
- fix tarsnode patch server, backup conf bug
- tarsnode onUpdateConfig add tars_ping
- fix udp server bug
- fix install framework, where OS has USER env, overload.profile,USER env invalid bug

### cn:

- 修复 windows tarsnode 启动脚本的 bug
- 修复 tarsnode 中发布应用, 备份的逻辑
- tarsnode onUpdateConfig add tars_ping
- fix udp server bug
- fix 安装过程中操作系统有 USER 的环境变量，当你重载.profile,USER 会失效的 bug

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
- 修改 stat & property, 增加共享内存 key 上报到 shm key & size to web
- 所有脚本全部修改为 bash 模式, 兼容各平台
- 修复 windows tars-start.bat & tars-stop.bat, web 不存在则不启动 web
- 调整 tars.cpp.default 模板, 增加协程和网络参数
- 调整环境变量写入文件到/etc/profile (mac/linux)
- 修复 CommandStart.h, php tars_start.sh 多了一个空格的 bug
- 修复 tarsnode 中启动 doMonScript 的 bug
- 修复启动 Java 服务 keep alive 中 pid 偶尔为 0 的问题
- 修复部署脚本, 删除不需要的 sql 权限
- 修复 windows 上 java 服务发布的 bug
- 修复 tarsnotify 中 eType default value 的 bug

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

- 框架全面支持 windows 安装
- 更新 tarsnode, 框架服务都 挂了, 启动 tarsnode 可以完成所有服务的拉起
- 修复 epoll 在 windows 下的 bug
- tlinux OS 安装支持
- 修复 web 创建 token
- 增加 t arsstat & tarsproperty 缺省的内存大小
- web 部分界面做了优化

## v2.1.2 2020.03.28

### en:

- Add MySQL tool to support deployment, no longer need to rely on MySQL client during installation
- Installation path can be specified during installation
- During installation, you can specify the user password of MySQL created by the framework
- Support the automatic deployment of windows
- depend on tarscpp v2.1.2

### cn:

- 增加 mysql-tool 支持部署, 不需要依赖 mysql 客户端
- 安装时可以指定安装路径
- 安装时可以指定框架新建的 mysql 的用户密码
- 支持 windows 的自动化部署
- 依赖 tarscpp v2.1.2

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

- 修复服务权重不生效的 bug
- 修改安装时 check.sh 没有替换 ip 的 bug
- 修复 adaminRegsitry 的部署 sql 的问题
- 修复 tars-tools.cmake 的问题
- fix 5.5 mysql 兼容性的问题
- 增加 querystat &queryproperty 返回错误信息
- 完善 tars 组件服务的 start & stop 脚本
- 修改环境从 1.9 版本升级上来的问题

## v2.1.0 2020.03.14

### en:

- fix tarsAdminRegistry bug
- remove rapidjson depends
- querystat & query property protocol change to tars
- tarsnode timing update template configuration, auto update registry locator
- Adjust template configuration, DB configuration is centralized to one
  file

### cn:

- 修改 tarsAdminRegistry bug
- 删除 rapidjson 依赖
- 修改 querystat & query property 协议到 tars
- tarsnode 定时更 tarsnode 配置到本地, 当主控切换地址后, tarsnode 可以跟随自动变动
- 调整模板继承方式, 将框架 db 配置集中到一个模板, 方便管理

## v2.0.1 2020.03.03

- support mac & linux
- depends tarscpp v2.0.0
