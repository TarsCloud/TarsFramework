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

#include <iostream>

#include "RegistryImp.h"
#include "RegistryProcThread.h"
#include "RegistryServer.h"
#include "NodeManager.h"

//初始化配置db连接
extern TC_Config * g_pconf;
extern RegistryServer g_app;

void RegistryImp::initialize()
{
    TLOG_DEBUG("begin RegistryImp init"<<endl);

    _db.init(g_pconf);

	_db.getFrameworkKey(_fKey);
    TLOG_DEBUG("RegistryImp init ok."<<endl);
}

int RegistryImp::doClose(CurrentPtr current)
{
	NodeManager::getInstance()->eraseNodeCurrent(current);

	return 0;
}

int RegistryImp::reportNode(const ReportNode &rn, CurrentPtr current)
{
	NodeManager::getInstance()->createNodeCurrent(rn.nodeName, current);
	return 0;
}

int RegistryImp::registerNode(const string & name, const NodeInfo & ni, const LoadInfo & li, CurrentPtr current)
{
    return _db.registerNode(name, ni, li);
}

int RegistryImp::keepAlive(const string & name, const LoadInfo & ni, CurrentPtr current)
{
    RegistryProcInfo procInfo;
    procInfo.nodeName = name;
    procInfo.loadinfo = ni;
    procInfo.cmd      = EM_NODE_KEEPALIVE;

    //放入异步处理线程中
    g_app.getRegProcThread()->put(procInfo);

    return 0;
}

vector<ServerDescriptor> RegistryImp::getServers(const std::string & app,const std::string & serverName,const std::string & nodeName,CurrentPtr current)
{
    return  _db.getServers(app, serverName, nodeName);
}

int RegistryImp::updateServer(const string & nodeName, const string & app, const string & serverName, const ServerStateInfo & stateInfo, CurrentPtr current)
{
    return _db.updateServerState(app, serverName, nodeName, "present_state", stateInfo.serverState, stateInfo.processId);
}

int RegistryImp::updateServerBatch(const std::vector<ServerStateInfo> & vecStateInfo, CurrentPtr current)
{
    return _db.updateServerStateBatch(vecStateInfo);
}

int RegistryImp::destroyNode(const string & name, CurrentPtr current)
{
    return _db.destroyNode(name);
}

int RegistryImp::reportVersion(const string & app, const string & serverName, const string & nodeName, const string & version, CurrentPtr current)
{
    RegistryProcInfo procInfo;
    procInfo.appName    = app;
    procInfo.serverName = serverName;
    procInfo.nodeName   = nodeName;
    procInfo.tarsVersion = version;
    procInfo.cmd        = EM_REPORTVERSION;

    //放入异步处理线程中
    g_app.getRegProcThread()->put(procInfo);

    return 0;
}

int RegistryImp::getNodeTemplate(const std::string & nodeName,std::string &profileTemplate,CurrentPtr current)
{
    string sTemplateName;
    int iRet = _db.getNodeTemplateName(nodeName, sTemplateName);
    if(iRet != 0 || sTemplateName == "")
    {
        //默认模板配置
        sTemplateName = (*g_pconf)["/tars/nodeinfo<defaultTemplate>"];
    }

    string sDesc;
    profileTemplate = _db.getProfileTemplate(sTemplateName, sDesc);

    TLOG_DEBUG(nodeName << " get sTemplateName:" << sTemplateName << " result:" << sDesc << endl);

    return 0;
}

int RegistryImp::getClientIp(std::string &sClientIp,CurrentPtr current)
{
    sClientIp = current->getIp();
    TLOG_DEBUG("RegistryImp::getClientIp ip: " << sClientIp <<  endl);

    return 0;
}


int RegistryImp::getGroupId(const string & ip, int &groupId, tars::CurrentPtr current)
{
	try
	{
		TLOG_DEBUG("RegistryImp::getGroupId ip: "<<ip<<endl);

		groupId= ObjectsCacheManager::getInstance()->getGroupId(ip);

		return 0;
	}
	catch(TarsException & ex)
	{
		TLOG_ERROR(("RegistryImp::getGroupId '" + ip + "' exception:" + ex.what())<< endl);
		return -1;
	}
}

int RegistryImp::updatePatchResult(const PatchResult & result, CurrentPtr current)
{
    TLOG_DEBUG( "RegistryImp::updatePatchResult " << result.sApplication + "." + result.sServerName + "_" + result.sNodeName << "|V:" << result.sVersion << "|U:" <<  result.sUserName << endl);

    return _db.setPatchInfo(result.sApplication, result.sServerName, result.sNodeName, result.sVersion, result.sUserName);
}

int RegistryImp::getDockerRegistry(vector<DockerRegistry> &doctorRegistries, CurrentPtr current)
{
	auto v = g_app.getDockerThread()->getDockerRegistry();

	for(auto e : v)
	{
		doctorRegistries.push_back(e.second);
	}

	TLOG_DEBUG("RegistryImp::getDockerRegistry registry size: " << doctorRegistries.size() <<  endl);

	return 0;
}

int RegistryImp::dockerPull(const string & baseImageId, CurrentPtr current)
{
	TLOGEX_DEBUG("DockerThread", "RegistryImp::dockerPull baseImageId: " << baseImageId <<  endl);

	return g_app.getDockerThread()->dockerPull(baseImageId);
}

int RegistryImp::getFrameworkKey(FrameworkKey &fKey,  CurrentPtr current)
{
	fKey = _fKey;
	return 0;
}