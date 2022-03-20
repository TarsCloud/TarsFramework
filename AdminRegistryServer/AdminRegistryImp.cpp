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

extern TC_Config * g_pconf;

void AdminRegistryImp::initialize()
{
    TLOG_DEBUG("begin AdminRegistryImp init"<<endl);

    _patchPrx = Application::getCommunicator()->stringToProxy<PatchPrx>(g_pconf->get("/tars/objname<patchServerObj>", "tars.tarspatch.PatchObj"));

//	_registryPrx = Application::getCommunicator()->stringToProxy<RegistryPrx>("/tars/objname<RegistryObjName>", "tars.tarsregistry.RegistryObj");

	int timeout = TC_Common::strto<int>(g_pconf->get("/tars/patch<patch_timeout>", "30000"));

	_patchPrx->tars_set_timeout(timeout);

	_remoteLogIp = g_pconf->get("/tars/log<remotelogip>", "");

    _remoteLogObj = g_pconf->get("/tars/log<remotelogobj>", "tars.tarslog.LogObj");

	_dockerSocket = g_pconf->get("/tars/container<socket>", "/var/run/docker.sock");
    TLOG_DEBUG("AdminRegistryImp init ok. _remoteLogIp:" << _remoteLogIp << ", _remoteLogObj:" << _remoteLogObj << endl);
}

int AdminRegistryImp::undeploy(const string & application, const string & serverName, const string & nodeName, const string &user, string &log, tars::CurrentPtr current)
{
	TLOG_DEBUG("application:" << application
		<< ", serverName:" << serverName
		<< ", nodeName:" << nodeName << endl);

	return undeploy_inner(application, serverName, nodeName, user, log);
}
    // string info;
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

int AdminRegistryImp::cancelTask(const string& taskNo, CurrentPtr current)
{
    TLOG_DEBUG("taskNo:" << taskNo << endl);
    return ExecuteTask::getInstance()->cancelTask(taskNo);
}
int AdminRegistryImp::getTaskRsp(const string &taskNo, TaskRsp &taskRsp, tars::CurrentPtr current)
{
    //优先从内存中获取
    bool ret = ExecuteTask::getInstance()->getTaskRsp(taskNo, taskRsp);
    if (ret)
    {
        // TLOG_DEBUG("AdminRegistryImp::getTaskRsp taskNo:" << taskNo << " from running time, ret:" << ret <<endl);
        return 0;
    }

//    TLOG_DEBUG("AdminRegistryImp::getTaskRsp taskNo:" << taskNo << " from db."<<endl);

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
        NodePrx nodePrx = DBPROXY->getNodePrx(name);
        
        int timeout = TC_Common::strto<int>(g_pconf->get("/tars/nodeinfo<ping_node_timeout>","3000"));
        nodePrx->tars_set_timeout(timeout)->tars_ping();

        result = "succ";
        TLOG_DEBUG("AdminRegistryImp::pingNode name:"<<name<<"|result:"<<result<<endl);
        return true;
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
        NodePrx nodePrx = DBPROXY->getNodePrx(name);
        return nodePrx->shutdown(result);
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

        //判断是否为dns 非dns才需要到node调用
        if(server[0].serverType == "tars_dns")
        {
            TLOG_DEBUG("AdminRegistryImp::getServerState " << ("'" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server") <<endl);
            state.presentStateInNode = server[0].presentState;
        }
        else
        {
            NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
            try
            {
                tars::ServerStateInfo info;
                current->setResponse(false);

                NodePrxCallbackPtr callback = new GetServerStateCallbackImp(nodePrx, application, serverName, nodeName, state, current);
                nodePrx->async_getStateInfo(callback, application, serverName);
            }
            catch(TarsException & e )
            {
                current->setResponse(true);
                string s = e.what();
                if(s.find("server function mismatch exception") != string::npos)
                {
                    ServerState stateInNode  = nodePrx->getState(application, serverName, result);
                    state.presentStateInNode = etos(stateInNode);
                    state.processId = nodePrx->getServerPid(application, serverName, result);
                }
                TLOG_ERROR("AdminRegistryImp::getServerState "<<"'" + application + "." + serverName + "_" + nodeName<<"|"<< e.what() <<endl);
            }
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
            NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
            TLOGINFO("call node into " << __FUNCTION__ << "|"
                << application << "." << serverName << "_" << nodeName << "|" << current->getHostName() << ":" << current->getPort() <<endl);

            current->setResponse(false);
            NodePrxCallbackPtr callback = new StartServerCallbackImp(application, serverName, nodeName, current);
            nodePrx->async_startServer(callback, application, serverName);
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
			NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
			TLOGINFO("call node into " << __FUNCTION__ << "|"
				<< application << "." << serverName << "_" << nodeName << endl);
			iRet = nodePrx->startServer(application, serverName, result);
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
            NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
            TLOGINFO("call node into " << __FUNCTION__ << "|"
                << application << "." << serverName << "_" << nodeName << "|" << current->getHostName() << ":" << current->getPort() <<endl);
            current->setResponse(false);
            NodePrxCallbackPtr callback = new StopServerCallbackImp(application, serverName, nodeName, current);
            nodePrx->async_stopServer(callback, application, serverName);
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
			NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
			TLOGINFO("call node into " << __FUNCTION__ << "|"
				<< application << "." << serverName << "_" << nodeName << endl);

			iRet = nodePrx->stopServer(application, serverName, result);		
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
				//马上要自己退出
				TC_Common::msleep(100);
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
				NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
				nodePrx->tars_timeout(12000);
				iRet = nodePrx->stopServer(application, serverName, result);
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
                NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);

                TLOG_DEBUG("call node restartServer(), start|" << application << "." << serverName << "_" << nodeName << "|" << current->getHostName() << ":" << current->getPort() << endl);

                return nodePrx->startServer(application, serverName, result);
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
	tarsErrCode iRet = EM_TARS_SUCCESS;
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
			NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
			nodePrx->tars_timeout(12000);
			iRet = (tarsErrCode)nodePrx->stopServer(application, serverName, result);
		}
		if(iRet != EM_TARS_SUCCESS)
		{
			RemoteNotify::getInstance()->report(string("restart server, stop error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
		}
		TLOG_DEBUG("stop " << application << "." << serverName << "_" << nodeName << ", " << etos(iRet) << endl);
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
				NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
				TLOG_DEBUG("start, " << application << "." << serverName << "_" << nodeName << endl);
				return nodePrx->startServer(application, serverName, result);
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
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
        current->setResponse(false);
        NodePrxCallbackPtr callback = new NotifyServerCallbackImp(application, serverName, nodeName, current);
        nodePrx->async_notifyServer(callback, application, serverName, command);
        return EM_TARS_SUCCESS;
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
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
        iRet = nodePrx->notifyServer(application, serverName, command, result);
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

       iRet = EM_TARS_UNKNOWN_ERR;
       int defaultTime = 3000;
       NodePrx proxy;

       proxy = DBPROXY->getNodePrx(reqPro.nodename);
       int timeout = TC_Common::strto<int>(g_pconf->get("/tars/nodeinfo<batchpatch_node_timeout>", "10000"));

       current->setResponse(false);
       NodePrxCallbackPtr callback = new PatchProCallbackImp(reqPro, proxy, defaultTime, current);
       proxy->tars_set_timeout(timeout)->async_patchPro(callback, reqPro);
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

		int timeout = TC_Common::strto<int>(g_pconf->get("/tars/nodeinfo<batchpatch_node_timeout>", "10000"));

	    iRet = EM_TARS_UNKNOWN_ERR;
		
        NodePrx proxy;

        proxy = DBPROXY->getNodePrx(reqPro.nodename);

		iRet = proxy->tars_set_timeout(timeout)->patchPro(reqPro, result);

		TLOG_DEBUG(reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << ", patchPro ret: " << iRet << ", result:" << result << endl);

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

        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);

        current->setResponse(false);
        NodePrxCallbackPtr callback = new GetPatchPercentCallbackImp(application, serverName, nodeName, current);
        nodePrx->async_getPatchPercent(callback, application, serverName);
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
	string &result = tPatchInfo.sResult;
	try
	{
		TLOG_DEBUG(application + "." + serverName + "_" + nodeName << endl);

		NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
		
		iRet = nodePrx->getPatchPercent(application, serverName, tPatchInfo);

		if(iRet != 0)
		{
			TLOG_ERROR(application + "." + serverName + "_" + nodeName << ", iRet:" << iRet << endl);
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
			return -2;
		}
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeIp);
        return nodePrx->getLogData(application, serverName, logFile, cmd, fileData, context);
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
			return -2;
		}

		TLOG_DEBUG("into " << __FUNCTION__ << "|" << application << "|" << serverName << "|" << nodeName << "|" << nodeIp << endl);
		NodePrx nodePrx = DBPROXY->getNodePrx(nodeIp);
        int ret = nodePrx->getLogFileList(application, serverName, logFileList, context);
        //TLOG_DEBUG("logFileList===>" + TC_Common::tostr(logFileList));
        return ret;
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

		NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
		return nodePrx->getNodeLoad(application, serverName, pid, fileData);
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
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
        return nodePrx->loadServer(application, serverName, result);
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
		NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
		return nodePrx->tars_set_timeout(10000)->forceDockerLogin(result);
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

/////////////////////////////////////////////////////////////////////////////
void PatchProCallbackImp::callback_patchPro(tars::Int32 ret,
       const std::string& result)
{
   TLOG_DEBUG("PatchProCallbackImp::callback_patchPro: |success ret."<<ret<<"|"
                << _reqPro.appname + "." + _reqPro.servername + "_" + _reqPro.nodename << "|"
                << _reqPro.binname      << "|"
                << _reqPro.version      << "|"
                << _reqPro.user         << "|"
                << _reqPro.servertype   << endl);

   _nodePrx->tars_timeout(_defaultTime);

   AdminReg::async_response_batchPatch(_current, ret, result);

}

void PatchProCallbackImp::callback_patchPro_exception(tars::Int32 ret)
{
   int iRet = EM_TARS_UNKNOWN_ERR;
   _nodePrx->tars_timeout(_defaultTime);

   if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
   {
       iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
   }

   RemoteNotify::getInstance()->report("call node to patch, timeout, ret:" + TC_Common::tostr(ret), _reqPro.appname, _reqPro.servername, _reqPro.nodename);

   AdminReg::async_response_batchPatch(_current, iRet, "");
   TLOG_DEBUG("PatchProCallbackImp::callback_patchPro_exception: |exception ret."<<ret<<"|"
                << _reqPro.appname + "." + _reqPro.servername + "_" + _reqPro.nodename << "|"
                << _reqPro.binname      << "|"
                << _reqPro.version      << "|"
                << _reqPro.user         << "|"
                << _reqPro.servertype   << endl);
}
/////////////////////////////////////////////////////////////////////////////
void StartServerCallbackImp::callback_startServer(tars::Int32 ret,
        const std::string& result)
{
    TLOG_DEBUG( _application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getHostName() << ":" << _current->getPort() << "|" << ret <<endl);

	if(ret != EM_TARS_SUCCESS)
	{
		RemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);
	}
    AdminReg::async_response_startServer(_current, ret, result);
}

void StartServerCallbackImp::callback_startServer_exception(tars::Int32 ret)
{
    TLOG_DEBUG(_application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getHostName() << ":" << _current->getPort() << "|" << ret <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }

	RemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);

    AdminReg::async_response_startServer(_current, iRet, "");
}
/////////////////////////////////////////////////////////////////////////////
void StopServerCallbackImp::callback_stopServer(tars::Int32 ret,
        const std::string& result)
{
    TLOG_DEBUG( _application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getHostName() << ":" << _current->getPort() << "|" << ret <<endl);

	if(ret != EM_TARS_SUCCESS)
	{
		RemoteNotify::getInstance()->report(string("stop server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);
	}

    AdminReg::async_response_stopServer(_current, ret, result);
}

void StopServerCallbackImp::callback_stopServer_exception(tars::Int32 ret)
{
    TLOG_DEBUG( _application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getHostName() << ":" << _current->getPort() << "|" << ret <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }

	RemoteNotify::getInstance()->report(string("stop server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);

    AdminReg::async_response_stopServer(_current, iRet, "");
}
/////////////////////////////////////////////////////////////////////////////
void NotifyServerCallbackImp::callback_notifyServer(tars::Int32 ret,  const std::string& result)
{
    TLOG_DEBUG(_current->getHostName() << ":" << _current->getPort() << "|" << ret  << "|" << result <<endl);
	if(ret != EM_TARS_SUCCESS)
	{
		RemoteNotify::getInstance()->report(string("notify server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);
	}

	AdminReg::async_response_notifyServer(_current, ret, result);
}

void NotifyServerCallbackImp::callback_notifyServer_exception(tars::Int32 ret)
{
    TLOG_DEBUG(_current->getHostName() << ":" << _current->getPort() << "|" << ret <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
	RemoteNotify::getInstance()->report(string("notify server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);

    AdminReg::async_response_notifyServer(_current, iRet, "");
}
/////////////////////////////////////////////////////////////////////////////
void GetServerStateCallbackImp::callback_getStateInfo(tars::Int32 ret,  const tars::ServerStateInfo& info,  const std::string& result)
{
    string resultRef = result;
    if(EM_TARS_SUCCESS == ret|| EM_TARS_UNKNOWN_ERR == ret)
    {
        _state.presentStateInNode = etos(info.serverState);
        _state.processId          = info.processId;
    }
    else
    {
        if(result.find("server function mismatch exception") != string::npos)
        {
            ServerState stateInNode   = _nodePrx->getState(_application, _serverName, resultRef);
            _state.presentStateInNode = etos(stateInNode);
            _state.processId = _nodePrx->getServerPid(_application, _serverName, resultRef);
            TLOG_WARN(_application + "." + _serverName + "_" + _nodeName<<"|"<< resultRef <<endl);
        }
    }
    AdminReg::async_response_getServerState(_current, EM_TARS_SUCCESS, _state, resultRef);
    TLOG_DEBUG("GetServerStateCallbackImp::callback_getStateInfo_exception " <<"|position2|"<<"'" + _application  + "." + _serverName + "_" + _nodeName<<"|"<< ret << "|" << resultRef <<endl);

}

void GetServerStateCallbackImp::callback_getStateInfo_exception(tars::Int32 ret)
{
    int iRet = EM_TARS_UNKNOWN_ERR;
    string result;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    else if(ret == tars::TARSSERVERNOFUNCERR)
    {
        iRet = EM_TARS_SUCCESS;
        ServerState stateInNode = _nodePrx->getState(_application, _serverName, result);
        _state.presentStateInNode = etos(stateInNode);
        _state.processId = _nodePrx->getServerPid(_application, _serverName, result);
        TLOG_ERROR(_application  + "." + _serverName + "_" + _nodeName<<"|"<< result <<endl);
    }
    AdminReg::async_response_getServerState(_current, iRet, _state, result);
    TLOG_ERROR( _application  + "." + _serverName + "_" + _nodeName<<"|"<< ret << "|" << iRet <<endl);
}
/////////////////////////////////////////////////////////////////////////////
void GetPatchPercentCallbackImp::callback_getPatchPercent(tars::Int32 ret,  const tars::PatchInfo& tPatchInfo)
{
    AdminReg::async_response_getPatchPercent(_current, ret, tPatchInfo);
    TLOG_DEBUG(_application + "." + _serverName + "_" + _nodeName<<"|"<< ret <<endl);
}

void GetPatchPercentCallbackImp::callback_getPatchPercent_exception(tars::Int32 ret)
{
    TLOG_ERROR(_application + "." + _serverName + "_" + _nodeName<<"|"<< ret <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    tars::PatchInfo tPatchInfo;
    AdminReg::async_response_getPatchPercent(_current, iRet, tPatchInfo);
}
