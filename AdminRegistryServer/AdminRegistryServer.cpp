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

#include "AdminRegistryServer.h"
#include "AdminRegistryImp.h"
#include "DbProxy.h"
#include "ExecuteTask.h"
#include "NodeManager.h"

TC_Config * g_pconf;
AdminRegistryServer g_app;

extern TC_Config * g_pconf;

//内部版本
const string SERVER_VERSION = "B003";

void AdminRegistryServer::initialize()
{
    TLOG_DEBUG("AdminRegistryServer::initialize..." << endl);

    try
    {
        extern TC_Config * g_pconf;
        string size = Application::getCommunicator()->getProperty("timeout-queue-size", "");
        if(size.empty())
        {
            Application::getCommunicator()->setProperty("timeout-queue-size","100");
        }

        loadServantEndpoint();

		DBPROXY->init(g_pconf);
        //轮询线程
        _reapThread.init();
        _reapThread.start();

        //供admin client访问的对象
        string adminObj = g_pconf->get("/tars/objname<AdminRegObjName>", "");
        if(adminObj != "")
        {
            addServant<AdminRegistryImp>(adminObj);
        }
		ExecuteTask::getInstance()->init(g_pconf);

		NodeManager::getInstance()->initialize();
		NodeManager::getInstance()->start();
    }
    catch(TC_Exception & ex)
    {
        TLOG_ERROR("RegistryServer initialize exception:" << ex.what() << endl);
        cerr << "RegistryServer initialize exception:" << ex.what() << endl;
        exit(-1);
    }

    TLOG_DEBUG("RegistryServer::initialize OK!" << endl);

	TARS_ADD_ADMIN_CMD_PREFIX("nodelist", AdminRegistryServer::cmdNodeList);
}

bool AdminRegistryServer::cmdNodeList(const string &command, const string &params, string &result)
{
	auto data = NodeManager::getInstance()->getNodeList();

	for(auto e: data)
	{
		result += e.first + "     " + e.second.timeStr + "\r\n";
	}
	return true;
}

int AdminRegistryServer::loadServantEndpoint()
{
    map<string, string> mapAdapterServant;
    mapAdapterServant = _servantHelper->getAdapterServant();

    map<string, string>::iterator iter;
    for(iter = mapAdapterServant.begin(); iter != mapAdapterServant.end(); iter++ )
    {
        TC_Endpoint ep = getEpollServer()->getBindAdapter(iter->first)->getEndpoint();

        _mapServantEndpoint[iter->second] = ep.toString();

        TLOG_DEBUG("registry obj: " << iter->second << " = " << ep.toString() <<endl);
    }

    return 0;
}

void AdminRegistryServer::destroyApp()
{
    _reapThread.terminate();
	NodeManager::getInstance()->terminate();
	NodeManager::getInstance()->join();

	TLOG_DEBUG("AdminRegistryServer::destroyApp ok" << endl);
}

void doMonitor(const string &configFile)
{
	TC_Config conf;
	conf.parseFile(configFile);

	string obj = "AdminObj@" + conf["/tars/application/server<local>"];
	ServantPrx prx = Application::getCommunicator()->stringToProxy<ServantPrx>(obj);
	prx->tars_ping();
}
void doCommand(int argc, char *argv[])
{
	TC_Option tOp;
	tOp.decode(argc, argv);
	if (tOp.hasParam("version"))
	{
		cout << "TARS:" << Application::getTarsVersion() << endl;
		exit(0);
	}
	if (tOp.hasParam("monitor"))
	{
		try
		{
			string configFile = tOp.getValue("config");
			doMonitor(configFile);
		}
		catch (exception &ex)
		{
			cout << "doMonitor failed:" << ex.what() << endl;
			exit(-1);
		}
		exit(0);
		return;
	}
}
int main(int argc, char *argv[])
{
    try
    {
		doCommand(argc, argv);
        g_pconf =  & g_app.getConfig();
        g_app.main(argc, argv);

        g_app.waitForShutdown();
    }
    catch(exception &ex)
    {
        cerr<< ex.what() << endl;
    }

    return 0;
}


