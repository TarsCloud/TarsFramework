#ifndef CMD_PING
#define CMD_PING

#include "util/tc_option.h"
#include "TarsClient.h"

using namespace tars;

void start(TarsClient* tarsClient, TC_Option& option)
{
    string app = option.getValue("app");
    string serverName = option.getValue("server-name");
    string nodeName = option.getValue("node-name");

    cout << "start:" << app << "." << serverName << endl;

    AdminRegPrx adminPrx = tarsClient->getCommunicator()->stringToProxy<AdminRegPrx>("tars.tarsAdminRegistry.AdminRegObj");

    try
    {
        // if(nodeName.empty())
        // {
        //     adminPrx->getAllServerIds();
        // }

        // ServantPrx prx = tarsClient->getCommunicator()->stringToProxy<ServantPrx>(obj);
        // prx->tars_ping();
        cout << "start succ" << endl;
    }
    catch(exception &ex)
    {
        cout << "start err:" << ex.what() << endl;
    }
}

#endif 