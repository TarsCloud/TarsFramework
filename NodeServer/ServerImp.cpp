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

#include "ServerImp.h"
#include "util.h"
#include "NodeRollLogger.h"
#include "NodeServer.h"

int ServerImp::keepAlive( const tars::ServerInfo& serverInfo, tars::TarsCurrentPtr current )
{
	string serverId = serverInfo.application + "." + serverInfo.serverName;

    try
    {   
        string sApp     = serverInfo.application;
        string sName    = serverInfo.serverName;

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( sApp, sName );
        if(pServerObjectPtr)
        {
            // NODE_LOG(serverId)->debug() << "ServerImp::keepAlive server " << serverId << " keep alive, pid:" << serverInfo.pid << endl;

            pServerObjectPtr->keepAlive(serverInfo.pid,serverInfo.adapter);

            return 0;
        }

	    NODE_LOG(serverId)->debug() << "ServerImp::keepAlive server " << serverId << " is not exist, pid:" << serverInfo.pid << endl;
    }
    catch ( exception& e )
    {
	    NODE_LOG(serverId)->error() << "ServerImp::keepAlive catch exception :" << e.what() << endl;
    }
    catch ( ... )
    {
	    NODE_LOG(serverId)->error() << "ServerImp::keepAlive unkown exception catched" << endl;
    }

    return -1;
}

int ServerImp::keepActiving( const tars::ServerInfo& serverInfo, tars::TarsCurrentPtr current )
{
	string serverId = serverInfo.application + "." + serverInfo.serverName;

	try
    {
        string sApp     = serverInfo.application;
        string sName    = serverInfo.serverName;

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( sApp, sName );
        if ( pServerObjectPtr )
        {
	        NODE_LOG(serverId)->debug()<< "server " << serverId << " keep activing"<< endl;
            pServerObjectPtr->keepActiving(serverInfo.pid);
            return 0;
        }
	    NODE_LOG(serverId)->error() << FILE_FUN<< "server " << serverId << " is not exist"<< endl;
    }
    catch ( exception& e )
    {
	    NODE_LOG(serverId)->error() << FILE_FUN << "catch exception :" << e.what() << endl;
    }
    catch ( ... )
    {
	    NODE_LOG(serverId)->error() << FILE_FUN << "unkown exception catched" << endl;
    }
    return -1;
}

int ServerImp::reportVersion( const string &app,const string &serverName,const string &version,tars::TarsCurrentPtr current)
{
	string serverId = app + "." + serverName;

	try
    {
        NODE_LOG(serverId)->debug() << "ServerImp::reportVersion|server|" << serverId << "|version|" << version<< endl;

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( app, serverName );
        if(pServerObjectPtr)
        {
            pServerObjectPtr->setVersion(version);

            return 0;
        }

	    NODE_LOG(serverId)->debug() << "ServerImp::reportVersion server " << serverId << " is not exist"<< endl;
    }
    catch ( exception& e )
    {
	    NODE_LOG(serverId)->error() << "ServerImp::reportVersion catch exception :" << e.what() << endl;
    }
    catch ( ... )
    {
	    NODE_LOG(serverId)->error() << "ServerImp::reportVersion unkown exception catched" << endl;
    }

    return -1;
}

unsigned int ServerImp::getLatestKeepAliveTime(tars::CurrentPtr current)
{
    try
    {
        if (g_app.getKeepAliveThread())
        {
            return g_app.getKeepAliveThread()->getLatestKeepAliveTime();
        }
    }
    catch ( exception& e )
    {
        TLOGERROR("ServerImp::getLatestKeepAliveTime catch exception :" << e.what() << endl);
    }
    catch ( ... )
    {
        TLOGERROR("ServerImp::getLatestKeepAliveTime unkown exception catched" << endl);
    }
    return 0;
}

