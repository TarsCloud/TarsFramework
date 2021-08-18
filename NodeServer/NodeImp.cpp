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
#include "util/tc_platform.h"
#include "util/tc_clientsocket.h"
#include "BatchPatchThread.h"
#include "NodeImp.h"
#include "RegistryProxy.h"
#include "NodeServer.h"
#include "CommandStart.h"
#include "CommandNotify.h"
#include "CommandStop.h"
#include "CommandDestroy.h"
#include "CommandAddFile.h"
#include "CommandPatch.h"
#include "NodeRollLogger.h"
#include "AdminReg.h"
#include "util/tc_timeprovider.h"

extern BatchPatch * g_BatchPatchThread;

void NodeImp::initialize()
{
    try
    {
        TLOGDEBUG("initialize NodeImp" << endl);

        _nodeInfo      = _platformInfo.getNodeInfo();
        _downloadPath  = _platformInfo.getDownLoadDir();
    }
    catch ( exception& e )
    {
        TLOGERROR(FILE_FUN << e.what() << endl);
        exit( 0 );
    }
}

int NodeImp::destroyServer( const string& application, const string& serverName,string &result, TarsCurrentPtr current )
{
    int64_t startMs = TC_TimeProvider::getInstance()->getNowMs();
	string serverId = application + "." + serverName;

    int iRet = EM_TARS_SUCCESS;
    if ( g_app.isValid(current->getIp()) == false )
    {
        result += " erro:ip "+ current->getIp()+" is invalid";
        iRet    = EM_TARS_INVALID_IP_ERR;
        NODE_LOG(serverId)->error() << FILE_FUN << "access_log|" << iRet << "|" << (TC_TimeProvider::getInstance()->getNowMs() - startMs)
                << "|" << serverId << "|" << result << endl;
        return iRet;
    }

    result = " [" + serverId + "] ";

    NODE_LOG(serverId)->debug() << result << endl;
    ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
    if ( pServerObjectPtr )
    {
        string s;
        CommandDestroy command(pServerObjectPtr);
        iRet = command.doProcess(current, s, false);
        if ( iRet == 0 && ServerFactory::getInstance()->eraseServer(application, serverName) == 0)
        {
            pServerObjectPtr = NULL;
            result = result+"succ:" + s;
        }
        else
        {
            result = FILE_FUN_STR+"error:"+s;
        }
    }
    else
    {
        result += FILE_FUN_STR+"server is not exist";
        iRet = -2;
    }

    NODE_LOG(serverId)->error()  << FILE_FUN << "access_log|" << iRet << "|" << (TC_TimeProvider::getInstance()->getNowMs() - startMs)
                << "|" << application << "|" << serverName << "|" << result << endl;
    return iRet;
}

int NodeImp::patchPro(const tars::PatchRequest & req, string & result, TarsCurrentPtr current)
{
	string serverId = req.appname + "." + req.servername;

    NODE_LOG(serverId)->debug() << FILE_FUN
                 << serverId + "_" + req.nodename << "|"
                 << req.groupname   << "|"
                 << req.version     << "|"
                 << req.user        << "|"
                 << req.servertype  << "|"
                 << req.patchobj    << "|"
                 << req.md5            << "|"
                 << req.ostype      << endl;

    if ( g_app.isValid(current->getIp()) == false )
    {
        result += FILE_FUN_STR+ " error:ip "+ current->getIp()+" is invalid";
        NODE_LOG(serverId)->error()<<FILE_FUN << result << endl;
        return EM_TARS_INVALID_IP_ERR;
    }

    try
    {
         //加载服务信息
        string sError("");
        ServerObjectPtr server = ServerFactory::getInstance()->loadServer(req.appname, req.servername, true, sError);
        if (!server)
        {
            result =FILE_FUN_STR+ "load " + serverId + "|" + sError;
            NODE_LOG(serverId)->error() << FILE_FUN << serverId + "_" + req.nodename << "|" << result << endl;
            if(sError.find("server state") != string::npos)
                return EM_TARS_SERVICE_STATE_ERR;
            else
                return EM_TARS_LOAD_SERVICE_DESC_ERR;
        }

        {
            TC_ThreadRecLock::Lock lock(*server);
            ServerObject::InternalServerState  eState = server->getInternalState();

            //去掉启动中不能发布的限制
            if (server->toStringState(eState).find("ing") != string::npos && eState != ServerObject::Activating)
            {
                result = FILE_FUN_STR+"cannot patch the server ,the server state is " + server->toStringState(eState);
                NODE_LOG(serverId)->error() <<FILE_FUN<< serverId << "|" << result << endl;
                return EM_TARS_SERVICE_STATE_ERR;
            }

            if(req.md5 == "")
            {
                result = FILE_FUN_STR+"parameter error, md5 not setted.";
                NODE_LOG(serverId)->error() <<FILE_FUN<< serverId << "|" << result << endl;
                return EM_TARS_PARAMETER_ERR;
            }

            if(CommandPatch::checkOsType())
            {
                string osType = CommandPatch::getOsType();
                if(req.ostype != "" && req.ostype != osType)
                {
                    result = FILE_FUN_STR+"parameter error, req.ostype=" + req.ostype + ", local osType:" + osType;
                    NODE_LOG(serverId)->error() <<FILE_FUN<< serverId << "|" << result << endl;
                    return EM_TARS_PARAMETER_ERR;
                }
            }

            //保存patching前的状态，patch完成后恢复
            NODE_LOG(serverId)->debug() <<FILE_FUN<<serverId + "_" + req.nodename << "|saved state:" << server->toStringState(eState) << endl;
            server->setLastState(eState);
            server->setState(ServerObject::BatchPatching);
            //将百分比初始化为0，防止上次patch结果影响本次patch进度
            server->setPatchPercent(0);

            NODE_LOG(serverId)->debug()<<FILE_FUN << serverId + "_" + req.nodename << "|" << req.groupname   << "|preset success" << endl;
        }

        g_BatchPatchThread->push_back(req,server);
    }
    catch (std::exception & ex)
    {
        NODE_LOG(serverId)->error() << FILE_FUN << serverId + "_" + req.nodename << "|Exception:" << ex.what() << endl;

        result = FILE_FUN_STR+ex.what();

        if(result.find("reduplicate") != string::npos)
        {
            return EM_TARS_REQ_ALREADY_ERR;
        }

        return EM_TARS_UNKNOWN_ERR;
    }
    catch (...)
    {
        NODE_LOG(serverId)->error() << FILE_FUN << serverId + "_" + req.nodename << "|Unknown Exception" << endl;
        result = FILE_FUN_STR+"Unknown Exception";
        return EM_TARS_UNKNOWN_ERR;
    }

    NODE_LOG(serverId)->debug() <<FILE_FUN
             << serverId + "_" + req.nodename << "|"
             << req.groupname   << "|"
             << req.version     << "|"
             << req.user        << "|"
             << req.servertype  << "|return success"<< endl;

    return EM_TARS_SUCCESS;
}

int NodeImp::addFile(const string &application,const string &serverName,const string &file, string &result, TarsCurrentPtr current)
{
	string serverId = application + "." + serverName;

    result = string(__FUNCTION__)+" server ["+serverId+"] file:"+file;

    NODE_LOG(serverId) ->debug() << FILE_FUN << result  << endl;

    ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
    if ( pServerObjectPtr )
    {
        string s;
        CommandAddFile command(pServerObjectPtr,file);
        int iRet = command.doProcess(current,s);
        if ( iRet == 0)
        {
            result = result+"succ:" + s;
        }
        else
        {
            result = FILE_FUN_STR+"error:"+s;
        }
        return iRet;
    }

    result += FILE_FUN_STR+"server is  not exist";

    return -1;
}

string NodeImp::getName( TarsCurrentPtr current )
{
    TLOGDEBUG(FILE_FUN << endl);

    _nodeInfo = _platformInfo.getNodeInfo();

    return _nodeInfo.nodeName;
}

LoadInfo NodeImp::getLoad( TarsCurrentPtr current )
{
    TLOGDEBUG(FILE_FUN<< endl);

    return  _platformInfo.getLoadInfo();
}

int NodeImp::shutdown( string &result,TarsCurrentPtr current )
{
    try
    {
        result = FILE_FUN_STR;

        TLOGDEBUG( result << endl);
        if ( g_app.isValid(current->getIp()) == false )
        {
            result += "erro:ip "+ current->getIp()+" is invalid";
            TLOGERROR(FILE_FUN << result << endl);
            return -1;
        }
        getApplication()->terminate();
        return 0;
    }
    catch ( exception& e )
    {
        TLOGERROR( "NodeImp::shutdown catch exception :" << e.what() << endl);
    }
    catch ( ... )
    {
        TLOGERROR( "NodeImp::shutdown catch unkown exception" << endl);
    }
    return -1;
}

int NodeImp::stopAllServers( string &result,TarsCurrentPtr current )
{
    try
    {
        if ( g_app.isValid(current->getIp()) == false )
        {
            result = FILE_FUN_STR+" erro:ip "+ current->getIp()+" is invalid";
            TLOGERROR( result << endl);
            return -1;
        }

        map<string, ServerGroup> mmServerList;
        mmServerList.clear();
        mmServerList = ServerFactory::getInstance()->getAllServers();
        for ( map<string, ServerGroup>::const_iterator it = mmServerList.begin(); it != mmServerList.end(); it++ )
        {
            for ( map<string, ServerObjectPtr>::const_iterator p = it->second.begin(); p != it->second.end(); p++ )
            {
	            string serverId = it->first+"."+ p->first;

	            ServerObjectPtr pServerObjectPtr = p->second;
                if ( pServerObjectPtr )
                {
                    result = result + "stop server ["+serverId+"] ";

                    NODE_LOG(serverId) ->debug() << FILE_FUN << result << endl;
                    
                    bool bByNode = true;
                    string s;
                    CommandStop command(pServerObjectPtr,true,bByNode);
                    int iRet = command.doProcess(s);
                    if (iRet == 0)
                    {
                        result = result+"succ:"+s+"\n";
                    }
                    else
                    {
                        result = result+"error:"+s+"\n";
                    }

	                NODE_LOG(serverId) ->debug() << FILE_FUN << result << endl;

                }
            }
        }
        return 0;
    }
    catch ( exception& e )
    {
        TLOGERROR( FILE_FUN<<" catch exception :" << e.what() << endl);
        result += e.what();
    }
    catch ( ... )
    {
        TLOGERROR( FILE_FUN<<" catch unkown exception" << endl);
        result += "catch unkown exception";
    }
    TLOGDEBUG("stop succ"<<endl);
    return -1;
}

int NodeImp::loadServer( const string& application, const string& serverName,string &result, TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	try
    {
        result = string(__FUNCTION__)+" ["+serverId+"] ";
        NODE_LOG(serverId)->debug() << "NodeImp::loadServer"<<result<<endl;

        if ( g_app.isValid(current->getIp()) == false )
        {
            result += " erro:ip "+ current->getIp()+" is invalid";
	        NODE_LOG(serverId)->error() << FILE_FUN << result << endl;
            return -1;
        }

        string s;

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->loadServer(application,serverName,false,s);
        if (!pServerObjectPtr)
        {
            result =result+" error::cannot load server description.\n"+s;
	        NODE_LOG(serverId)->error() << FILE_FUN << result << endl;

	        return EM_TARS_LOAD_SERVICE_DESC_ERR;
        }
        result = result + " succ" + s;

	    NODE_LOG(serverId)->debug() << "NodeImp::loadServer"<<result<<endl;

	    return 0;
    }
    catch ( exception& e )
    {
	    NODE_LOG(serverId)->error() << FILE_FUN<<" catch exception :" << e.what() << endl;
        result += e.what();
    }
    catch ( ... )
    {
	    NODE_LOG(serverId)->error() <<  FILE_FUN<<" catch unkown exception" << endl;
        result += "catch unkown exception";
    }
    return -1;
}

int NodeImp::startServer( const string& application, const string& serverName,string &result, TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
        result = string(__FUNCTION__)+ " ["+serverId+"] from "+ current->getIp()+" ";
        NODE_LOG(serverId)->debug()<<FILE_FUN << result << endl;
        if ( g_app.isValid(current->getIp()) == false )
        {
            result += " erro:ip "+ current->getIp()+" is invalid";
            NODE_LOG(serverId)->error()<<FILE_FUN << result << endl;
            return EM_TARS_INVALID_IP_ERR;
        }

        NODE_LOG(serverId)->debug()<<FILE_FUN<<result <<"|load begin"<< endl;

        string s;

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->loadServer( application, serverName,true,s);
        // NODE_LOG(serverId)->debug()<<FILE_FUN<<result<<"|load"<< endl;

        if ( pServerObjectPtr )
        {
            bool bByNode = true;
            CommandStart command(pServerObjectPtr,bByNode);
            iRet = command.doProcess(s);
            if (iRet == 0 )
            {
                result = result+":server is activating, please check: "+s;
            }
            else
            {
                result = result+"error:"+s;
            }
            NODE_LOG(serverId)->debug()<<FILE_FUN <<result << endl;
            return iRet;
        }
        
        //设置服务状态enable
        pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
        if (pServerObjectPtr)
        {
            pServerObjectPtr->setEnabled(true);
        }

        result += "error::cannot load server description from registry.\n" + s;
        iRet = EM_TARS_LOAD_SERVICE_DESC_ERR;
    }
    catch ( exception& e )
    {
        NODE_LOG(serverId)->error() << FILE_FUN << " catch exception:" << e.what() << endl;
        result += e.what();
    }
    catch ( ... )
    {
        NODE_LOG(serverId)->error() << FILE_FUN <<" catch unkown exception" << endl;
        result += "catch unkown exception";
    }
    return iRet;
}

int NodeImp::stopServer( const string& application, const string& serverName,string &result, TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
        result = string(__FUNCTION__)+" ["+serverId+"] from "+ current->getIp()+" ";

        NODE_LOG(serverId)->debug() <<FILE_FUN<<result << endl;

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );

        if (pServerObjectPtr)
        {
            NODE_LOG(serverId)->debug() << "server exists" << endl;

            string s;
            bool bByNode = true;
            CommandStop command(pServerObjectPtr,true, bByNode);
            iRet = command.doProcess(current,s);
            if (iRet == 0 )
            {
                result = result+"succ:"+s;
            }
            else
            {
                result = result+"error:"+s;
            }
            NODE_LOG(serverId)->debug()<<FILE_FUN <<result << endl;
        }
        else {
	        NODE_LOG(serverId)->debug() <<FILE_FUN<< " " << serverId << " not exists, load from registry" << endl;

	        pServerObjectPtr = ServerFactory::getInstance()->loadServer(application, serverName, false, result);
        }

        if(!pServerObjectPtr)
        {
            result += "server is  not exist";
            iRet = EM_TARS_LOAD_SERVICE_DESC_ERR;
        }

	    NODE_LOG(serverId)->debug() << FILE_FUN<< " ret:" << result << endl;

	    return iRet;
    }
    catch ( exception& e )
    {
        NODE_LOG(serverId)->error() << FILE_FUN<<" catch exception :" << e.what() << endl;
        result += e.what();
    }
    catch ( ... )
    {
        NODE_LOG(serverId)->error() << FILE_FUN<<" catch unkown exception" << endl;
        result += "catch unkown exception";
    }
    return EM_TARS_UNKNOWN_ERR;
}

int NodeImp::notifyServer( const string& application, const string& serverName, const string &sMsg, string &result, TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	try
    {
        result = string(__FUNCTION__)+" ["+serverId+"] '" + sMsg + "' ";
        NODE_LOG(serverId)->debug() << result << endl;

        if ( g_app.isValid(current->getIp()) == false )
        {
            result += " erro:ip "+ current->getIp()+" is invalid";
	        NODE_LOG(serverId)->error() << "NodeImp::notifyServer " << result << endl;
            return EM_TARS_INVALID_IP_ERR;
        }

        if ( application == "tars" && serverName == "tarsnode" )
        {
            AdminFPrx pAdminPrx;    //服务管理代理
            pAdminPrx = Application::getCommunicator()->stringToProxy<AdminFPrx>("AdminObj@"+ServerConfig::Local);
            result = pAdminPrx->notify(sMsg);
            return 0;
        }

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
        if ( pServerObjectPtr )
        {
            string s;
            CommandNotify command(pServerObjectPtr,sMsg);
            int iRet = command.doProcess(s);
            if (iRet == 0 )
            {
                result = result + "succ:" + s;
            }
            else
            {
                result = result + "error:" + s;
            }
            return iRet;
        }

        result += "server is not exist";
    }
    catch ( exception& e )
    {
	    NODE_LOG(serverId)->error() << "NodeImp::notifyServer catch exception :" << e.what() << endl;
        result += e.what();
    }
    catch ( ... )
    {
	    NODE_LOG(serverId)->error() <<  "NodeImp::notifyServer catch unkown exception" << endl;
        result += "catch unkown exception";
    }
    return -1;
}

int NodeImp::getServerPid( const string& application, const string& serverName,string &result,TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	result = string(__FUNCTION__)+" ["+serverId+"] ";

    NODE_LOG(serverId)->debug() << "NodeImp::getServerPid " << result  << endl;

    ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
    if ( pServerObjectPtr )
    {
        result += "succ";
        return pServerObjectPtr->getPid();
    }
    result += "server not exist";

	NODE_LOG(serverId)->error() << "NodeImp::getServerPid " << result << endl;

    return -1;
}

ServerState NodeImp::getSettingState( const string& application, const string& serverName,string &result, TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	result = string(__FUNCTION__)+" ["+serverId+"] ";
    ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
    if ( pServerObjectPtr )
    {
        result += "succ";
        return pServerObjectPtr->isEnabled()==true?tars::Active:tars::Inactive;
    }
    result += "server not exist";

	NODE_LOG(serverId)->error() << "NodeImp::getSettingState " << result << endl;

    return tars::Inactive;
}

ServerState NodeImp::getState( const string& application, const string& serverName, string &result, TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	result = string(__FUNCTION__)+" ["+serverId+"] ";
    ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
    if ( pServerObjectPtr )
    {
        result += "succ";
        return pServerObjectPtr->getState();
    }
    result += "server not exist";
	NODE_LOG(serverId)->error() << "NodeImp::getState " << result << endl;
    return tars::Inactive;
}

tars::Int32 NodeImp::getStateInfo(const std::string & application,const std::string & serverName,tars::ServerStateInfo &info,std::string &result,tars::TarsCurrentPtr current)
{
	string serverId = application + "." + serverName;

	result = string(__FUNCTION__)+" ["+application + "." + serverName+"] ";
    ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
    if ( pServerObjectPtr )
    {
        result += "succ";

        info.serverState = pServerObjectPtr->getState();
        info.processId = pServerObjectPtr->getPid();
        info.settingState = pServerObjectPtr->isEnabled()==true?tars::Active:tars::Inactive;

	    NODE_LOG(serverId)->debug() << "NodeImp::getStateInfo " << result << endl;

        return EM_TARS_SUCCESS;
    }

    result += "server not exist";
	NODE_LOG(serverId)->error() << "NodeImp::getStateInfo " << result << endl;

    info.serverState = tars::Inactive;
    info.processId = -1;
    info.settingState = tars::Inactive;

    return EM_TARS_UNKNOWN_ERR;
}

int NodeImp::synState( const string& application, const string& serverName, string &result, TarsCurrentPtr current )
{
	string serverId = application + "." + serverName;

	try
    {
        result = string(__FUNCTION__)+" ["+application + "." + serverName+"] ";
        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
        if ( pServerObjectPtr )
        {
            result += "succ";
            pServerObjectPtr->synState();
            return  0;
        }
        result += "server not exist";
	    NODE_LOG(serverId)->debug() << "NodeImp::synState "<<result<< endl;
    }
    catch ( exception& e )
    {
	    NODE_LOG(serverId)->error() << "NodeImp::synState catch exception :" << e.what() << endl;
        result += e.what();
    }
    catch ( ... )
    {
	    NODE_LOG(serverId)->error() << "NodeImp::synState catch unkown exception" << endl;
        result += "catch unkown exception";
    }
    return -1;
}

int NodeImp::getPatchPercent( const string& application, const string& serverName,PatchInfo &tPatchInfo, TarsCurrentPtr current)
{
	string serverId = application + "." + serverName;

	string &result =  tPatchInfo.sResult;
    try
    {
        result = string(__FUNCTION__)+" ["+application + "." + serverName+"] ";

        ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
        if ( pServerObjectPtr )
        {
            result += "succ";
	        NODE_LOG(serverId)->debug()<<FILE_FUN << result << "|get ServerObj" << endl;
            return pServerObjectPtr->getPatchPercent(tPatchInfo);
        }

        result += "server not exist";
	    NODE_LOG(serverId)->error() << FILE_FUN <<" "<< result<< endl;
        return EM_TARS_LOAD_SERVICE_DESC_ERR;
    }
    catch ( exception& e )
    {
	    NODE_LOG(serverId)->error() << FILE_FUN << " catch exception :" << e.what() << endl;
        result += e.what();
    }
    catch ( ... )
    {
	    NODE_LOG(serverId)->error() << FILE_FUN <<" catch unkown exception" << endl;
        result += "catch unkown exception";
    }

    return EM_TARS_UNKNOWN_ERR;

}

tars::Int32 NodeImp::delCache(const  string &sFullCacheName, const std::string &sBackupPath, const std::string & sKey, std :: string &result, TarsCurrentPtr current)
{
#if TARGET_PLATFORM_WINDOWS
	 return -1;
#else
    try
    {
        TLOGDEBUG("NodeImp::delCache "<<sFullCacheName << "|" << sBackupPath << "|" << sKey << endl);
        if (sFullCacheName.empty())
        {
            result = "cache server name is empty";
            throw runtime_error(result);
        }

        string sBasePath = "/usr/local/app/tars/tarsnode/data/"+sFullCacheName+"/bin/";
        if (!TC_File::isFileExistEx(sBasePath, S_IFDIR))
        {
            result = "no such directory:" + sBasePath;
            return 0;
        }

        key_t key;
        if (sKey.empty())
        {
            key = ftok(sBasePath.c_str(), 'D');
        }
        else
        {
            key = TC_Common::strto<key_t>(sKey);
        }
        TLOGDEBUG("NodeImp::delCache|key=" << key << endl);

        int shmid = shmget(key, 0, 0666);
        if (shmid == -1)
        {
            result = "failed to shmget " + sBasePath + "|key=" + TC_Common::tostr(key);
            //如果获取失败则认为共享内存已经删除,直接返回成功
            TLOGDEBUG("NodeImp::delCache"<< "|"  << sFullCacheName << "|" << result << endl);
            return 0;
        }

        shmid_ds *pShmInfo= new shmid_ds;
        int iRet = shmctl(shmid, IPC_STAT, pShmInfo);
        if (iRet != 0)
        {
            result = "failed to shmctl " + sBasePath + "|key=" + TC_Common::tostr(key) + "|ret=" + TC_Common::tostr(iRet);
            delete pShmInfo;
            pShmInfo = NULL;
            throw runtime_error(result);
        }

        if (pShmInfo->shm_nattch>=1)
        {
            result = "current attach count >= 1,please check it |" + sBasePath + "|key=" +TC_Common::tostr(key)+"|attch_count=" + TC_Common::tostr(pShmInfo->shm_nattch);
            delete pShmInfo;
            pShmInfo = NULL;
            throw runtime_error(result);
        };

        delete pShmInfo;
        pShmInfo = NULL;

        int ret =shmctl(shmid, IPC_RMID, NULL);
        if (ret !=0)
        {
            result = "failed to rm share memory|key=" + TC_Common::tostr(key) + "|shmid=" + TC_Common::tostr(shmid) + "|ret=" + TC_Common::tostr(ret);
            throw runtime_error(result);
        }

        TLOGDEBUG("NodeImp::delCache success delete cache:" << sFullCacheName <<", no need backup it"<< endl);

        return 0;
    }
    catch ( exception& e )
    {
        result = TC_Common::tostr(__FILE__)+":"+TC_Common::tostr(__FUNCTION__) + ":"+ TC_Common::tostr(__LINE__)  + "|" + e.what();
        TLOGERROR("NodeImp::delCache " << result << endl);
        TARS_NOTIFY_ERROR(result);
    }

    return -1;
#endif

	 return -1;
}

tars::Int32 NodeImp::getUnusedShmKeys(tars::Int32 count,vector<tars::Int32> &shm_keys,tars::TarsCurrentPtr current) {
#if TARGET_PLATFORM_WINDOWS
	 return -1;
#else
    int ret = 0;
    for(size_t i = 0; i < 256 && shm_keys.size() < (size_t)count; i++) 
    {
        key_t key = ftok(ServerConfig::BasePath.c_str(), i);
        if(key == -1) 
        {
            TLOGDEBUG("NodeImp::getUnusedShmKeys create an key failed, i=" << i << endl);
        }
        else 
        {
            int shmid = shmget(key, 0, 0666);
            if(shmid == -1) 
            {
                TLOGDEBUG("NodeImp::getUnusedShmKeys find an key:" << key << " 0x" << keyToStr(key) << endl);
                shm_keys.push_back(key);
            }
            else 
            {
                TLOGDEBUG("NodeImp::getUnusedShmKeys key: " << key << " 0x" << keyToStr(key) << " is busy"<< endl);
            }
        }
    }

    return ret;
#endif // TARGET_PLATFORM_WINDOWS

	 return -1;	 
}

string NodeImp::keyToStr(key_t key_value)
{
    char buf[32];
    snprintf(buf, 32, "%08x", key_value);
    string s(buf);
    return s;
}

int NodeImp::getLogFileList(const string& application, const string& serverName, vector<string>& logFileList,tars::TarsCurrentPtr current)
{
	string serverId = application + "." + serverName;

	string logPath = ServerConfig::LogPath + FILE_SEP + application + FILE_SEP + serverName + FILE_SEP;

	NODE_LOG(serverId)->debug() << "logpath:" << logPath << endl;

    vector<string> logFileListTmp;
    TC_File::listDirectory(logPath, logFileListTmp, false);

    for (size_t i = 0; i < logFileListTmp.size(); ++i)
    {
        string fileName = TC_File::extractFileName(logFileListTmp[i]);
        if(fileName.length() >= 5 && fileName.find(".log") != string::npos)
        {
            logFileList.push_back(fileName);
        }
    }
    return 0;
}

int NodeImp::getLogData(const string& application, const string& serverName, const string& logFile, const string& cmd, string& fileData, tars::TarsCurrentPtr current)
{
	string serverId = application + "." + serverName;

	string filePath = ServerConfig::LogPath + FILE_SEP + application + FILE_SEP + serverName + FILE_SEP + logFile;
    if (!TC_File::isFileExistEx(filePath))
    {
	    NODE_LOG(serverId)->debug() << "The log file: " << filePath << " is not exist" << endl;
        return -1;
    }

    string newCmd = TC_Common::replace(cmd, "__log_file_name__", filePath);

	NODE_LOG(serverId)->debug() << "[NodeImp::getLogData] cmd:" << newCmd << endl;

#if TARGET_PLATFORM_WINDOWS
    string tmpCmd;
    vector<string> subCmd = TC_Common::sepstr<string>(newCmd, "|");
    for(size_t i = 0; i < subCmd.size(); i++)
    {
        tmpCmd += ServerConfig::TarsPath + FILE_SEP + "tarsnode" + FILE_SEP + "util\\busybox.exe " + subCmd[i]; 
        if(i != subCmd.size() - 1)
        {
            tmpCmd += " | "; 
        }
    }

    newCmd = tmpCmd;
#endif

	NODE_LOG(serverId)->debug() << "[NodeImp::getLogData] newcmd:" << newCmd << endl;

    string errstr;
    fileData = TC_Port::exec(newCmd.c_str(), errstr);
    if (fileData.empty()) {
        if (!errstr.empty()) {
            fileData = errstr;
            NODE_LOG(serverId)->error() << errstr << endl;
        }
    }
    
// #if TARGET_PLATFORM_WINDOWS

//     FILE* fp = _popen(newCmd.c_str(), "r");

//     static size_t buf_len = 2 * 1024 * 1024;
//     char *buf = new char[buf_len];
//     memset(buf, 0, buf_len);
//     fread(buf, sizeof(char), buf_len - 1, fp);
//     fclose(fp);

//     fileData = string(buf);
//     delete []buf;

// #else
//     FILE* fp = popen(newCmd.c_str(), "r");

//     static size_t buf_len = 2 * 1024 * 1024;
//     char *buf = new char[buf_len];
//     memset(buf, 0, buf_len);
//     fread(buf, sizeof(char), buf_len - 1, fp);
//     fclose(fp);

//     fileData = string(buf);
//     delete []buf;
// #endif

    return 0;
}
//
//string fillLine(int num)
//{
//	string str;
//	while (num -- > 0)
//	{
//
//	}
//	return str;
//}

// TASKLIST /FI "USERNAME ne NT AUTHORITY\SYSTEM" /FI "STATUS eq running" /V /FO TABLE
int NodeImp::getNodeLoad(const string& application, const string& serverName, int pid, string& fileData, tars::TarsCurrentPtr current)
{
	string serverId = application + "." + serverName;

	NODE_LOG(serverId)->debug() << serverId << ", pid:" << pid << endl;
#if TARGET_PLATFORM_WINDOWS
	fileData = "not support!";	
#else
	string cmd = "top -c -bw 160 -n 1 -o '%CPU' -o '%MEM'";

    string errstr;
    fileData = TC_Port::exec(cmd.c_str(), errstr);
    if (fileData.empty()) {
        if (!errstr.empty()) {
            fileData = errstr;
            NODE_LOG(serverId)->error() << errstr << endl;
        }
    }
    
	// FILE* fpTop = popen(cmd.c_str(), "r");
	// char   buf[1 * 1024 * 1024] = { 0 };
	// fread(buf, sizeof(char), sizeof(buf)-1, fpTop);
	// fclose(fpTop);
	// fileData = string(buf);
	fileData += "#global-top-end#";

	if (pid > 0)
	{
//		memset(buf, 0, sizeof(buf));
		cmd = "top -b -n 1 -o '%CPU' -o '%MEM' -H -p " + TC_Common::tostr(pid);
		// FILE* fpTop2 = popen(cmd.c_str(), "r");	
		// fread(buf, sizeof(char), sizeof(buf)-1, fpTop2);
		// fclose(fpTop2);
		fileData += "\n\n";
		fileData += "#this-top-begin#" + string(100, '-');
		fileData += "\n";
        
        errstr.clear();
        fileData += TC_Port::exec(cmd.c_str(), errstr);
        if (fileData.empty()) {
            if (!errstr.empty()) {
                fileData += errstr;
                NODE_LOG(serverId)->error() << errstr << endl;
            }
        }
		// fileData += string(buf);
		fileData += "#this-top-end#";
	}
	
	// memset(buf, 0, sizeof(buf));
	cmd = "cd /usr/local/app/taf/app_log/; ls -alth core.*";
	// FILE* fpLL = popen(cmd.c_str(), "r");
	// fread(buf, sizeof(char), sizeof(buf)-1, fpLL);
	// fclose(fpLL);
	fileData += "\n\n";
	fileData += "#core-file-begin#" + string(100, '-');
	fileData += "\n";

    errstr.clear();
    fileData = TC_Port::exec(cmd.c_str(), errstr);
    if (fileData.empty()) {
        if (!errstr.empty()) {
            fileData = errstr;
            NODE_LOG(serverId)->error() << errstr << endl;
        }
    }
	// fileData += string(buf);

	NODE_LOG(serverId)->debug() << fileData << endl;
#endif

	return 0;
}

/*********************************************************************/





