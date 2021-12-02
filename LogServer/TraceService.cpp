#include "TraceService.h"
#include "ESClient.h"
#include "ESIndex.h"
#include "ESWriter.h"
#include "TimerTaskQueue.h"
#include "TopologyImp.h"
#include "servant/RemoteLogger.h"
#include "util/tc_common.h"
#include "util/tc_file.h"
#include <thread>

[[noreturn]] void TraceService::watchFile()
{
    while (true)
    {
        auto todayLogFile = buildLogFileName(false);
        auto absTodayLogFile = buildLogFileName(true);
        
        if (TC_File::isFileExist(absTodayLogFile))
        {
            //TLOG_DEBUG("absTodayLogFile:" << absTodayLogFile << endl);
            onModify(todayLogFile);
        }
        TC_Common::sleep(1);
    }
}

void TraceService::initialize(TC_Config &config)
{

    //auto &&config = getConfig();

    logDir_ = config.get("/tars/elk<log_dir>", "/usr/local/app/tars/remote_app_log/_tars_/_trace_");

    std::vector<std::string> esNodes = config.getDomainKey("/tars/elk/nodes");

    if (esNodes.empty())
    {
        TLOGERROR("Initialize with empty es nodes, please configure es addr in server template." << endl);
        TARS_NOTIFY_ERROR("Initialize with empty es nodes, if you want to open tarstrace, please configure es addr in server template.");
        //exit(0);
        return;
    }

    string proto = config.get("/tars/elk<protocol>", "http");
    if (proto != "http" && proto != "https")
    {
        TLOGERROR("Initialize with es protocol fail:" << proto << endl);
        TARS_NOTIFY_ERROR("Initialize es empty protocol fail:" + proto);
        exit(0);
    }
    vector<string> esOneNode = TC_Common::sepstr<string>(esNodes[0], ":", true);
    if (esOneNode.empty())
    {
        TLOGERROR("Initialize with es node fail:" << esNodes[0] << endl);
        TARS_NOTIFY_ERROR("Initialize with es node fail:" + esNodes[0]);
        exit(0);
    }
    
    if (esOneNode.size() == 1)
    {
        if(proto == "http") {
            esOneNode.push_back("80");
        }else if(proto == "https") {
            esOneNode.push_back("443");
        }
    }
    ESClient::instance().setServer(proto, esOneNode[0], TC_Common::strto<unsigned short>(esOneNode[1]));
    //ESClient::instance().setServer(esNodes[0]);

    string message{};
    if (!loadTimerValue(config, message))
    {
        throw std::runtime_error("bad config: " + message);
    }

    auto todayLogFile = buildLogFileName(false);
    auto absTodayLogFile = buildLogFileName(true);
    if (TC_File::isFileExist(absTodayLogFile))
    {
        onModify(todayLogFile);
    }

    ESWriter::createIndexTemplate();

    timerThread_ = std::thread([]
                               { TimerTaskQueue::instance().run(); });
    timerThread_.detach();

    watchThread_ = std::thread([this]()
                               { watchFile(); });
    watchThread_.detach();
};

// void TraceService::destroyApp() {
//     TC_Common::msleep(1);
// }

void TraceService::onModify(const string &file)
{
    //TLOG_DEBUG("file:" << file << endl);
    auto iterator = pairs_.find(file);
    if (iterator != pairs_.end())
    {
        auto pair = iterator->second;
        while (true)
        {
            auto &&rs = pair->logReader.read();
            if (rs.empty())
            {
                //TLOG_DEBUG("file empty()" << endl);
                break;
            }
            TLOGDEBUG("read " << rs.size() << " lines " << endl);
            ESWriter::postRawLog(file, rs);
            for (auto &&r : rs)
            {
                pair->aggregation->pushRawLog(r);
            }
        }
        return;
    }
    //TLOG_DEBUG("iterator != pairs_.end()" << endl);
    
    if (isExpectedFile(file))
    {
        auto absFileLog = getAbsLogFileName(file);
        auto pair = std::make_shared<LogReadAggregationPair>(absFileLog);
        pair->aggregation->setTimer(firstCheckTimer, checkCycleTimer, closureOvertime, maxOvertime);
        pairs_[file] = pair;
        while (true)
        {
            auto &&rs = pair->logReader.read();
            if (rs.empty())
            {
                break;
            }
            TLOGDEBUG("read " << rs.size() << " lines " << endl);
            ESWriter::postRawLog(file, rs);
            for (auto &&r : rs)
            {
                pair->aggregation->pushRawLog(r);
            }
        }

        /*
            we only monitor one file at the same time,
            if the current file is expected,means that other LogReadAggregationPair need to be deleted.
        */
        for (auto it = pairs_.begin(); it != pairs_.end();)
        {
            if (it->first == file)
            {
                it++;
                continue;
            }
            auto ptr = it->second;
            TimerTaskQueue::instance().pushTimerTask([ptr]
                                                     {
                /*
                 *  ptr will release after 30 minutes.
                 */ },
                                                     60 * 30);
            pairs_.erase(it++);
        }
    }
}