
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
