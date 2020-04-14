#ifndef CMD_PING
#define CMD_PING

#include "util/tc_option.h"
#include "TarsClient.h"

using namespace tars;

void ping(TarsClient* tarsClient, TC_Option& option)
{
    string obj = option.getValue("obj");

    cout << "ping:" << obj << endl;

    try
    {
        ServantPrx prx = tarsClient->getCommunicator()->stringToProxy<ServantPrx>(obj);
        prx->tars_ping();
    }
    catch(exception &ex)
    {
        cout << "ping err:" << ex.what() << endl;
    }
}

#endif 