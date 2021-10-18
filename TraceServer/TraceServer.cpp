#include <thread>
#include "TraceServer.h"
#include "ESClient.h"
#include "ESIndex.h"
#include "util/tc_file.h"
#include "ESWriter.h"
#include "TopologyImp.h"
#include "TimerTaskQueue.h"
#include "servant/RemoteLogger.h"

[[noreturn]] void TraceServer::watchFile() {
    while (true) {
        auto todayLogFile = buildLogFileName(false);
        auto absTodayLogFile = buildLogFileName(true);
        if (TC_File::isFileExist(absTodayLogFile)) {
            onModify(todayLogFile);
        }
        sleep(1);
    }
}

void TraceServer::initialize() {

    auto &&config = getConfig();

    string message{};
    if (!loadTimerValue(config, message)) {
        throw std::runtime_error("bad config: " + message);
    }


    logDir_ = config.get("/tars/trace<log_dir>", "/usr/local/app/tars/remote_app_log/_tars_/_trace_");

    std::vector<std::string> esNodes = config.getDomainKey("/tars/trace/es_nodes");

    if (esNodes.empty()) {
        TLOGERROR("iInitialize with empty es nodes, please configure es addr in server template." << endl);
        TARS_NOTIFY_ERROR("Initialize with empty es nodes, please configure es addr in server template.");
        exit(0);
    }
    
    ESClient::instance().setServer(esNodes[0]);

    auto todayLogFile = buildLogFileName(false);
    auto absTodayLogFile = buildLogFileName(true);
    if (TC_File::isFileExist(absTodayLogFile)) {
        onModify(todayLogFile);
    }

    ESWriter::createIndexTemplate();

    timerThread_ = std::thread([] {
        TimerTaskQueue::instance().run();
    });
    timerThread_.detach();

    watchThread_ = std::thread([this]() {
        watchFile();
    });
    watchThread_.detach();

    addServant<TopologyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".TopologyObj");
};

void TraceServer::destroyApp() {
    usleep(1000);
}

void TraceServer::onModify(const string &file) {
    auto iterator = pairs_.find(file);
    if (iterator != pairs_.end()) {
        auto pair = iterator->second;
        while (true) {
            auto &&rs = pair->logReader.read();
            if (rs.empty()) {
                break;
            }
            TLOGDEBUG("read " << rs.size() << " lines " << endl);
            ESWriter::postRawLog(file, rs);
            for (auto &&r: rs) {
                pair->aggregation->pushRawLog(r);
            }
        }
        return;
    }

    if (isExpectedFile(file)) {
        auto absFileLog = getAbsLogFileName(file);
        auto pair = std::make_shared<LogReadAggregationPair>(absFileLog);
        pair->aggregation->setTimer(firstCheckTimer, checkCycleTimer, closureOvertime, maxOvertime);
        pairs_[file] = pair;
        while (true) {
            auto &&rs = pair->logReader.read();
            if (rs.empty()) {
                break;
            }
            TLOGDEBUG("read " << rs.size() << " lines " << endl);
            ESWriter::postRawLog(file, rs);
            for (auto &&r: rs) {
                pair->aggregation->pushRawLog(r);
            }
        }

        /*
            we only monitor one file at the same time,
            if the current file is expected,means that other LogReadAggregationPair need to be deleted.
        */
        for (auto it = pairs_.begin(); it != pairs_.end();) {
            if (it->first == file) {
                it++;
                continue;
            }
            auto ptr = it->second;
            TimerTaskQueue::instance().pushTimerTask([ptr] {
                /*
                 *  ptr will release after 30 minutes.
                 */
            }, 60 * 30);
            pairs_.erase(it++);
        }
    }
}