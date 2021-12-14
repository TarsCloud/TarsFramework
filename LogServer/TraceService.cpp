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
#include "SnapshotDraftsman.h"

void TraceService::initialize(TC_Config &config)
{

    //auto &&config = getConfig();

    string message{};
	if (!loadTimerValue(config, message))
	{
		throw std::runtime_error("bad config: " + message);
	}

	logDir_ = config.get("/tars/trace<log_dir>", "/usr/local/app/tars/remote_app_log/_tars_/_trace_");
	SnapshotDraftsman::setSavePath(ServerConfig::DataPath);

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

    timerThread_ = std::thread([]
	{
		TimerTaskQueue::instance().run();
	});
	timerThread_.detach();


	TimerTaskQueue::instance().pushCycleTask(
			[this](const size_t&, size_t&)
			{
				auto todayLogFile = buildLogFileName(false);
				auto absTodayLogFile = buildLogFileName(true);
				if (TC_File::isFileExist(absTodayLogFile))
				{
					onModify(todayLogFile);
				}
			}, 0, 1);

	ESWriter::createIndexTemplate();
};

bool TraceService::loadTimerValue(const TC_Config& config, string& message)
{
	snapshotTimer_ = TC_Common::strto<int>(config.get("/tars/trace<SnapshotTimer>", "300"));
	firstCheckTimer_ = TC_Common::strto<int>(config.get("/tars/trace<FirstCheckTimer>", "3"));
	checkCycleTimer_ = TC_Common::strto<int>(config.get("/tars/trace<CheckCycleTimer>", "3"));
	closureOvertime_ = TC_Common::strto<int>(config.get("/tars/trace<OvertimeWhenClosure>", "3"));
	maxOvertime_ = TC_Common::strto<int>(config.get("/tars/trace<Overtime>", "100"));

	if (snapshotTimer_ < 60 || snapshotTimer_ > 10000)
	{
		message = "SnapshotTimer value should in [60,10000]";
		return false;
	}

	if (firstCheckTimer_ < 1 || firstCheckTimer_ > 600)
	{
		message = "FirstCheckTimer value should in [1,600]";
		return false;
	}

	if (checkCycleTimer_ < 1 || checkCycleTimer_ > 100)
	{
		message = "CheckCycleTimer value should in [1,100]";
		return false;
	}

	if (closureOvertime_ < 1 || closureOvertime_ > 100)
	{
		message = "OvertimeWhenClosure value should in [1,100]";
		return false;
	}

	if (maxOvertime_ < 100 || maxOvertime_ > 600)
	{
		message = "Overtime value should in [100,600]";
		return false;
	}

	for (auto& pair: pairs_)
	{
		pair.second->aggregation->setTimer(firstCheckTimer_, checkCycleTimer_, closureOvertime_, maxOvertime_);
	}

	message = "done";
	return true;
}

// void TraceService::destroyApp() {
//     TC_Common::msleep(1);
// }

void TraceService::onModify(const string &file)
{
	auto iterator = pairs_.find(file);
	if (iterator != pairs_.end())
	{
		auto pair = iterator->second;
		while (true)
		{
			auto&& rs = pair->logReader->read();
			if (rs.empty())
			{
				break;
			}
			TLOGDEBUG("read " << rs.size() << " lines " << endl);
			ESWriter::postRawLog(file, rs);
			for (auto&& r: rs)
			{
				pair->aggregation->pushRawLog(r);
			}
		}
		return;
	}

	if (isExpectedFile(file))
	{
		auto absFileLog = getAbsLogFileName(file);
		auto pair = std::make_shared<LogReadAggregationPair>(absFileLog);
		SnapshotDraftsman::restoreSnapshot(pair->logReader, pair->aggregation);

		std::weak_ptr<LogReadAggregationPair> weak_ptr = pair;
		TimerTaskQueue::instance().pushCycleTask([weak_ptr](const size_t&, size_t& nextTimer)
		{
			auto ptr = weak_ptr.lock();
			if (ptr != nullptr)
			{
				SnapshotDraftsman::createSnapshot(ptr->logReader, ptr->aggregation);
				return;
			}
			nextTimer = 0;
		}, snapshotTimer_, snapshotTimer_);
		pair->aggregation->setTimer(firstCheckTimer_, checkCycleTimer_, closureOvertime_, maxOvertime_);
		pairs_[file] = pair;
		while (true)
		{
			auto&& rs = pair->logReader->read();
			if (rs.empty())
			{
				break;
			}
			TLOGDEBUG("read " << rs.size() << " lines " << endl);
			ESWriter::postRawLog(file, rs);
			for (auto&& r: rs)
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
				 */
			}, 60 * 30);
			pairs_.erase(it++);
		}
	}
}