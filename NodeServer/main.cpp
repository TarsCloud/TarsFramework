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

#include "NodeServer.h"
#include "servant/Communicator.h"
#include "util/tc_platform.h"
#include "util/tc_option.h"
#include "util/tc_file.h"
#include "util/tc_config.h"
#include <iostream>

using namespace std;
using namespace tars;

NodeServer g_app;
TC_Config* g_pconf;


void monitorNode(const string &configFile)
{
    TC_Config conf;
    conf.parseFile(configFile);
    CommunicatorPtr c = new Communicator();

    string serverObj = "tars.tarsnode.ServerObj@" + conf["/tars/application/server/ServerAdapter<endpoint>"];
    ServerFPrx sprx = c->stringToProxy<ServerFPrx>(serverObj);
    unsigned int latestKeepAliveTime = sprx->tars_set_timeout(2000)->getLatestKeepAliveTime();
    unsigned int kaTimeout = TC_Common::strto<unsigned int>(conf.get("/tars/node/keepalive<synTimeout>", "300"));
    if (latestKeepAliveTime + kaTimeout < TNOW)
    {
        cerr << "MonitorNode fail, keepalive timeout! LatestKeepAliveTime:" << latestKeepAliveTime << ", kaTimeout:" << kaTimeout << ", now:" << TNOW << endl;
        throw TC_Exception("KeepAlive Timeout", -1);
    }
    else
    {
//        cerr << "MonitorNode ok, latestKeepAliveTime:" << latestKeepAliveTime << ", kaTimeout:" << kaTimeout << endl;
    }

}

void parseConfig(int argc, char *argv[])
{
    TC_Option tOp;
    tOp.decode(argc, argv);

    if (tOp.hasParam("version"))
    {
        cout << "TARS:" << Application::getTarsVersion() << endl;
        exit(0);
    }
    
    if (tOp.hasParam("nodeversion"))
    {
        cout << "Node:" << Application::getTarsVersion() << "_" << NODE_VERSION << endl;
        exit(0);
    }


    if (tOp.hasParam("monitor"))
    {
        try
        {
            string configFile = tOp.getValue("config");

            monitorNode(configFile);
        }
        catch (exception &ex)
        {
            cout << "failed:" << ex.what() << endl;
            exit(-1);
        }
        exit(0);
    }

    string sNodeId          = tOp.getValue("nodeid");
    string sConfigFile      = tOp.getValue("config");

    cout << endl;
    cout << TC_Common::outfill("", '-') << endl;
    cout << TC_Common::outfill("nodeid", ' ', 10) << sNodeId << endl;
	cout << TC_Common::outfill("config", ' ', 10) << sConfigFile << endl;
	if(sConfigFile == "")
    {
        cerr << endl;
        cerr <<"start server with config, for example: "<<endl;
        cerr << argv[0] << " --config=config.conf --nodeid=172.25.38.67" << endl;
        cerr << argv[0] << " --config=config.conf" << endl;
        cout << TC_Common::outfill("", '-') << endl;
        cerr << argv[0] << " --version  for view tars-version" << endl;
        cerr << argv[0] << " --nodeversion  for view tarsnode-version" << endl;
        exit(0);
    }

    if(sNodeId == "localip.tars.com")
    {
        sNodeId = "";
    }

	NodeServer::onUpdateConfig(sNodeId, sConfigFile, true);
}

int main( int argc, char* argv[] )
{
    try
    {
        bool bNoDaemon = false;
        for (int i = 1; i < argc; ++i)
        {
            if (::strcmp(argv[i], "--monitor") == 0)
            {
                bNoDaemon = true;
                break;
            }
        }
#if !TARGET_PLATFORM_WINDOWS
        if (!bNoDaemon)
        {
            TC_Common::daemon();
        }
#endif
        parseConfig(argc,argv);

        g_pconf = &g_app.getConfig();

        g_app.main( argc, argv );
        g_app.waitForShutdown();
    }
    catch ( exception& ex )
    {
        cout<< ex.what() << endl;
    }
    catch ( ... )
    {
        cout<< "main unknow exception cathed" << endl;
    }

    return 0;
}
