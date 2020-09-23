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

#include "RegistryServer.h"
#include <iostream>

using namespace tars;

RegistryServer g_app;
TC_Config * g_pconf;

void doMonitor(const string &configFile)
{
	TC_Config conf;
	conf.parseFile(configFile);

	string obj = "AdminObj@" + conf["/tars/application/server<local>"];

	ServantPrx prx = CommunicatorFactory::getInstance()->getCommunicator()->stringToProxy<ServantPrx>(obj);
	prx->tars_ping();
}

void doCommand(int argc, char *argv[])
{
	TC_Option tOp;
	tOp.decode(argc, argv);
	//直接输出编译的TAF版本
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


