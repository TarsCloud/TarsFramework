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

#ifndef __STOP_COMMAND_H_
#define __STOP_COMMAND_H_

#include "ServerCommand.h"

class CommandStop : public ServerCommand
{
public:
    enum
    {
        STOP_WAIT_INTERVAL  = 2,        /*服务关闭等待时间 秒*/
        STOP_SLEEP_INTERVAL = 100000,   /*微妙*/
    };
public:
    CommandStop(const ServerObjectPtr& pServerObjectPtr, bool bUseAdmin = true, bool bByNode = false, bool bGenerateCore = false);

    ExeStatus canExecute(string& sResult);

    int execute(string& sResult);

private:
    ServerObjectPtr  _serverObjectPtr;
    bool             _useAdmin; //是否使用管理端口停服务
    bool             _byNode;
    bool             _generateCore;
    ServerDescriptor _desc;
};

//////////////////////////////////////////////////////////////
//
inline CommandStop::CommandStop(const ServerObjectPtr& pServerObjectPtr, bool bUseAdmin, bool bByNode, bool bGenerateCore)
: _serverObjectPtr(pServerObjectPtr)
, _useAdmin(bUseAdmin)
, _byNode(bByNode)
, _generateCore(bGenerateCore)
{
    _desc      = _serverObjectPtr->getServerDescriptor();
}

#endif
