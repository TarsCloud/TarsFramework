
#include <mutex>
#include <sstream>
#include "ESWriter.h"
#include "ESIndex.h"
#include "servant/RemoteLogger.h"
#include "ESClient.h"
#include "TimerTaskQueue.h"

static std::string generaId(const std::string& indexName, const std::string& key)
{
	auto iIndexNameHash = std::hash<std::string>{}(indexName);
	auto iContentHash = std::hash<std::string>{}(key);
	auto hashValue = (iIndexNameHash << 54u) + iContentHash;
	return std::to_string(hashValue);
}

void ESWriter::createIndexTemplate()
{
	std::string response{};
	if (ESClient::instance().postRequest(ESClientRequestMethod::Put, LogIndexUri, LogIndexTemplate, response) != 200)
	{
		TLOGERROR("create index template \"tars_call_log\" error: " << response);
		return;
	}
	if (ESClient::instance().postRequest(ESClientRequestMethod::Put, TraceIndexTemplateUri, TraceIndexTemplate, response) != 200)
	{
		TLOGERROR("create index template \"tars_call_trace\" error: " << response);
		return;
	}
	if (ESClient::instance().postRequest(ESClientRequestMethod::Put, GraphIndexTemplateUri, GraphIndexTemplate, response) != 200)
	{
		TLOGERROR("create index template \"tars_call_graph\" error: " << response);
		return;
	}
}

void ESWriter::postRawLog(const std::string& file, const std::vector<std::shared_ptr<IRawLog>>& rawLogs, size_t triedTimes)
{
	auto index = buildLogIndexByLogfile(file);
	std::ostringstream os;
	for (auto&& r: rawLogs)
	{
		auto js = r->writeToJsonString();
		os << (R"({"index":{"_id":")") << generaId(index, js) << ("\"}}\n");
		os << js;
		os << "\n";
	}
	auto url = "/" + buildLogIndexByLogfile(file) + "/_bulk";
	auto body = os.str();
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Post, url, body, response);
	if (res != 200)
	{
		if (triedTimes <= 10)
		{
			TLOGERROR("do es request error: " << response << ", this is " << triedTimes << "th" << " retry" << endl;);
			TimerTaskQueue::instance().pushTimerTask([file, rawLogs, triedTimes]
			{
				ESWriter::postRawLog(file, rawLogs, triedTimes + 1);
			}, (triedTimes - 1) * 5 + 1);
			return;
		}
		TLOGERROR("do es request error: " << response << ", this is " << triedTimes << "th" << " retry, request will discard" << endl);
	}
}

void ESWriter::postTrace(const string& file, const std::string& traceName, const shared_ptr<ITrace>& tracePtr,
		uint64_t shash, uint64_t fhash, size_t triedTimes)
{
	auto index = buildTraceIndexByDate(file);
	std::ostringstream os;
	os << (R"({"index":{"_id":")") << generaId(index, traceName) << ("\"}}\n");
	os << tracePtr->writeToJsonString();
	os << "\n";
	auto url = "/" + buildTraceIndexByLogfile(file) + "/_bulk";
	auto body = os.str();
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Post, url, body, response);
	if (res != 200)
	{
		if (triedTimes <= 10)
		{
			TLOGERROR("do es request error: " << response << ", this is " << triedTimes << "th" << " retry" << endl;);
			TimerTaskQueue::instance().pushTimerTask([file, traceName, tracePtr, shash, fhash, triedTimes]
			{
				ESWriter::postTrace(file, traceName, tracePtr, shash, fhash, triedTimes + 1);
			}, (triedTimes - 1) * 5 + 1);
			return;
		}
		TLOGERROR("do es request error: " << response << ", this is " << triedTimes << "th" << " retry, request will discard" << endl);
	}
}

void ESWriter::postGraph(const string& file, uint64_t linkId, const shared_ptr<IGraph>& graph)
{
	std::ostringstream os;
	os << (R"({"index":{"_id":")") << linkId << ("\"}}\n");
	os << graph->writeToJsonString();
	os << "\n";
	auto url = "/" + buildGraphIndexByLogfile(file) + "/_bulk";
	auto body = os.str();
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Post, url, body, response);
	if (res != 200)
	{
		TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response);
		return;
	}
}
