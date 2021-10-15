#include "ESReader.h"
#include "InternGraph.h"
#include <memory>
#include "Link.h"
#include "ESIndex.h"
#include <iostream>
#include "util/tc_common.h"
#include "ESClient.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "servant/RemoteLogger.h"

static std::shared_ptr<rapidjson::Document> parserToJson(const char *data, size_t dataLen) {
    auto document = std::make_shared<rapidjson::Document>();
    if (document->Parse(data, dataLen).HasParseError()) {
        return nullptr;
    }
    return document;
}

int ESReader::listTrace(const string &date, int64_t beginTime, int64_t endTime, const string &serverName, vector<string> &ts) {
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
            {"BEGIN_TIME", to_string(beginTime)},
            {"END_TIME",   to_string(endTime)},
            {"SERVER",     serverName}
    };
    body = TC_Common::replace(body, replaceMap);
    auto url = "/" + buildTraceIndexByDate(date) + "/_search";
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body);
    if (!request->waitFinish(std::chrono::seconds(2))) {
        TLOGERROR("request to es overtime" << endl);
    }
    if (request->phase() != Done) {
        TLOGERROR("request to es error, " << request->phaseMessage());
        return -1;
    }
    auto &&response = request->responseBody();
    auto jsonPtr = parserToJson(response, request->responseSize());
    if (jsonPtr == nullptr) {
        TLOGERROR("parser es response to json error\n, \tdata: " << response << endl);
        return -1;
    }
    auto hitsPtr = rapidjson::GetValueByPointer(*jsonPtr, "/hits/hits");
    if (hitsPtr == nullptr) {
        return 0;
    }
    assert(hitsPtr->IsArray());
    auto hitsValue = hitsPtr->GetArray();
    for (auto &&hit: hitsValue) {
        auto tracePtr = rapidjson::GetValueByPointer(hit, "/_source/trace");
        assert(tracePtr != nullptr && tracePtr->IsString());
        auto trace = string(tracePtr->GetString(), tracePtr->GetStringLength());
        ts.emplace_back(trace);
    }
    return 0;
}

int ESReader::listTraceSummary(const string &date, int64_t beginTime, int64_t endTime, const string &serverName, vector<Summary> &ss) {
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
            {"BEGIN_TIME", to_string(beginTime)},
            {"END_TIME",   to_string(endTime)},
            {"SERVER",     serverName}
    };
    body = TC_Common::replace(body, replaceMap);
    auto url = "/" + buildTraceIndexByDate(date) + "/_search";
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body);
    if (!request->waitFinish(std::chrono::seconds(2))) {
        TLOGERROR("request to es overtime" << endl);
    }
    if (request->phase() != Done) {
        TLOGERROR("request to es error, " << request->phaseMessage());
        return -1;
    }
    auto &&response = request->responseBody();
    auto jsonPtr = parserToJson(response, request->responseSize());
    if (jsonPtr == nullptr) {
        TLOGERROR("parser es response to json error\n, \tdata: " << response << endl);
        return -1;
    }
    auto hitsPtr = rapidjson::GetValueByPointer(*jsonPtr, "/hits/hits");
    if (hitsPtr == nullptr) {
        return 0;
    }
    assert(hitsPtr->IsArray());
    auto hitsValue = hitsPtr->GetArray();
    for (auto &&hit: hitsValue) {
        auto tracePtr = rapidjson::GetValueByPointer(hit, "/_source/trace");
        assert(tracePtr != nullptr && tracePtr->IsString());
        auto trace = string(tracePtr->GetString(), tracePtr->GetStringLength());

        auto tsPtr = rapidjson::GetValueByPointer(hit, "/_source/tsTime");
        assert(tsPtr != nullptr && tsPtr->IsInt64());
        auto tsTime = tsPtr->GetInt64();

        auto tePtr = rapidjson::GetValueByPointer(hit, "/_source/teTime");
        assert(tePtr != nullptr && tePtr->IsInt64());
        auto teTime = tePtr->GetInt64();
        Summary summary;
        summary.name = trace;
        summary.startTime = tsTime;
        summary.endTime = teTime;
        ss.emplace_back(summary);
    }
    auto compare = [](const Summary &ls, const Summary &rs) {
        return ls.startTime > rs.startTime;
    };
    sort(ss.begin(), ss.end(), compare);
    return 0;
}

int ESReader::getServerGraph(const string &date, const string &serverName, vector<InternalGraph> &graphs) {
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
            {"SERVER", serverName}
    };
    body = TC_Common::replace(body, replaceMap);
    auto url = "/" + buildGraphIndexByDate(date) + "/_search";
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body);
    if (!request->waitFinish(std::chrono::seconds(2))) {
        TLOGERROR("request to es overtime" << endl);
    }
    if (request->phase() != Done) {
        TLOGERROR("request to es error, " << request->phaseMessage());
        return -1;
    }
    auto &&response = request->responseBody();
    auto jsonPtr = parserToJson(response, request->responseSize());
    if (jsonPtr == nullptr) {
        TLOGERROR("parser es response to json error\n, \tdata: " << response << endl);
        return -1;
    }
    auto hitsPtr = rapidjson::GetValueByPointer(*jsonPtr, "/hits/hits");
    if (hitsPtr == nullptr) {
        return 0;
    }
    assert(hitsPtr->IsArray());
    auto hitsValue = hitsPtr->GetArray();
    graphs.reserve(hitsValue.Size());
    for (auto &&hit: hitsValue) {
        InternalGraph graph{};
        auto vertexesPtr = rapidjson::GetValueByPointer(hit, "/_source/vertexes");
        assert(vertexesPtr != nullptr && vertexesPtr->IsArray());
        auto vertexes = vertexesPtr->GetArray();
        for (auto &&item: vertexes) {
            auto iv = item.GetObject();
            InternalVertex vertex{};
            vertex.name = iv["vertex"].GetString();
            vertex.record = iv["record"].GetInt64();
            graph.vs.emplace(vertex);
        }

        auto edgesPtr = rapidjson::GetValueByPointer(hit, "/_source/edges");
        assert(edgesPtr != nullptr && edgesPtr->IsArray());
        auto edges = edgesPtr->GetArray();
        for (auto &&item: edges) {
            auto iv = item.GetObject();
            InternalEdge edge{};
            edge.fromVertex = iv["fromVertex"].GetString();
            edge.toVertex = iv["toVertex"].GetString();
            edge.record = iv["record"].GetInt64();
            edge.order = iv["order"].GetInt();
            graph.es.emplace(edge);
        }
        graphs.emplace_back(graph);
    }
    return 0;
}

int ESReader::getFunctionGraph(const string &date, const string &functionName, vector<InternalGraph> &graphs) {
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
            {"FUNCTION", functionName}
    };
    body = TC_Common::replace(body, replaceMap);
    auto url = "/" + buildGraphIndexByDate(date) + "/_search";
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body);
    if (!request->waitFinish(std::chrono::seconds(2))) {
        TLOGERROR("request to es overtime" << endl);
    }
    if (request->phase() != Done) {
        TLOGERROR("request to es error, " << request->phaseMessage());
        return -1;
    }
    auto &&response = request->responseBody();
    auto jsonPtr = parserToJson(response, request->responseSize());
    if (jsonPtr == nullptr) {
        TLOGERROR("parser es response to json error\n, \tdata: " << response << endl);
        return -1;
    }
    auto hitsPtr = rapidjson::GetValueByPointer(*jsonPtr, "/hits/hits");
    if (hitsPtr == nullptr) {
        return 0;
    }
    assert(hitsPtr->IsArray());
    auto hitsValue = hitsPtr->GetArray();
    graphs.reserve(hitsValue.Size());
    for (auto &&hit: hitsValue) {
        InternalGraph graph{};
        auto vertexesPtr = rapidjson::GetValueByPointer(hit, "/_source/vertexes");
        assert(vertexesPtr != nullptr && vertexesPtr->IsArray());
        auto vertexes = vertexesPtr->GetArray();
        for (auto &&item: vertexes) {
            auto iv = item.GetObject();
            InternalVertex vertex{};
            vertex.name = iv["vertex"].GetString();
            vertex.record = iv["record"].GetInt64();
            graph.vs.emplace(vertex);
        }

        auto edgesPtr = rapidjson::GetValueByPointer(hit, "/_source/edges");
        assert(edgesPtr != nullptr && edgesPtr->IsArray());
        auto edges = edgesPtr->GetArray();
        for (auto &&item: edges) {
            auto iv = item.GetObject();
            InternalEdge edge{};
            edge.fromVertex = iv["fromVertex"].GetString();
            edge.toVertex = iv["toVertex"].GetString();
            edge.record = iv["record"].GetInt64();
            edge.order = iv["order"].GetInt();
            graph.es.emplace(edge);
        }
        graphs.emplace_back(graph);
    }
    return 0;

}

int ESReader::getTraceGraph(const string &date, const string &traceName, InternalGraph &graph) {
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
            {"TRACE", traceName}
    };
    body = TC_Common::replace(body, replaceMap);
    auto url = "/" + buildTraceIndexByDate(date) + "/_search";
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body);
    if (!request->waitFinish(std::chrono::seconds(2))) {
        TLOGERROR("request to es overtime" << endl);
    }
    if (request->phase() != Done) {
        TLOGERROR("request to es error, " << request->phaseMessage());
        return -1;
    }
    auto &&response = request->responseBody();
    auto jsonPtr = parserToJson(response, request->responseSize());
    if (jsonPtr == nullptr) {
        TLOGERROR("parser es response to json error\n, \tdata: " << response << endl);
        return -1;
    }
    auto hitsPtr = rapidjson::GetValueByPointer(*jsonPtr, "/hits/hits");
    if (hitsPtr == nullptr) {
        return 0;
    }
    assert(hitsPtr->IsArray());
    auto hitsValue = hitsPtr->GetArray();
    for (auto &&hit: hitsValue) {
        auto sourcePtr = rapidjson::GetValueByPointer(hit, "/_source");
        assert(sourcePtr != nullptr && sourcePtr->IsObject());

        auto &&sourceValue = sourcePtr->GetObject();
        auto tracePtr = std::make_shared<Trace>();
        tracePtr->tSpan = sourceValue["tSpan"].GetString();
        tracePtr->tMaster = sourceValue["tMaster"].GetString();
        tracePtr->tsTime = sourceValue["tsTime"].GetInt64();
        tracePtr->teTime = sourceValue["teTime"].GetInt64();
        auto spansValue = sourceValue["spans"].GetArray();
        for (auto &&span: spansValue) {
            auto &&spanValue = span.GetObject();
            auto spanName = span["span"].GetString();
            auto spanPtr = std::make_shared<Span>();
            spanPtr->parent = span["parent"].GetString();
            spanPtr->parent = span["parent"].GetString();
            spanPtr->master = span["master"].GetString();
            spanPtr->slave = span["slave"].GetString();
            spanPtr->function = span["function"].GetString();
            spanPtr->csTime = span["csTime"].GetInt64();
            spanPtr->srTime = span["srTime"].GetInt64();
            spanPtr->ssTime = span["ssTime"].GetInt64();
            spanPtr->crTime = span["crTime"].GetInt64();
            spanPtr->csData = span["csData"].GetString();
            spanPtr->srData = span["srData"].GetString();
            spanPtr->ssData = span["ssData"].GetString();
            spanPtr->crData = span["crData"].GetString();
            spanPtr->ret = span["ret"].GetString();
            auto &&children = span["children"].GetArray();
            for (auto &&child: children) {
                spanPtr->children.insert(child.GetString());
            }
            tracePtr->spans[spanName] = spanPtr;
        }
        graph = tracePtr->graphFunction();
        return 0;
    }
    return 0;
}

int ESReader::listFunction(const string &date, const string &serverName, set<string> &fs) {
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
            {"SERVER", serverName}
    };
    body = TC_Common::replace(body, replaceMap);
    auto url = "/" + buildGraphIndexByDate(date) + "/_search";
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Get, url, body);
    if (!request->waitFinish(std::chrono::seconds(2))) {
        TLOGERROR("request to es overtime" << endl);
    }
    if (request->phase() != Done) {
        TLOGERROR("request to es error, " << request->phaseMessage());
        return -1;
    }
    auto &&response = request->responseBody();
    auto jsonPtr = parserToJson(response, request->responseSize());
    if (jsonPtr == nullptr) {
        TLOGERROR("parser es response to json error\n, \tdata: " << response << endl);
        return -1;
    }
    auto hitsPtr = rapidjson::GetValueByPointer(*jsonPtr, "/hits/hits");
    if (hitsPtr == nullptr) {
        return 0;
    }
    assert(hitsPtr->IsArray());
    auto hitsValue = hitsPtr->GetArray();
    for (auto &&hit: hitsValue) {
        auto sourcePtr = rapidjson::GetValueByPointer(hit, "/_source");
        assert(sourcePtr != nullptr && sourcePtr->IsObject());
        auto &&sourceValue = sourcePtr->GetObject();
        auto vertexes = sourceValue["vertexes"].GetArray();
        for (auto &&vertex: vertexes) {
            auto v = vertex.GetObject();
            std::string s = v["vertex"].GetString();
            if (s.compare(0, serverName.size(), serverName) == 0) {
                fs.emplace(s);
            }
        }
    }
    return 0;
}
