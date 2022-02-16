#include "ESReader.h"
#include "TraceData.h"
#include "ESIndex.h"
#include <iostream>
#include "util/tc_common.h"
#include "ESClient.h"
#include "servant/RemoteLogger.h"

//void graphTraceFunction_(const ITrace *trace, short order, const string &parentFunction, const string &span, Graph &graph) {
//    const ISpan *ptr{};
//    for (auto &&item: trace->spans) {
//        if (item.span == span) {
//            ptr = &item;
//            break;
//        }
//    }
//    if (ptr == nullptr) {
//        throw std::runtime_error("span not exist");
//    }
//
//    auto function = ptr->slave + "." + ptr->function;
//    IVertex vertex{};
//    vertex.vertex = function;
//    if (ptr->srTime != 0 && ptr->ssTime != 0 && ptr->srTime <= ptr->ssTime) {
//        vertex.callCount += 1;
//        vertex.callTime += ptr->ssTime - ptr->srTime;
//    }
//    insertIVertex(vertex, graph.vertexes);
//
//    IEdge edge{};
//    edge.fromVertex = parentFunction;
//    edge.toVertex = function;
//    if (ptr->csTime != 0 && ptr->crTime != 0 && ptr->csTime <= ptr->crTime) {
//        edge.callCount += 1;
//        edge.callTime += ptr->crTime - ptr->csTime;
//    }
//    edge.order = order;
//    insertIEdge(edge, graph.edges);
//    for (auto &&child: ptr->children) {
//        graphTraceFunction_(trace, order + 1, function, child, graph);
//    }
//}
//
//void graphITraceFunction(const ITrace *trace, Graph &graph) {
//    if (trace->tSpan.empty()) {
//        return;
//    }
//    IVertex vertex{};
//    vertex.vertex = trace->tMaster;
//    if (trace->tsTime != 0 && trace->teTime != 0 && trace->tsTime <= trace->teTime) {
//        vertex.callCount += 1;
//        vertex.callTime += trace->teTime - trace->tsTime;
//    }
//    insertIVertex(vertex, graph.vertexes);
//    graphTraceFunction_(trace, 1, trace->tMaster, trace->tSpan, graph);
//    std::sort(graph.vertexes.begin(), graph.vertexes.end());
//    std::sort(graph.edges.begin(), graph.edges.end());
//}

static int readESResponse(const std::string& response, const std::function<int(const TC_AutoPtr<JsonValueObj>&)>& f)
{
	auto jsonPtr = TC_Json::getValue(response);
	auto&& jsonValuePtr = JsonValueObjPtr::dynamicCast(jsonPtr);
//	const auto& jsonValue = jsonValuePtr->value;
	auto&& firstHitsPtr = jsonValuePtr->get("hits");
	if (firstHitsPtr.get() == nullptr)
	{
		return -1;
	}
	auto&& firstHitsValue = JsonValueObjPtr::dynamicCast(firstHitsPtr);
	auto&& secondHitsPtr = firstHitsValue->get("hits");
	if (secondHitsPtr.get() == nullptr)
	{
		return -1;
	}
	auto&& secondHitsValuePtr = JsonValueArrayPtr::dynamicCast(secondHitsPtr);
	auto&& secondHitsValue = secondHitsValuePtr->value;
	for (auto&& hits: secondHitsValue)
	{
		auto&& hitsPtr = JsonValueObjPtr::dynamicCast(hits);
		auto&& sourcePtr = hitsPtr->get("_source");
		if (sourcePtr.get() == nullptr)
		{
			continue;
		}
		auto&& sourceValue = JsonValueObjPtr::dynamicCast(sourcePtr);
		int res = f(sourceValue);
		if (res != 0)
		{
			return res;
		}
	}
	return 0;
}

int ESReader::listTrace(const string& date, int64_t beginTime, int64_t endTime, const string& serverName, vector<string>& ts)
{
	constexpr char ListTraceTemplate[] = R"(
{
    "size":10000,
    "_source":["trace"],
    "query":{
        "bool":{
            "filter":[
                {
                    "range":{"tsTime":{"gte":BEGIN_TIME,"lte":END_TIME}}
                },
                {
                    "nested":{
                        "path":"spans",
                        "query":{
                            "bool":{
                                "should":[
                                    {"match":{"spans.master":"SERVER"}},
                                    {"match":{"spans.slave":"SERVER"}}
                                ]
                            }
                        }
                    }
                }
            ]
        }
    }
}
)";
	string body = ListTraceTemplate;
	map<string, string> replaceMap = {
			{ "BEGIN_TIME", to_string(beginTime) },
			{ "END_TIME",   to_string(endTime) },
			{ "SERVER",     serverName }
	};
	body = TC_Common::replace(body, replaceMap);
	auto url = "/" + buildTraceIndexByDate(date) + "/_search";
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body, response);
	if (res != 200)
	{
		TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	if (readESResponse(response, [&ts](const TC_AutoPtr<JsonValueObj>& ptr)mutable -> int
	{
		auto v = JsonValueStringPtr::dynamicCast(ptr->get("trace"))->value;
		ts.emplace_back(v);
		return 0;
	}) != 0)
	{
		TLOGERROR("unexpected response\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	return 0;
}

int ESReader::listFunction(const string& date, const string& serverName, set<string>& fs)
{
	constexpr char ListFunctionTemplate[] = R"(
{
  "size": 10000,
  "_source": ["vertexes.vertex"],
  "query": {
    "bool": {
      "filter": [
        {
          "term": {
            "type": "function"
          }
        },
        {
          "nested": {
            "path": "vertexes",
            "query": {
              "bool": {
                "must": [
                  {
                    "prefix": {
                      "vertexes.vertex": "SERVER."
                    }
                  }
                ]
              }
            }
          }
        }
      ]
    }
  }
})";
	string body = ListFunctionTemplate;
	map<string, string> replaceMap = {
			{ "SERVER", serverName }
	};
	body = TC_Common::replace(body, replaceMap);
	auto url = "/" + buildGraphIndexByDate(date) + "/_search";
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body, response);
	if (res != 200)
	{
		TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	if (readESResponse(response, [&fs, &serverName](const TC_AutoPtr<JsonValueObj>& ptr)mutable -> int
	{
		auto jsonObj = JsonValueObjPtr::dynamicCast(ptr);
		auto vertexes = jsonObj->get("vertexes");
		if (vertexes.get() == nullptr)
		{
			return 0;
		}
		auto vertexesArray = JsonValueArrayPtr::dynamicCast(vertexes);
		auto compareStr = serverName + ".";
		for (auto&& item: vertexesArray->value)
		{
			auto obj = JsonValueObjPtr::dynamicCast(item);
			auto v = JsonValueStringPtr::dynamicCast(obj->get("vertex"))->value;
			if (v.compare(0, compareStr.size(), compareStr) == 0)
			{
				fs.insert(v);
			}
		}
		return 0;
	}) != 0)
	{
		TLOGERROR("unexpected response\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	return 0;
}

int ESReader::listTraceSummary(const string& date, int64_t beginTime, int64_t endTime, const string& serverName, vector<Summary>& ss)
{
//fixme trace may had no tsTime.
	constexpr char ListTraceSummaryTemplate[] = R"(
{
    "size":10000,
    "_source":["trace","tsTime","teTime"],
    "query":{
        "bool":{
            "filter":[
                {"range":{"tsTime":{"gte":BEGIN_TIME,"lte":END_TIME}}},
                {"nested":{"path":"spans","query":{"bool":{"should":[{"match":{"spans.master":"SERVER"}},{"match":{"spans.slave":"SERVER"}}]}}}}
            ]
        }
    },
    "sort":{"tsTime":{"order":"desc"}}
}
)";
	string body = ListTraceSummaryTemplate;
	map<string, string> replaceMap = {
			{ "BEGIN_TIME", to_string(beginTime) },
			{ "END_TIME",   to_string(endTime) },
			{ "SERVER",     serverName }
	};
	body = TC_Common::replace(body, replaceMap);
	auto url = "/" + buildTraceIndexByDate(date) + "/_search";
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body, response);
	if (res != 200)
	{
		TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	if (readESResponse(response, [&ss](const TC_AutoPtr<JsonValueObj>& ptr)mutable -> int
	{
		auto traceName = JsonValueStringPtr::dynamicCast(ptr->get("trace"))->value;
		auto tsTimeNum = JsonValueNumPtr::dynamicCast(ptr->get("tsTime"))->value;
		auto teTimeNum = JsonValueNumPtr::dynamicCast(ptr->get("teTime"))->value;

		auto tsTime = (int64_t)tsTimeNum;
		auto teTime = (int64_t)teTimeNum;
		Summary summary;
		summary.name = traceName;
		summary.startTime = tsTime;
		summary.endTime = teTime;
		ss.emplace_back(summary);
		return 0;
	}) != 0)
	{
		TLOGERROR("unexpected response\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	auto compare = [](const Summary& ls, const Summary& rs)
	{
		return ls.startTime > rs.startTime;
	};
	sort(ss.begin(), ss.end(), compare);
	return 0;
}

int ESReader::getServerGraph(const string& date, const string& serverName, vector<IGraph>& graphs)
{
	constexpr char GetServerGraphTemplate[] = R"(
{
    "size":10000,
    "query":{
        "bool":{
            "filter":[
                {"term":{"type":"server"}},
                {"nested":{"path":"vertexes","query":{"bool":{"must":[{"term":{"vertexes.vertex":"SERVER"}}]}}}}
            ]
        }
    }
}
)";
	string body = GetServerGraphTemplate;
	map<string, string> replaceMap = {
			{ "SERVER", serverName }
	};
	body = TC_Common::replace(body, replaceMap);
	auto url = "/" + buildGraphIndexByDate(date) + "/_search";
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body, response);
	if (res != 200)
	{
		TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	if (readESResponse(response, [&graphs](const TC_AutoPtr<JsonValueObj>& ptr)mutable -> int
	{
		graphs.emplace_back();
		graphs.rbegin()->readFromJson(ptr);
		return 0;
	}) != 0)
	{
		TLOGERROR("unexpected response\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	return 0;
}

int ESReader::getFunctionGraph(const string& date, const string& functionName, vector<IGraph>& graphs)
{
	constexpr char GetFunctionGraphTemplate[] = R"(
{
    "size":10000,
    "query":{
        "bool":{
            "filter":[
                {"term":{"type":"function"}},
                {"nested":{"path":"vertexes","query":{"bool":{"must":[{"term":{"vertexes.vertex":"FUNCTION"}}]}}}}
            ]
        }
    }
}
)";
	string body = GetFunctionGraphTemplate;
	map<string, string> replaceMap = {
			{ "FUNCTION", functionName }
	};
	body = TC_Common::replace(body, replaceMap);
	auto url = "/" + buildGraphIndexByDate(date) + "/_search";
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body, response);
	if (res != 200)
	{
		TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	if (readESResponse(response, [&graphs](const TC_AutoPtr<JsonValueObj>& ptr)mutable -> int
	{
		graphs.emplace_back();
		graphs.rbegin()->readFromJson(ptr);
		return 0;
	}) != 0)
	{
		TLOGERROR("unexpected response\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	return 0;
}

int ESReader::getTrace(const string& date, const string& traceName, ITrace& trace)
{
	constexpr char GetTraceGraphTemplate[] = R"(
{
    "query":{
        "bool":{
            "filter":[
                {"term":{"trace":"TRACE"}}
            ]
        }
    }
}
)";
	string body = GetTraceGraphTemplate;
	map<string, string> replaceMap = {
			{ "TRACE", traceName }
	};
	body = TC_Common::replace(body, replaceMap);
	auto url = "/" + buildTraceIndexByDate(date) + "/_search";
	std::string response{};
	int res = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body, response);
	if (res != 200)
	{
		TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	if (readESResponse(response, [&trace](const TC_AutoPtr<JsonValue>& ptr)mutable -> int
	{
		trace.readFromJson(ptr);
		return 0;
	}) != 0)
	{
		TLOGERROR("unexpected response\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << response << endl);
		return -1;
	}
	return 0;
}