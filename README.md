[点我查看中文版](README.zh.md)

TarsFramework is the basic service for the whole Tars Project and is also the basic operational framework for all the languages in TARS Project.


Directory |Features
----------------------|----------------
protocol              |Define communication interface files for each underlying service definition
RegistryServer        |Name service routing
NodeServer            |Management service
AdminRegistryServer   |Access management service that interacts with the foreground
PatchServer           |Publishing service
ConfigServer          |Configuration service
LogServer             |Log service
StatServer            |Modular data statistics service
PropertyServer        |Attribute statistics service
NotifyServer          |Abnormal reporting service
deploy                |Template configuration and tool scripts for core infrastructure services
tarscpp               |The source implementation of the Tars RPC framework C++ language
