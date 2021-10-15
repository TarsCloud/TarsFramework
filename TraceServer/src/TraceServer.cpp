#include <thread>
#include "TraceServer.h"
#include "ESClient.h"
#include "ESIndex.h"
#include "util/tc_file.h"
#include "ESWriter.h"
#include "TopologyImp.h"
#include "TimerTaskQueue.h"
#include "servant/RemoteLogger.h"

[[noreturn]] static void
watchDir(const std::string &dir, const std::function<void(const inotify_event *)> &callBack) {
    int inotifyFd_ = inotify_init1(IN_CLOEXEC);
    if (inotifyFd_ == -1) {
        throw std::runtime_error("call inotify_init error");
    }
    uint32_t INOTIFY_MASK = IN_MODIFY;
    int res = inotify_add_watch(inotifyFd_, dir.c_str(), INOTIFY_MASK);
    if (res == -1) {
        throw std::runtime_error("call inotify_add_watch error");
    }
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
    char buffer[EVENT_BUF_LEN];
    while (true) {
        ssize_t length = ::read(inotifyFd_, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
        }
        size_t pos = 0;
        while (pos < length) {
            auto *event = (struct inotify_event *) &buffer[pos];
            callBack(event);
            pos += EVENT_SIZE + event->len;
        }
    }
}

void TraceServer::initialize() {

    auto &&config = getConfig();

    string message{};
    if (!loadTimerValue(config, message)) {
        throw std::runtime_error("bad config: " + message);
    }

//    addAdminCommandNormal("reload", [this](const string &command, const string &params, string &result) -> bool {
//        parseConfig(ServerConfig::ConfigFile);
//        auto &&config = getConfig();
//        return loadTimerValue(config, result);
//    });

    logDir_ = config.get("/tars/trace<log_dir>", "/usr/local/app/tars/remote_app_log/_tars_/_trace_");

    std::vector<std::string> esNodes = config.getDomainKey("/tars/trace/es_nodes");

    if (esNodes.empty()) {
        TLOGERROR("initialize with empty es nodes " << endl);
        exit(0);
    }

    std::vector<std::tuple<std::string, int>> elkTupleNodes;
    for (auto &item: esNodes) {
        vector<string> vOneNode = TC_Common::sepstr<string>(item, ":", true);
        if (vOneNode.size() < 2) {
            TLOGERROR("initialize with wrong es nodes:" << item << endl);
            continue;
        }
        ESClient::setServer(vOneNode[0], std::stoi(vOneNode[1]));
        ESClient::instance().start();
        break;
    }

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

    watchThread_ = std::thread(watchDir, logDir_, [this](const inotify_event *event) {
        onINotify(event);
    });
    watchThread_.detach();

    addServant<TopologyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".TopologyObj");
};

void TraceServer::onINotify(const inotify_event *event) {
    if (event->len) {
        if (event->mask & IN_ISDIR) {
            return;
        }

        if (event->mask & IN_MODIFY) {
            onModify(event->name);
            return;
        }
    }
}

void TraceServer::destroyApp() {
    ESClient::instance().stop();
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