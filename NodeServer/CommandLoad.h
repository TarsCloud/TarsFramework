﻿/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#ifndef __LOAD_COMMAND_H_
#define __LOAD_COMMAND_H_

#include "ServerCommand.h"

/**
 * 加载服务
 *
 */
class CommandLoad : public ServerCommand
{

public:
    CommandLoad(const ServerObjectPtr& pServerObjectPtr, const NodeInfo& tNodeInfo, bool succ);

    ExeStatus canExecute(string& sResult);

    int execute(string& sResult);
private:

    /**
    *更新server对应配置文件
    * @return int
    */
    int updateConfigFile(string& sResult);

    /**
    * 获取server配置文件
    * @return int
    */
    void getRemoteConf();

	/**
	 *
	 * @return
	 */
	string hostIp();

	/**
	 * 容器模式下获取主机的ip(如果ip是: 127.0.0.1)
	 * @return
	 */
	string replaceHostLocalIp(const TC_Endpoint &ep);

	/**
	 * 容器模式下获取主机的ip(如果ip是: 127.0.0.1)
	 * @param ip
	 * @return
	 */
	string replaceHostLocalIp(const string &ip);

	/**
	 * 获取locator
	 * @return
	 */
	string getLocator();

private:
    static std::mutex _mutex;
    static set<int> _allPorts;
    //    bool                _byNode;
    NodeInfo            _nodeInfo;
    ServerDescriptor    _desc;
    ServerObjectPtr     _serverObjectPtr;
    bool                _succ = true;
	StatExChangePtr     _statExChange;		//用于自动维护状态, 不要删除!
private:
    string _serverDir;               //服务数据目录
    string _confPath;                //服务配置文件目录
    string _confFile;                //服务配置文件目录 _strConfPath/conf
    string _exePath;                 //服务可执行程序目录。可个性设定,一般为_strServerDir/bin
    string _exeFile;                 //一般为_strExePath+_strServerName 可个性指定
    string _logPath;                 //服务日志目录
    string _libPath;                 //动态库目录 一般为_desc.basePath/lib
    string _serverType;              //服务类型
    string _startScript;               //启动脚本
    string _stopScript;                //停止脚本
    string _monitorScript;             //监控脚本
};


#endif
