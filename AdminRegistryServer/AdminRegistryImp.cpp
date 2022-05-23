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

#include "AdminRegistryImp.h"
#include "ExecuteTask.h"
#include "servant/Application.h"
#include "servant/RemoteNotify.h"
#include "AdminRegistryServer.h"
#include "util/tc_docker.h"
#include "NodeManager.h"
#include "NodePush.h"

extern TC_Config * g_pconf;

void AdminRegistryImp::initialize()
{
    TLOG_DEBUG("begin AdminRegistryImp init"<<endl);

    _patchPrx = Application::getCommunicator()->stringToProxy<PatchPrx>(g_pconf->get("/tars/objname<patchServerObj>", "tars.tarspatch.PatchObj"));

	_registryPrx = Application::getCommunicator()->stringToProxy<RegistryPrx>(g_pconf->get("/tars/objname<RegistryObjName>"), "tars.tarsregistry.RegistryObj");

	_registryPrx->tars_async_timeout(60*1000);

	int timeout = TC_Common::strto<int>(g_pconf->get("/tars/patch<patch_timeout>", "30000"));

	_patchPrx->tars_set_timeout(timeout);

	_remoteLogIp = g_pconf->get("/tars/log<remotelogip>", "");

    _remoteLogObj = g_pconf->get("/tars/log<remotelogobj>", "tars.tarslog.LogObj");

	_dockerSocket = g_pconf->get("/tars/container<socket>", "/var/run/docker.sock");
    TLOG_DEBUG("AdminRegistryImp init ok. _remoteLogIp:" << _remoteLogIp << ", _remoteLogObj:" << _remoteLogObj << endl);
}

int AdminRegistryImp::doClose(CurrentPtr current)
{
	TLOG_DEBUG("uid:" << current->getUId() << endl);
	NodeManager::getInstance()->eraseNodeCurrent(current);
	return 0;
}

int AdminRegistryImp::reportNode(const ReportNode &rn, CurrentPtr current)
{
//	TLOG_DEBUG("nodeName:" << nodeName << ", uid:" << current->getUId() << endl);

	NodeManager::getInstance()->createNodeCurrent(rn.nodeName, rn.sid, current);
	return 0;
}

int AdminRegistryImp::deleteNode(const ReportNode &rn, CurrentPtr current)
{
	NodeManager::getInstance()->deleteNodeCurrent(rn.nodeName, rn.sid, current);
	return 0;
}

int AdminRegistryImp::reportResult(int requestId, const string &funcName, int ret, const string &result, CurrentPtr current)
{
	TLOG_DEBUG("requestId:" << requestId << ", " << funcName << ", ret:" << ret << endl);

	NodeManager::getInstance()->reportResult(requestId, funcName, ret, result, current);
	return 0;
}

int AdminRegistryImp::undeploy(const string & application, const string & serverName, const string & nodeName, const string &user, string &log, tars::CurrentPtr current)
{
	TLOG_DEBUG("application:" << application
		<< ", serverName:" << serverName
		<< ", nodeName:" << nodeName << endl);

	return undeploy_inner(application, serverName, nodeName, user, log);
}

int AdminRegistryImp::undeploy_inner(const string & application, const string & serverName, const string & nodeName, const string &user, string &log)
{
	TLOG_DEBUG("application:" << application
		<< ", serverName:" << serverName
		<< ", nodeName:" << nodeName << endl);

	stopServer_inner(application, serverName, nodeName, log);
	return DBPROXY->undeploy(application, serverName, nodeName, user, log);
}

int AdminRegistryImp::addTaskReq(const TaskReq &taskReq, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::addTaskReq taskNo:" << taskReq.taskNo <<endl);

    int ret = DBPROXY->addTaskReq(taskReq);
    if (ret != 0)
    {
        TLOG_ERROR("AdminRegistryImp::addTaskReq error, ret:" << ret <<endl);
        return ret;
    }

    ExecuteTask::getInstance()->addTaskReq(taskReq); 

    return 0;
}

int AdminRegistryImp::getTaskRsp(const string &taskNo, TaskRsp &taskRsp, tars::CurrentPtr current)
{
    //优先从内存中获取
    bool ret = ExecuteTask::getInstance()->getTaskRsp(taskNo, taskRsp);
    if (ret)
    {
        return 0;
    }

    return DBPROXY->getTaskRsp(taskNo, taskRsp);
}

int AdminRegistryImp::getTaskHistory(const string & application, const string & serverName, const string & command, vector<TaskRsp> &taskRsp, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::getTaskHistory application:" << application << "|serverName:" << serverName <<endl);

    return DBPROXY->getTaskHistory(application, serverName, command, taskRsp);
}

int AdminRegistryImp::setTaskItemInfo(const string & itemNo, const map<string, string> &info, tars::CurrentPtr current)
{
	return setTaskItemInfo_inner(itemNo, info);
}

int AdminRegistryImp::setTaskItemInfo_inner(const string & itemNo, const map<string, string> &info)
{
	return DBPROXY->setTaskItemInfo(itemNo, info);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
///
void AdminRegistryImp::deleteHistorys(const string &application, const string &serverName)
{
    TLOG_DEBUG("into " << __FUNCTION__ << endl);
    try
    {
        vector<string> patchFiles = DBPROXY->deleteHistorys(application, serverName);

        TLOG_DEBUG("into " << __FUNCTION__ << ", patch file size:" << patchFiles.size() << endl);

        for(auto patchFile : patchFiles)
        {
            _patchPrx->async_deletePatchFile(NULL, application, serverName, patchFile);
        }
    }
    catch(exception &ex)
    {
        TLOG_ERROR("into " << __FUNCTION__ << ", error:" << ex.what() << endl);
    }
}

vector<string> AdminRegistryImp::getAllApplicationNames(string & result, tars::CurrentPtr current)
{
	TLOG_DEBUG("into " << __FUNCTION__ << endl);
    return DBPROXY->getAllApplicationNames(result);
}

vector<string> AdminRegistryImp::getAllNodeNames(string & result, tars::CurrentPtr current)
{
    map<string, string> mNodes = DBPROXY->getActiveNodeList(result);
    map<string, string>::iterator it;
    vector<string> vNodes;

    TLOG_DEBUG("AdminRegistryImp::getAllNodeNames enter" <<endl);
    for(it = mNodes.begin(); it != mNodes.end(); it++)
    {
        vNodes.push_back(it->first);
    }

    return vNodes;
}

int AdminRegistryImp::getNodeVesion(const string &nodeName, string &version, string & result, tars::CurrentPtr current)
{
    try
    {
        TLOG_DEBUG("into " << __FUNCTION__ << endl);
        return DBPROXY->getNodeVersion(nodeName, version, result);
    }
    catch(TarsException & ex)
    {
        result = string(string(__FUNCTION__)) + " '" + nodeName + "' exception:" + ex.what();
        TLOG_ERROR(result << endl);
    }
    return -1;
}

bool AdminRegistryImp::pingNode(const string & name, string & result, tars::CurrentPtr current)
{
    try
    {
        TLOG_DEBUG("into " << __FUNCTION__ << "|" << name << endl);

		return NodeManager::getInstance()->pingNode(name, result, current);
    }
    catch(TarsException & ex)
    {
        result = string(string(__FUNCTION__)) + " '" + name + "' exception:" + ex.what();
        TLOG_ERROR(result << endl);
        return false;
    }

    return false;
}

int AdminRegistryImp::shutdownNode(const string & name, string & result, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::shutdownNode name:"<<name<<"|"<<current->getHostName()<<":"<<current->getPort()<<endl);
    try
    {
		return NodeManager::getInstance()->shutdownNode(name, result, current);
    }
    catch(TarsException & ex)
    {
        result = string(__FUNCTION__) + " '" + name + "' exception:" + ex.what();
        TLOG_ERROR( result << endl);
        return -1;
    }
}

///////////////////////////////////
vector<vector<string> > AdminRegistryImp::getAllServerIds(string & result, tars::CurrentPtr current)
{
    TLOG_DEBUG(__FILE__ << "|" << __LINE__ << "|into " << __FUNCTION__ << "|" << current->getHostName() << ":" << current->getPort() << endl);

    return DBPROXY->getAllServerIds(result);
}

int AdminRegistryImp::getServerState(const string & application, const string & serverName, const string & nodeName, ServerStateDesc &state, string &result, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::getServerState:" << application << "." << serverName << "_" << nodeName << "|" << current->getHostName() << ":" << current->getPort() <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(application, serverName, nodeName, true);
        if(server.size() == 0)
        {
            result = " '" + application  + "." + serverName + "_" + nodeName + "' no config";
            TLOG_ERROR("AdminRegistryImp::getServerState:" << result << endl);

            return EM_TARS_LOAD_SERVICE_DESC_ERR;
        }

        state.settingStateInReg = server[0].settingState;
        state.presentStateInReg = server[0].presentState;
        state.patchVersion      = server[0].patchVersion;
        state.patchTime         = server[0].patchTime;
        state.patchUser         = server[0].patchUser;
//        state.bakFlag = server[0].bakFlag;

        //判断是否为dns 非dns才需要到node调用
        if(server[0].serverType == "tars_dns")
        {
            TLOG_DEBUG("AdminRegistryImp::getServerState " << ("'" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server") <<endl);
            state.presentStateInNode = server[0].presentState;
        }
        else
        {
			return NodeManager::getInstance()->getServerState(application, serverName, nodeName, state, result, current);
        }

        TLOG_DEBUG("AdminRegistryImp::getServerState: "  << application << "." << serverName << "_" << nodeName << "|" << current->getHostName() << ":" << current->getPort() <<endl);

        return EM_TARS_SUCCESS;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' TarsSyncCallTimeoutException:" + ex.what();
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' TarsNodeNotRegistryException:" + ex.what();
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
    }
    catch(TarsException & ex)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' TarsException:" + ex.what();
    }
    catch(exception & tce)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' exception:" + tce.what();

    }
    TLOG_ERROR(result << endl);
    return iRet;

}

int AdminRegistryImp::getGroupId(const string & ip, int &groupId, string &result, tars::CurrentPtr current)
{
    try
    {
        TLOG_DEBUG("AdminRegistryImp::getGroupId ip: "<<ip<<endl);

        return DBPROXY->getGroupId(ip);
    }
    catch(TarsException & ex)
    {
        TLOG_ERROR(("AdminRegistryImp::getGroupId '" + ip + "' exception:" + ex.what())<< endl);
        return -1;
    }
}

int AdminRegistryImp::destroyServer(const string & application, const string & serverName, const string & nodeName, string &result, CurrentPtr current)
{
	TLOG_DEBUG("AdminRegistryImp::destroyServer: "<< application << "." << serverName << "_" << nodeName
											   << "|" << current->getHostName() << ":" << current->getPort() <<endl);

	int iRet = EM_TARS_UNKNOWN_ERR;
	try
	{
		//更新数据库server的设置状态
		DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Inactive);

		vector<ServerDescriptor> server;
		server = DBPROXY->getServers(application, serverName, nodeName, true);

		//判断是否为dns 非dns才需要到node启动服务
		if(server.size() != 0 && server[0].serverType == "tars_dns")
		{
			TLOGINFO( "|" << " '" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
			iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Inactive);
			TLOG_DEBUG( __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName
									 << "|" << current->getHostName() << ":" << current->getPort() << "|" << iRet <<endl);
		}
		else
		{
			iRet = NodeManager::getInstance()->destroyServer(application, serverName, nodeName, result, current);
		}

		return iRet;
	}
	catch(TarsSyncCallTimeoutException& ex)
	{
		current->setResponse(true);
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		RemoteNotify::getInstance()->report(string("destroyServer:") + ex.what(), application, serverName, nodeName);
		TLOG_ERROR("AdminRegistryImp::destroyServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsSyncCallTimeoutException:" + ex.what())<<endl);
	}
	catch(TarsNodeNotRegistryException& ex)
	{
		current->setResponse(true);
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		RemoteNotify::getInstance()->report(string("destroyServer:") + ex.what(), application, serverName, nodeName);
		TLOG_ERROR("AdminRegistryImp::destroyServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsNodeNotRegistryException:" + ex.what())<<endl);
	}
	catch(TarsException & ex)
	{
		current->setResponse(true);
		result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
				 + "' Exception:" + ex.what();
		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOG_ERROR(result << endl);
	}
	return iRet;
}

int AdminRegistryImp::startServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::startServer: "<< application << "." << serverName << "_" << nodeName
        << "|" << current->getHostName() << ":" << current->getPort() <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {

        //更新数据库server的设置状态
        DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);

        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(application, serverName, nodeName, true);

        //判断是否为dns 非dns才需要到node启动服务
        if(server.size() != 0 && server[0].serverType == "tars_dns")
        {
            TLOGINFO(" '" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
            iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
        }
        else
        {
			return NodeManager::getInstance()->startServer(application, serverName, nodeName, result, current);
        }

        return iRet;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        RemoteNotify::getInstance()->report(string("start server:") + ex.what(), application, serverName, nodeName);
        TLOG_ERROR("AdminRegistryImp::startServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsSyncCallTimeoutException:" + ex.what())<<endl);
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        RemoteNotify::getInstance()->report(string("start server:") + ex.what(), application, serverName, nodeName);
        TLOG_ERROR("AdminRegistryImp::startServer '"<<(application  + "." + serverName + "_" + nodeName + "' TarsNodeNotRegistryException:" + ex.what())<<endl);
    }
    catch(TarsException & ex)
    {
        current->setResponse(true);
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' TarsException:" + ex.what();
		RemoteNotify::getInstance()->report(string("start server:") + ex.what(), application, serverName, nodeName);
		 
        TLOG_ERROR(result << endl);
    }

	if(iRet != EM_TARS_SUCCESS)
	{
		RemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
	}
    return iRet;
}

int AdminRegistryImp::startServer_inner(const string & application, const string & serverName, const string & nodeName, string &result)
{
	TLOG_DEBUG("into " << __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName << endl);
	int iRet = EM_TARS_UNKNOWN_ERR;
	try
	{
		DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);
		vector<ServerDescriptor> server;
		server = DBPROXY->getServers(application, serverName, nodeName, true);
		if (server.size() != 0 && server[0].serverType == "tars_dns")
		{
			TLOGINFO(" '" + application + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
			iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
        }
		else
		{
			iRet = NodeManager::getInstance()->startServer(application, serverName, nodeName, result, NULL);
		}

		if(iRet != EM_TARS_SUCCESS)
		{
			RemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
		}
        return iRet;
	}
	catch (TarsSyncCallTimeoutException& tex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsSyncCallTimeoutException:" + tex.what();
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		TLOG_ERROR(result << endl);
	}
	catch (TarsNodeNotRegistryException& re)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsNodeNotRegistryException:" + re.what();
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		TLOG_ERROR(result << endl);
	}
	catch (TarsException & ex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsException:" + ex.what();
		TLOG_ERROR(result << endl);
	}
	return iRet;
}

int AdminRegistryImp::stopServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::stopServer: "<< application << "." << serverName << "_" << nodeName
        << "|" << current->getHostName() << ":" << current->getPort() <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
        //更新数据库server的设置状态
        DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Inactive);

        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(application, serverName, nodeName, true);

        //判断是否为dns 非dns才需要到node启动服务
        if(server.size() != 0 && server[0].serverType == "tars_dns")
        {
            TLOGINFO( "|" << " '" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
            iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Inactive);
            TLOG_DEBUG( __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName
                << "|" << current->getHostName() << ":" << current->getPort() << "|" << iRet <<endl);
        }
        else
        {
			iRet = NodeManager::getInstance()->startServer(application, serverName, nodeName, result, current);
        }

        return iRet;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        RemoteNotify::getInstance()->report(string("stop server:") + ex.what(), application, serverName, nodeName);
        TLOG_ERROR("AdminRegistryImp::stopServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsSyncCallTimeoutException:" + ex.what())<<endl);
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        RemoteNotify::getInstance()->report(string("stop server:") + ex.what(), application, serverName, nodeName);
        TLOG_ERROR("AdminRegistryImp::stopServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsNodeNotRegistryException:" + ex.what())<<endl);
    }
    catch(TarsException & ex)
    {
        current->setResponse(true);
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' Exception:" + ex.what();
		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
        TLOG_ERROR(result << endl);
    }
    return iRet;
}

int AdminRegistryImp::stopServer_inner(const string & application, const string & serverName, const string & nodeName, string &result)
{
	TLOG_DEBUG("into " << __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName << endl);
	int iRet = EM_TARS_UNKNOWN_ERR;
	try
	{
		if(application == "tars" && serverName == "tarsAdminRegistry")
		{
			result = "can not stop " + application + "." + serverName;
			RemoteNotify::getInstance()->report(result);
			return EM_TARS_CAN_NOT_EXECUTE;
		}
		DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Inactive);
		vector<ServerDescriptor> server;
		server = DBPROXY->getServers(application, serverName, nodeName, true);
		if (server.size() != 0 && server[0].serverType == "tars_dns")
		{
			TLOGINFO( "|" << " '" + application + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
			iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Inactive);
			TLOG_DEBUG( "|" << application << "." << serverName << "_" << nodeName << "|" << iRet << endl);
        }
		else
		{
			iRet = NodeManager::getInstance()->stopServer(application, serverName, nodeName, result, NULL);
		}

		if(iRet != EM_TARS_SUCCESS)
		{
			RemoteNotify::getInstance()->report(string("stop server error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
		}
		return iRet;
	}
	catch (TarsSyncCallTimeoutException& tex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsSyncCallTimeoutException:" + tex.what();
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		TLOG_ERROR(result << endl);
	}
	catch (TarsNodeNotRegistryException& re)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsNodeNotRegistryException:" + re.what();
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		TLOG_ERROR(result << endl);
	}
	catch (TarsException & ex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsException:" + ex.what();
		TLOG_ERROR(result << endl);
	}

	return iRet;
}

int AdminRegistryImp::restartServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::CurrentPtr current)
{
    TLOG_DEBUG(" AdminRegistryImp::restartServer: " << application << "." << serverName << "_" << nodeName << "|" << current->getHostName() << ":" << current->getPort() <<endl);

    bool isDnsServer = false;
    int iRet = EM_TARS_SUCCESS;
    try
    {
		if(application == ServerConfig::Application && serverName == ServerConfig::ServerName)
		{
			if(current->getContext()["restart"] == "true")
			{
				this->getApplication()->terminate();
			}
			else
			{
				//给对应的tarsAdminRegistry异步发消息, 然后回包, 然后退出自己
				string obj = g_pconf->get("/tars/objname<AdminRegObjName>", "") + "@tcp -h " + nodeName;

				AdminRegPrx prx = this->getApplication()->getCommunicator()->stringToProxy<AdminRegPrx>(obj);

				map<string, string> context;
				context["restart"] = "true";

				prx->async_restartServer(NULL, application, serverName, nodeName, context);
			}
		}
		else
		{
			vector<ServerDescriptor> server;
			server = DBPROXY->getServers(application, serverName, nodeName, true);

			//判断是否为dns 非dns才需要到node停止、启动服务
			if (server.size() != 0 && server[0].serverType == "tars_dns")
			{
				isDnsServer = true;
			}
			else
			{
				iRet = NodeManager::getInstance()->stopServer(application, serverName, nodeName, result, current);
			}
			TLOG_DEBUG("call node restartServer, stop|" << application << "." << serverName << "_" << nodeName << "|"
														<< current->getHostName() << ":" << current->getPort() << endl);
			if (iRet != EM_TARS_SUCCESS)
			{
				RemoteNotify::getInstance()->report(string("restart server, stop error:" + etos((tarsErrCode)iRet)),
						application, serverName, nodeName);
			}
		}
    }
    catch(TarsException & ex)
    {
    
        TLOG_ERROR(("AdminRegistryImp::restartServer '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + ex.what())<<endl);
        iRet = EM_TARS_UNKNOWN_ERR;
        RemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);
    }

    if(iRet == EM_TARS_SUCCESS)
    {
        try
        {
            //从停止状态发起的restart需重设状态
            DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);

            //判断是否为dns 非dns才需要到node启动服务
            if(isDnsServer == true)
            {
                TLOG_DEBUG( "|" << " '" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
                return DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
            }
            else
            {
				return NodeManager::getInstance()->startServer(application, serverName, nodeName, result, current);
            }
        }
        catch(TarsSyncCallTimeoutException& ex)
        {
            result = "AdminRegistryImp::restartServer '" + application  + "." + serverName + "_" + nodeName
                    + "' SyncCallTimeoutException:" + ex.what();

            iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
            RemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);
        }
        catch(TarsNodeNotRegistryException& ex)
        {
            result = "AdminRegistryImp::restartServer '" + application  + "." + serverName + "_" + nodeName
                    + "' NodeNotRegistryException:" + ex.what();
            RemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);

            iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        }
        catch (TarsException & ex)
        {
			RemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);

            result += string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                      + "' Exception:" + ex.what();
            iRet = EM_TARS_UNKNOWN_ERR;
        }
        TLOG_ERROR( result << endl);
    }

    return iRet;
}

int AdminRegistryImp::restartServer_inner(const string & application, const string & serverName, const string & nodeName, string &result)
{
	TLOG_DEBUG("into " << __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName << endl);
	bool isDnsServer = false;
	int iRet = EM_TARS_SUCCESS;
	try
	{
		vector<ServerDescriptor> server;
		server = DBPROXY->getServers(application, serverName, nodeName, true);
		if (server.size() != 0 && server[0].serverType == "tars_dns")
		{
			isDnsServer = true;
        }
		else
		{
			iRet = NodeManager::getInstance()->stopServer(application, serverName, nodeName, result, NULL);
		}
		if(iRet != EM_TARS_SUCCESS)
		{
			RemoteNotify::getInstance()->report(string("restart server, stop error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
		}
		TLOG_DEBUG("stop " << application << "." << serverName << "_" << nodeName << ", " << etos((tarsErrCode)iRet) << endl);
	}
	catch (exception & ex)
    {
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName + "' exception:" + ex.what();
		TLOG_ERROR(result << endl);
		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);

		iRet = EM_TARS_UNKNOWN_ERR;
	}

	if (iRet == EM_TARS_SUCCESS)
	{
		try
		{
			DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);
            // 重启后， 流量状态恢复正常
            DBPROXY->updateServerFlowStateOne(application, serverName, nodeName, true);
			if (isDnsServer == true)
			{
				TLOG_DEBUG( " '" + application + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
				return DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
            }
			else
			{
				return NodeManager::getInstance()->startServer(application, serverName, nodeName, result, NULL);
			}
		}
		catch (TarsSyncCallTimeoutException& tex)
		{
			result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
				+ "' SyncCallTimeoutException:" + tex.what();
			RemoteNotify::getInstance()->report(result, application, serverName, nodeName);

			iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		}
		catch (TarsNodeNotRegistryException& re)
		{
			result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
				+ "' NodeNotRegistryException:" + re.what();
			RemoteNotify::getInstance()->report(result, application, serverName, nodeName);

			iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		}
		catch (TarsException & ex)
		{
			result += string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
				+ "' Exception:" + ex.what();
			RemoteNotify::getInstance()->report(result, application, serverName, nodeName);

			iRet = EM_TARS_UNKNOWN_ERR;
		}
		TLOG_ERROR(result << endl);
    }
    return iRet;
}

int AdminRegistryImp::notifyServer(const string & application, const string & serverName, const string & nodeName, const string &command, string &result, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::notifyServer: " << application << "." << serverName << "_" << nodeName << "|" << current->getHostName() << ":" << current->getPort() <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
		return NodeManager::getInstance()->notifyServer(application, serverName, nodeName, command, result, current);
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        TLOG_ERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
                + "' SyncCallTimeoutException:" + ex.what())<<endl);
        RemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TLOG_ERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
                + "' NodeNotRegistryException:" + ex.what())<<endl);
        RemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }

    catch(TarsException & ex)
    {
        current->setResponse(true);
        TLOG_ERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
                + "' Exception:" + ex.what())<<endl);
        RemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }
    return iRet;
}

int AdminRegistryImp::notifyServer_inner(const string & application, const string & serverName, const string & nodeName, const string &command, string &result)
{
    TLOG_DEBUG("AdminRegistryImp::notifyServer: " << application << "." << serverName << "_" << nodeName <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
		return NodeManager::getInstance()->notifyServer(application, serverName, nodeName, command, result, NULL);
	}
    catch(TarsSyncCallTimeoutException& ex)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        TLOG_ERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
            + "' SyncCallTimeoutException:" + ex.what())<<endl);
    	RemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
	}
	catch(TarsNodeNotRegistryException& ex)
	{
    	iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
    	TLOG_ERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
    	    + "' TarsNodeNotRegistryException:" + ex.what())<<endl);
    	RemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }
	catch(TarsException & ex)
	{
		TLOG_ERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
			+ "' Exception:" + ex.what())<<endl);
        RemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }
    return iRet;
}

int AdminRegistryImp::batchPatch(const tars::PatchRequest & req, string & result, tars::CurrentPtr current)
{
   tars::PatchRequest reqPro = req;
   reqPro.patchobj = (*g_pconf)["/tars/objname<patchServerObj>"];
	reqPro.servertype = getServerType(req.appname, req.servername, req.nodename);
   int iRet = 0;
   string sServerName;

   try
   {
       sServerName  = reqPro.groupname.empty() ? reqPro.servername : reqPro.groupname;

       TLOG_DEBUG("AdminRegistryImp::batchPatch "
                << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|"
                << reqPro.binname      << "|"
                << reqPro.version      << "|"
                << reqPro.user         << "|"
                << reqPro.servertype   << "|"
                << reqPro.patchobj     << "|"
                << reqPro.md5          <<"|"
                << reqPro.ostype       << "|"
                << sServerName
                << endl);

       //获取patch包的文件信息和md5值
       string patchFile;
       string md5;
       iRet = DBPROXY->getInfoByPatchId(reqPro.version, patchFile, md5);
       if (iRet != 0)
       {
	        result = "get patch tgz error:" + reqPro.version;
           TLOG_ERROR("AdminRegistryImp::batchPatch, get patch tgz error:" << reqPro.version << endl);
           RemoteNotify::getInstance()->report("get patch tgz error:" + reqPro.version, reqPro.appname, sServerName, reqPro.nodename);

           return EM_TARS_GET_PATCH_FILE_ERR;
       }

	    TLOG_DEBUG("AdminRegistryImp::batchPatch " << sServerName << "|" << patchFile << endl);

	    //让tarspatch准备发布包
	    iRet = _patchPrx->preparePatchFile(reqPro.appname, sServerName, patchFile, result);
	    if (iRet != 0)
	    {
			result = "tarspatch preparePatchFile error:" + result;
	        TLOG_ERROR("AdminRegistryImp::batchPatch, prepare patch file " << patchFile << " error:" << result << endl);
	        RemoteNotify::getInstance()->report("prepare patch file error:" + result, reqPro.appname, sServerName, reqPro.nodename);
	        return EM_TARS_PREPARE_ERR;
	    }

       reqPro.md5 = md5;

	   return NodeManager::getInstance()->patchPro(req, result, current);
   }
   catch(TarsSyncCallTimeoutException& ex)
   {
       current->setResponse(true);
       result = ex.what();

       iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
       TLOG_ERROR("AdminRegistryImp::batchPatch " << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|TarsSyncCallTimeoutException:" << result << endl);
       RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);

   }
   catch(TarsNodeNotRegistryException& ex)
   {
       current->setResponse(true);
       result = ex.what();
       iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
       TLOG_ERROR("AdminRegistryImp::batchPatch "<< reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|TarsNodeNotRegistryException:" << result << endl);
       RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);
   }
   catch (std::exception & ex)
   {
       current->setResponse(true);
       result = ex.what();
       iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
       TLOG_ERROR("AdminRegistryImp::batchPatch "<< reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|exception:" << result << endl);
       RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);
   }
   catch (...)
   {
       current->setResponse(true);
       result = "Unknown Exception";
       TLOG_ERROR( "|" << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|ret." << iRet << "|Exception...:" << result << endl);
       RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);

	}
   return iRet;

}

int AdminRegistryImp::batchPatch_inner(const tars::PatchRequest & req, string &result)
{
    tars::PatchRequest reqPro = req;
    reqPro.patchobj = (*g_pconf)["/tars/objname<patchServerObj>"];
	reqPro.servertype = getServerType(req.appname, req.servername, req.nodename);

    int iRet = 0;
    string sServerName;

    try
    {
        //让tarspatch准备发布包
        sServerName  = reqPro.groupname.empty() ? reqPro.servername : reqPro.groupname;

        TLOG_DEBUG(reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << ", "
                 << reqPro.binname      << ", version:"
                 << reqPro.version      << ", user:"
                 << reqPro.user         << ", "
                 << reqPro.servertype   << ", "
                 << reqPro.patchobj     << ", md5:"
                 << reqPro.md5          << ", os:"
                 << reqPro.ostype       << ", serverName:"
                 << sServerName
                 << endl);

		iRet = NodeManager::getInstance()->patchPro(reqPro, result, NULL);

        deleteHistorys(reqPro.appname, sServerName);

		return iRet;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        result = ex.what();

        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        TLOG_ERROR("AdminRegistryImp::batchPatch " << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|TarsSyncCallTimeoutException:" << result << endl);
        RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);

    }
    catch(TarsNodeNotRegistryException& ex)
    {
        result = ex.what();
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TLOG_ERROR("AdminRegistryImp::batchPatch "<< reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|TarsNodeNotRegistryException:" << result << endl);
        RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);
    }
    catch (std::exception & ex)
    {
        result = ex.what();
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TLOG_ERROR("AdminRegistryImp::batchPatch "<< reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|exception:" << result << endl);
        RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);
    }
    catch (...)
    {
        result = "Unknown Exception";
        TLOG_ERROR( "|" << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|ret." << iRet << "|Exception...:" << result << endl);
        RemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);

	}

	TLOG_ERROR(reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << ", patchPro ret: " << iRet << ", result:" << result << endl);

    return iRet;

}

int AdminRegistryImp::updatePatchLog(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ, tars::CurrentPtr current)
{
	return updatePatchLog_inner(application, serverName, nodeName, patchId, user, patchType, succ);
}
int AdminRegistryImp::updatePatchLog_inner(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ)
{
	return DBPROXY->updatePatchByPatchId(application, serverName, nodeName, patchId, user, patchType, succ);
}

int AdminRegistryImp::getPatchPercent( const string& application, const string& serverName,  const string & nodeName, PatchInfo &tPatchInfo, CurrentPtr current)
{
    int iRet = EM_TARS_UNKNOWN_ERR;
    string &result = tPatchInfo.sResult;
    try
    {
        TLOG_DEBUG( "AdminRegistryImp::getPatchPercent: " + application  + "." + serverName + "_" + nodeName
                << "|caller: " << current->getHostName()  << ":" << current->getPort() <<endl);

		return NodeManager::getInstance()->getPatchPercent(application, serverName, nodeName, result, current);
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
	    result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
        TLOG_ERROR(result << endl);
	    RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
	    result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
	    RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
        TLOG_ERROR(result << endl);
    }
    catch(TarsException & ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_UNKNOWN_ERR;
	    result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
	    RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
	    TLOG_ERROR(result << endl);
    }
    return iRet;

}


int AdminRegistryImp::getPatchPercent_inner(const string &application, const string &serverName, const string & nodeName, PatchInfo &tPatchInfo)
{
	int iRet = EM_TARS_UNKNOWN_ERR;
	string result;
	try
	{
		iRet = NodeManager::getInstance()->getPatchPercent(application, serverName, nodeName, result, NULL);
		TLOG_DEBUG(application + "." + serverName + "_" + nodeName << ", iRet:" << iRet << ", size:" << result.size() << endl);

		if(iRet == EM_TARS_SUCCESS)
		{
			TarsInputStream<> is;
			is.setBuffer(result.c_str(), result.length());
			tPatchInfo.readFrom(is);

			TLOG_DEBUG(application + "." + serverName + "_" + nodeName << " " << tPatchInfo.writeToJsonString() << endl);

		}
		else
		{
			TLOG_ERROR(application + "." + serverName + "_" + nodeName << ", iRet:" << iRet << ", result:" << result << endl);
			tPatchInfo.sResult = "getPatchPercent " + nodeName + " error, iRet:" + TC_Common::tostr(iRet);
		}
	}
	catch (TarsSyncCallTimeoutException& ex)
	{
		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOG_ERROR(result << endl);
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
	}
	catch (TarsNodeNotRegistryException& ex)
	{
		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOG_ERROR(result << endl);
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
	}
	catch (TarsException & ex)
	{
		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOG_ERROR(result << endl);
		iRet = EM_TARS_UNKNOWN_ERR;
	}
	return iRet;
}

tars::Int32 AdminRegistryImp::delCache(const string &nodeName, const std::string & sFullCacheName, const std::string &sBackupPath, const std::string & sKey, std::string &result,CurrentPtr current)
{
	int iRet = EM_TARS_UNKNOWN_ERR;
	try
	{
		TLOG_DEBUG( "AdminRegistryImp::delCache: " + sFullCacheName  + "." + sBackupPath + "_" + sKey
				<< "|caller: " << current->getHostName()  << ":" << current->getPort() <<endl);

		return NodeManager::getInstance()->delCache(nodeName, sFullCacheName, sBackupPath, sKey, result, current);
	}
	catch(TarsSyncCallTimeoutException& ex)
	{
		current->setResponse(true);
//		result = "delCache: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
		TLOG_ERROR(result << endl);
//		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
	}
	catch(TarsNodeNotRegistryException& ex)
	{
		current->setResponse(true);
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
//		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
//		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOG_ERROR(result << endl);
	}
	catch(TarsException & ex)
	{
		current->setResponse(true);
		iRet = EM_TARS_UNKNOWN_ERR;
//		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
//		RemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOG_ERROR(result << endl);
	}
	return iRet;
}

int AdminRegistryImp::getLogData(const std::string & application, const std::string & serverName, const std::string & nodeName, const std::string & logFile, const std::string & cmd, std::string &fileData,CurrentPtr current)
{
    string result = "succ";
    try
    {
        TLOG_DEBUG("into " << __FUNCTION__ << endl);
		string nodeIp = nodeName;
		map<string, string> context;
		if (nodeName.empty() || nodeName == "remote")
		{
			nodeIp = _remoteLogIp;
			context["pathFlag"] = "remote_app_log";
		}
		else
		{
			context["pathFlag"] = "app_log";
		}

		if (nodeIp.empty())
		{
			return EM_TARS_PARAMETER_ERR;
		}

		string result;
		return NodeManager::getInstance()->getLogData(application, serverName, nodeName, logFile, cmd, context, result, current);
    }
    catch (TarsSyncCallTimeoutException& tex)
    {
        TLOG_ERROR( "|" << application + "." + serverName + "_" + nodeName << "|Exception:" << tex.what() << endl);
        //result = tex.what();
        return EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch (TarsNodeNotRegistryException& rex)
    {
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' exception:" + rex.what();
        TLOG_ERROR(result << endl);
        return EM_TARS_NODE_NOT_REGISTRY_ERR;
    }
    catch (exception & ex)
    {
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' exception:" + ex.what();
        TLOG_ERROR(result << endl);
        return EM_TARS_UNKNOWN_ERR;
    }
    return -1;
}


int AdminRegistryImp::getLogFileList(const std::string & application,const std::string & serverName,const std::string & nodeName,vector<std::string> &logFileList,tars::CurrentPtr current)
{
	string result = "succ";
   try
    {
        TLOG_DEBUG("into " << __FUNCTION__ << endl);
		string nodeIp = nodeName;
		map<string, string> context;
		if (nodeName.empty() || nodeName == "remote")
		{
			nodeIp = _remoteLogIp;
			context["pathFlag"] = "remote_app_log";
		}
		else
		{
			context["pathFlag"] = "app_log";
		}

		if (nodeIp.empty())
		{
            TLOG_ERROR("no nodeIp error." << endl);
			return EM_TARS_PARAMETER_ERR;
		}

		string result;

		return NodeManager::getInstance()->getLogFileList(application, serverName, nodeName, context, result, current);
    }
    catch (TarsSyncCallTimeoutException& tex)
    {
        TLOG_ERROR( "|" << application + "." + serverName + "_" + nodeName << "|Exception:" << tex.what() << endl);
        result = tex.what();
        return EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch (TarsNodeNotRegistryException& rex)
    {
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' exception:" + rex.what();
        TLOG_ERROR(result << endl);
        return EM_TARS_NODE_NOT_REGISTRY_ERR;
    }
    catch (TarsException & ex)
    {
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' exception:" + ex.what();
        TLOG_ERROR(result << endl);
        return EM_TARS_UNKNOWN_ERR;
    }

    return -1;
}

string AdminRegistryImp::getRemoteLogIp(const string& serverIp)
{
    try
    {
        vector<TC_Endpoint> ep = Application::getCommunicator()->getEndpoint(_remoteLogObj);
        if (ep.size() > 0)
        {
            return ep[0].getHost();
        }
        return "";    
    }
    catch(const std::exception& e)
    {
        TLOG_ERROR(e.what() << '\n');
    }
    return "";
}

int AdminRegistryImp::getNodeLoad(const string& application, const string& serverName, const std::string & nodeName, int pid, string& fileData, tars::CurrentPtr current)
{
	string result = "succ";
	try
	{
		TLOG_DEBUG("into " << __FUNCTION__ << endl);
		return NodeManager::getInstance()->getNodeLoad(application, serverName, nodeName, pid, result, current);
//
//		NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
//		return nodePrx->getNodeLoad(application, serverName, pid, fileData);
	}
	catch (TarsSyncCallTimeoutException& tex)
	{
		TLOG_ERROR("|" << application + "." + serverName + "_" + nodeName << "|Exception:" << tex.what() << endl);
		//result = tex.what();
		return EM_TARS_CALL_NODE_TIMEOUT_ERR;
	}
	catch (TarsNodeNotRegistryException& rex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' exception:" + rex.what();
		TLOG_ERROR(result << endl);
		return EM_TARS_NODE_NOT_REGISTRY_ERR;
	}
	catch (TarsException & ex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' exception:" + ex.what();
		TLOG_ERROR(result << endl);
		return EM_TARS_UNKNOWN_ERR;
	}

	return -1;
}


string AdminRegistryImp::getServerType(const std::string & application, const std::string & serverName, const std::string & nodeName)
{
	vector<ServerDescriptor> server;
	server = DBPROXY->getServers(application, serverName, nodeName, true);
	if (server.size() == 0)
	{
		TLOG_ERROR("|" << " '" + application + "." + serverName + "_" + nodeName + "' no config" << endl);

		return "";
	}

	return server[0].serverType;
}

int AdminRegistryImp::loadServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::CurrentPtr current)
{
    try
    {
        TLOG_DEBUG("AdminRegistryImp::loadServer enter"<<endl);
		return NodeManager::getInstance()->loadServer(application, serverName, nodeName, result, current);

//		NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
//        return nodePrx->loadServer(application, serverName, result);
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        TLOG_ERROR("AdminRegistryImp::loadServer: "<< application + "." + serverName + "_" + nodeName << "|Exception:" << ex.what() << endl);
        RemoteNotify::getInstance()->report(string("loadServer:") + ex.what(), application, serverName, nodeName);
        return EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch(TarsNodeNotRegistryException& rex)
    {
        TLOG_ERROR("AdminRegistryImp::loadServer: '"<<(" '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + rex.what())<<endl);
        RemoteNotify::getInstance()->report(string("loadServer:") + rex.what(), application, serverName, nodeName);
        return EM_TARS_NODE_NOT_REGISTRY_ERR;
    }
    catch(TarsException & ex)
    {
        TLOG_ERROR("AdminRegistryImp::loadServer: '"<<(" '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + ex.what())<<endl);
        RemoteNotify::getInstance()->report(string("loadServer:") + ex.what(), application, serverName, nodeName);
        return EM_TARS_UNKNOWN_ERR;
    }

}

int AdminRegistryImp::getProfileTemplate(const std::string & profileName,std::string &profileTemplate, std::string & resultDesc, tars::CurrentPtr current)
{
    profileTemplate = DBPROXY->getProfileTemplate(profileName, resultDesc);

    TLOG_DEBUG("AdminRegistryImp::getProfileTemplate get " << profileName  << " return length:"<< profileTemplate.size() << endl);
    return 0;
}

int AdminRegistryImp::getServerProfileTemplate(const string & application, const string & serverName, const string & nodeName,std::string &profileTemplate, std::string & resultDesc, tars::CurrentPtr current)
{
    TLOG_DEBUG("AdminRegistryImp::getServerProfileTemplate get " << application<<"."<<serverName<<"_"<<nodeName<< endl);

    int iRet =  -1;
    try
    {
        if(application != "" && serverName != "" && nodeName != "")
        {
            vector<ServerDescriptor> server;
            server = DBPROXY->getServers(application, serverName, nodeName, true);
            if(server.size() > 0)
            {
                profileTemplate = server[0].profile;
                iRet = 0;
            }
        }
        TLOG_DEBUG( "AdminRegistryImp::getServerProfileTemplate get " << application<<"."<<serverName<<"_"<<nodeName
            << " ret:"<<iRet<<" return length:"<< profileTemplate.size() << endl);
        return iRet;
    }
    catch(exception & ex)
    {
        resultDesc = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + ex.what();
        TLOG_ERROR(resultDesc << endl);
	    RemoteNotify::getInstance()->report(resultDesc, application, serverName, nodeName);

    }
    return iRet;
}

int AdminRegistryImp::getClientIp(std::string &sClientIp,tars::CurrentPtr current)
{
    sClientIp = current->getHostName();
    return 0;
}

int AdminRegistryImp::prepareInfo_inner(PrepareInfo &pi, string &result)
{
	//获取patch包的文件信息和md5值
	int iRet = DBPROXY->getInfoByPatchId(pi.patchId, pi.patchFile, pi.md5);
	if (iRet != 0) {
		result = "load patch info from database error, patchId:" + pi.patchId;
		TLOG_ERROR(result << endl);
		return EM_TARS_GET_PATCH_FILE_ERR;
	}

	return EM_TARS_SUCCESS;
}

int AdminRegistryImp::preparePatch_inner(const PrepareInfo &pi, string &result)
{
    try
    {
        int timeout = TC_Common::strto<int>(g_pconf->get("/tars/nodeinfo<batchpatch_node_timeout>", "10000"));
        int iRet = _patchPrx->tars_set_timeout(timeout)->preparePatchFile(pi.application, pi.serverName, pi.patchFile, result);
        if (iRet != 0)
        {
            result = "tarspatch preparePatchFile error:" + result;
            TLOG_ERROR(result << endl);
            return EM_TARS_PREPARE_ERR;
        }
        deleteHistorys(pi.application, pi.serverName);
    }
    catch (exception &ex)
    {
        result = string("tarspatch preparePatchFile error:") + ex.what();
        TLOG_ERROR(pi.application << "." << pi.serverName << ", exception:" << ex.what() << endl);
        return EM_TARS_UNKNOWN_ERR;
    }

	return EM_TARS_SUCCESS;
}

int AdminRegistryImp::deletePatchFile(const string &application, const string &serverName, const string & patchFile, tars::CurrentPtr current)
{
	TLOG_DEBUG(__FUNCTION__ << ":" << application << ", " << serverName << ", " << patchFile << endl);

	_patchPrx->async_deletePatchFile(NULL, application, serverName, patchFile);

	return 0;
}

int AdminRegistryImp::getServers(vector<tars::FrameworkServer> &servers, tars::CurrentPtr current)
{
	TLOG_DEBUG(__FUNCTION__ << endl);

	int ret = DBPROXY->getFramework(servers);

	if(ret != 0)
	{
		return -1;
	}

	return 0;
}

int AdminRegistryImp::getVersion(string &version, tars::CurrentPtr current)
{
	TLOG_DEBUG(__FUNCTION__ << "version:" << FRAMEWORK_VERSION << endl);

    version = FRAMEWORK_VERSION;
    return 0;
}

int AdminRegistryImp::checkServer(const FrameworkServer &server, tars::CurrentPtr current)
{
	TLOG_DEBUG(__FUNCTION__ << ", " << server.objName << endl);

	ServantPrx prx = Application::getCommunicator()->stringToProxy<ServantPrx>(server.objName);

	try
	{
		prx->tars_ping();
	}
	catch(exception &ex)
	{
		TLOG_ERROR(__FUNCTION__ << ", ping: " << server.objName << ", failed:" << ex.what() << endl);
		return -1;
	}

	return 0;
}


int AdminRegistryImp::updateServerFlowState(const string& application, const string& serverName, const vector<string>& nodeList, bool bActive, CurrentPtr current)
{
	TLOG_DEBUG("" << endl);

	int ret = DBPROXY->updateServerFlowState(application, serverName, nodeList, bActive);

	if(ret < 0)
	{
		return -1;
	}

	return 0;
}

int AdminRegistryImp::forceDockerLogin(const string &nodeName, vector<string> &result, CurrentPtr current)
{
	TLOG_DEBUG(nodeName << endl);
	try
	{
		string buff;
		return NodeManager::getInstance()->forceDockerLogin( nodeName, buff, current);
	}
	catch (exception & ex)
	{
		result.push_back(ex.what());
		TLOG_ERROR(ex.what() << endl);
		return EM_TARS_UNKNOWN_ERR;
	}

	return -1;
}

int AdminRegistryImp::checkDockerRegistry(const string & registry, const string & userName, const string & password, string &result, CurrentPtr current)
{
	TLOG_DEBUG(registry << ", " << userName << endl);
	try
	{
		TC_Docker docker;
		docker.setDockerUnixLocal(_dockerSocket);
		docker.setRequestTimeout(80000);

		bool succ = docker.login(userName, password, registry);

		result = succ?docker.getResponseMessage():docker.getErrMessage();

		TLOG_DEBUG(registry << ", " << userName << ", " << result << endl);

		return 0;
	}
	catch (exception & ex)
	{
		result = ex.what();
		TLOG_ERROR(ex.what() << endl);
		return EM_TARS_UNKNOWN_ERR;
	}

	return -1;
}

int AdminRegistryImp::dockerPull(const string & baseImageId, CurrentPtr current)
{
	TLOG_DEBUG("base image id: " << baseImageId << endl);
	try
	{
		return _registryPrx->dockerPull(baseImageId);
	}
	catch (exception & ex)
	{
		TLOG_ERROR(ex.what() << endl);
		return EM_TARS_UNKNOWN_ERR;
	}

	return -1;
}

int AdminRegistryImp::getNodeList(const vector<string> &nodeNames, map<string, string> &heartbeats, CurrentPtr current)
{
	auto data = NodeManager::getInstance()->getNodeList();

	for(auto nodeName : nodeNames)
	{
		heartbeats[nodeName] = data[nodeName].timeStr;
	}
	return 0;
}

