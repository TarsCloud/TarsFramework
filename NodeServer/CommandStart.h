/**
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

#ifndef __START_COMMAND_H_
#define __START_COMMAND_H_

#include "ServerCommand.h"
#include "util/tc_timeprovider.h"

class CommandStart : public ServerCommand
{
public:
    enum
    {
        START_WAIT_INTERVAL  = 3,        /*服务启动等待时间 秒*/
        START_SLEEP_INTERVAL = 100000,   /*微妙*/
    };
public:
    CommandStart(const ServerObjectPtr& pServerObjectPtr, bool bByNode = false);

    ExeStatus canExecute(string& sResult);

    int execute(string& sResult);

private:
	/**
	 * tars服务启动时准备启动脚本
	 * @return
	 */
	void prepareScript();

	/**
	 * 启动普通的Tars服务
	 * @param scriptFile
	 * @param sResult
	 * @return
	 */
    bool startNormal(string& sResult);

	/**
	 * 启动以脚本形式启动的服务
	 * @param sResult
	 * @return
	 */
    bool startByScript(string& sResult);

	/**
	 * 在容器中启动!
	 * @return
	 */
	static bool startContainer(const ServerObjectPtr &serverObjectPtr, string &sResult);

	/**
	 * 普通tars服务的启动脚本
	 * @return
	 */
	static string getStartScript(const ServerObjectPtr &serverObjectPtr);

#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_IOS
	/**
	 *
	 * @param iPid
	 * @return
	 */
	int waitProcessDone(int64_t iPid);
#endif
	/**
	 * start成功
	 */
	static void startFinish(const ServerObjectPtr &serverObjectPtr, bool bSucc, int64_t startMs, const string &sResult);

	/**
	 * 搜索package.json文件
	 * @param package
	 * @return
	 */
	bool searchPackage(const string &searchDir, string &package);

	/**
	 * 搜索可执行程序
	 * @param searchDir
	 * @param server
	 * @return
	 */
	bool searchServer(const string &searchDir, string &server);

private:
    bool                _byNode;
    string              _exeFile;
    string              _logPath;
    ServerDescriptor    _desc;
    ServerObjectPtr     _serverObjectPtr;
};

//////////////////////////////////////////////////////////////
//
inline CommandStart::CommandStart(const ServerObjectPtr& pServerObjectPtr, bool bByNode)
: _byNode(bByNode)
, _serverObjectPtr(pServerObjectPtr)
{
    _exeFile   = _serverObjectPtr->getExeFile();
    _logPath   = _serverObjectPtr->getLogPath();
    _desc      = _serverObjectPtr->getServerDescriptor();
}
//////////////////////////////////////////////////////////////
#endif
