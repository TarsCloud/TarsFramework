
#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <queue>
#include <mutex>
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "util/tc_http.h"
#include "servant/Application.h"

using namespace tars;

enum class ESClientRequestMethod {
    Put,
    Post,
    Get,
};

class ESClient {
public:
    static ESClient &instance() {
        static ESClient esClient;
        return esClient;
    }

    void setServer(const std::string &host, int port) {
        string es = "es@tcp -h " + host + " -p " + TC_Common::tostr(port);
        _esPrx = Application::getCommunicator()->stringToProxy<ServantPrx>(es);
        _esPrx->tars_set_protocol(ServantProxy::PROTOCOL_HTTP1, 5);
    }

    int postRequest(ESClientRequestMethod method, const std::string &url, const std::string &body, std::string &response);

private:
    ESClient() = default;

private:
    ServantPrx _esPrx;
};