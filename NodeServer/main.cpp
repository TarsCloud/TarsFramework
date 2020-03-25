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

    string nodeObj = "AdminObj@"+conf["/tars/application/server<local>"];

    ServantPrx prx = CommunicatorFactory::getInstance()->getCommunicator()->stringToProxy<ServantPrx>(nodeObj);
    prx->tars_set_timeout(500)->tars_ping();
}

void parseConfig(int argc, char *argv[])
{
    TC_Option tOp;
    tOp.decode(argc, argv);

     if (tOp.hasParam("nodeversion"))
    {
        cout << "Node:" << TARS_VERSION << "." << NODE_VERSION << endl;
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
        exit(0);
    }

    if(sNodeId == "localip.tars.com")
    {
        sNodeId = "";
    }

	NodeServer::onUpdateConfig(sNodeId, sConfigFile);

//
//    if(!TC_File::isAbsolute(sConfigFile))
//    {
//        char sCwd[PATH_MAX];
//        if ( getcwd( sCwd, PATH_MAX ) == NULL )
//        {
//            TLOGERROR("cannot get the current directory:\n" << endl);
//            exit( 0 );
//        }
//        sConfigFile = string(sCwd) +"/"+ sConfigFile;
//    }
//    sConfigFile = TC_File::simplifyDirectory(sConfigFile);
//    if(sLocator != "")
//    {
//        if(sRegistryObj == "")
//        {
//            sRegistryObj = "tars.tarsregistry.RegistryObj";
//        }
//        bool bCloseOut=true;
//        if(tOp.hasParam("closecout"))
//        {
//            if(TC_Common::lower(tOp.getValue("closecout"))=="n")
//            {
//                bCloseOut = false;
//            }
//        }
//        bool bRet = getConfig(sLocator,sRegistryObj,sNodeId, sLocalIp, sConfigFile, bCloseOut);
//        if(bRet == false)
//        {
//            cerr<<"reload config erro. use old config";
//        }
//    }
//    cout <<endl;
//    map<string,string> mOp = tOp.getMulti();
//    for(map<string,string>::const_iterator it = mOp.begin(); it != mOp.end(); ++it)
//    {
//        cout << outfill( it->first)<< it->second << endl;
//    }

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
//#if !TARGET_PLATFORM_WINDOWS
//        if (!bNoDaemon)
//        {
//            TC_Common::daemon();
//        }
//#endif
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
