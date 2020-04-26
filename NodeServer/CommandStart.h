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
#include "NodeRollLogger.h"
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
    bool startNormal(string& sResult);

    bool startByScript(string& sResult);

    // bool startByScriptPHP(string& sResult);

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
