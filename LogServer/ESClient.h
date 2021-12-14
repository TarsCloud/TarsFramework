
#pragma once

#include "servant/Application.h"
#include "util/tc_http.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

using namespace tars;

enum class ESClientRequestMethod
{
    Put,
    Post,
    Get,
};

class ESClient
{
  public:
    static ESClient &instance()
    {
        static ESClient esClient;
        return esClient;
    }

    void setServer(const string &proto, const std::string &host, int port)
    {
        string es;
        if ("http" == proto)
        {
            es = "es@tcp -h " + host + " -p " + TC_Common::tostr(port);
        }
        else
        {
            es = "es@ssl -h " + host + " -p " + TC_Common::tostr(port);
        }
        _host = host;
        TLOG_ERROR("init es server:" << es << endl);
        _esPrx = Application::getCommunicator()->stringToProxy<ServantPrx>(es);
        _esPrx->tars_set_protocol(ServantProxy::PROTOCOL_HTTP1, 5);
    }

    void setServer(const std::string &host, int port)
    {
        string es = "es@tcp -h " + host + " -p " + TC_Common::tostr(port);
        _host = host;
        _esPrx = Application::getCommunicator()->stringToProxy<ServantPrx>(es);
        _esPrx->tars_set_protocol(ServantProxy::PROTOCOL_HTTP1, 5);
    }

    void setServer(const std::string &esPre)
    {
        string es = "es@";
        TC_URL url;
        url.parseURL(esPre);
        if (url.getType() == TC_URL::HTTPS)
        {
            es += "ssl";
        }
        else
        {
            es += "tcp";
        }
        es += " -h " + url.getDomain() + " -p " + url.getPort();
        _host = url.getDomain();
        _esPrx = Application::getCommunicator()->stringToProxy<ServantPrx>(es);
        _esPrx->tars_set_protocol(ServantProxy::PROTOCOL_HTTP1, 5);
    }

    int postRequest(ESClientRequestMethod method, const std::string &url, const std::string &body, std::string &response);

  private:
    ESClient() = default;

  private:
    ServantPrx _esPrx;
    string     _host;
};