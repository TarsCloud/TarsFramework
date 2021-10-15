
#include <mutex>
#include <sstream>
#include "ESWriter.h"
#include "ESIndex.h"
#include "JsonTr.h"
#include "servant/RemoteLogger.h"
#include "ESClient.h"
#include "TimerTaskQueue.h"

static std::string generaId(const std::string &indexName, const std::string &key) {
    auto iIndexNameHash = std::hash<std::string>{}(indexName);
    auto iContentHash = std::hash<std::string>{}(key);
    auto hashValue = (iIndexNameHash << 54u) + iContentHash;
    return std::to_string(hashValue);
}

void ESWriter::createIndexTemplate() {
    auto a = ESClient::instance().postRequest(ESClientRequestMethod::Put, LogIndexUri, LogIndexTemplate);
    auto b = ESClient::instance().postRequest(ESClientRequestMethod::Put, TraceIndexTemplateUri, TraceIndexTemplate);
    auto c = ESClient::instance().postRequest(ESClientRequestMethod::Put, GraphIndexTemplateUri, GraphIndexTemplate);
    a->waitFinish();
    b->waitFinish();
    c->waitFinish();
}

void ESWriter::postRawLog(const std::string &file, const std::vector<std::shared_ptr<RawLog>> &rawLogs, size_t triedTimes) {
    auto index = buildLogIndexByLogfile(file);
    std::ostringstream os;
    for (auto &&r: rawLogs) {
        os << (R"({"index":{"_id":")") << generaId(index, r->line) << ("\"}}\n");
        os << "{";
        os << jsonTr("trace") << ":" << jsonTr(r->trace);
        os << "," << jsonTr("span") << ":" << jsonTr(r->span);
        os << "," << jsonTr("parent") << ":" << jsonTr(r->parent);
        os << "," << jsonTr("type") << ":" << jsonTr(r->type);
        os << "," << jsonTr("master") << ":" << jsonTr(r->master);
        os << "," << jsonTr("slave") << ":" << jsonTr(r->slave);
        os << "," << jsonTr("function") << ":" << jsonTr(r->function);
        os << "," << jsonTr("time") << ":" << jsonTr(r->time);
        os << "," << jsonTr("ret") << ":" << jsonTr(r->ret);
        os << "," << jsonTr("data") << ":" << jsonTr(r->data);
        os << "}";
        os << "\n";
    }
    auto uri = "/" + buildLogIndexByLogfile(file) + "/_bulk";
    auto body = os.str();
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Post, uri, body);
    request->waitFinish();
    if (request->phase() != Done) {
        if (triedTimes <= 10) {
            TLOGERROR("do es request error: " << request->phaseMessage() << ", this is " << triedTimes << "th" << "retry");
            TimerTaskQueue::instance().pushTimerTask([file, rawLogs, triedTimes] {
                ESWriter::postRawLog(file, rawLogs, triedTimes + 1);
            }, (triedTimes - 1) * 5 + 1);
            return;
        }
        TLOGERROR("do es request error: " << request->phaseMessage() << ", this is " << triedTimes << "th" << "retry, request will discard" << endl);
    }
    if (request->responseCode() != HTTP_STATUS_OK) {
        TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << request->responseBody());
        return;
    }
}

void ESWriter::postTrace(const string &file, const std::string &traceName, const shared_ptr<Trace> &tracePtr,
                         uint64_t serverGraphId, uint64_t functionGraphID, size_t triedTimes) {
    auto index = buildTraceIndexByDate(file);
    std::ostringstream os;
    os << (R"({"index":{"_id":")") << generaId(index, traceName) << ("\"}}\n");
    os << "{";
    os << jsonTr("trace") << ":" << jsonTr(traceName);
    os << "," << jsonTr("tSpan") << ":" << jsonTr(tracePtr->tSpan);
    os << "," << jsonTr("tMaster") << ":" << jsonTr(tracePtr->tMaster);
    os << "," << jsonTr("tsTime") << ":" << jsonTr(tracePtr->tsTime);
    os << "," << jsonTr("teTime") << ":" << jsonTr(tracePtr->teTime);
    os << "," << jsonTr("sHash") << ":" << jsonTr(serverGraphId);
    os << "," << jsonTr("fHash") << ":" << jsonTr(functionGraphID);
    os << "," << jsonTr("spans") << ": [";
    bool firstSpan = true;
    for (auto &&item: tracePtr->spans) {
        os << (firstSpan ? "" : ",");
        firstSpan = false;
        auto &&spanPtr = item.second;
        os << "{";
        os << jsonTr("span") << ":" << jsonTr(item.first);
        os << "," << jsonTr("parent") << ":" << jsonTr(spanPtr->parent);
        os << "," << jsonTr("master") << ":" << jsonTr(spanPtr->master);
        os << "," << jsonTr("slave") << ":" << jsonTr(spanPtr->slave);
        os << "," << jsonTr("function") << ":" << jsonTr(spanPtr->function);
        os << "," << jsonTr("csTime") << ":" << jsonTr(spanPtr->csTime);
        os << "," << jsonTr("srTime") << ":" << jsonTr(spanPtr->srTime);
        os << "," << jsonTr("ssTime") << ":" << jsonTr(spanPtr->ssTime);
        os << "," << jsonTr("crTime") << ":" << jsonTr(spanPtr->crTime);
        os << "," << jsonTr("csData") << ":" << jsonTr(spanPtr->csData);
        os << "," << jsonTr("srData") << ":" << jsonTr(spanPtr->srData);
        os << "," << jsonTr("ssData") << ":" << jsonTr(spanPtr->ssData);
        os << "," << jsonTr("crData") << ":" << jsonTr(spanPtr->crData);
        os << "," << jsonTr("ret") << ":" << jsonTr(spanPtr->ret);
        os << "," << jsonTr("children") << ":" << "[";
        bool firstChild = true;
        for (auto &&child: spanPtr->children) {
            os << (firstChild ? "" : ",");
            firstChild = false;
            os << jsonTr(child);
        }
        os << "]";
        os << "}";
    }
    os << "]}";
    os << "\n";
    auto url = "/" + buildTraceIndexByLogfile(file) + "/_bulk";
    auto body = os.str();
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Post, url, body);
    request->waitFinish();
    if (request->phase() != Done) {
        TLOGERROR("do es request error: " << request->phaseMessage() << ", this is " << triedTimes << "th" << "retry");
        if (triedTimes <= 10) {
            TimerTaskQueue::instance().pushTimerTask([file, traceName, tracePtr, serverGraphId, functionGraphID, triedTimes] {
                ESWriter::postTrace(file, traceName, tracePtr, serverGraphId, functionGraphID, triedTimes + 1);
            }, (triedTimes - 1) * 5 + 1);
        }
        return;
    }
    if (request->responseCode() != HTTP_STATUS_OK) {
        TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << request->responseBody());
        return;
    }
}

void ESWriter::postLink(const string &file, uint64_t linkId, const shared_ptr<Link> &linkPtr) {
    std::ostringstream os;
    os << (R"({"index":{"_id":")") << linkId << ("\"}}\n");
    os << "{";
    os << jsonTr("type") << ":" << jsonTr(linkPtr->type);
    os << "," << jsonTr("vertexes") << ":" << "[";
    bool first = true;
    for (auto &&v: linkPtr->vertexes) {
        os << (first ? "" : ",");
        first = false;
        os << "{";
        os << jsonTr("vertex") << ":" << jsonTr(v.second->name);
        os << "," << jsonTr("record") << ":" << jsonTr(v.second->record);
        os << "}";
    }
    os << "]";
    os << "," << jsonTr("edges") << ":" << "[";
    first = true;
    for (auto &&e: linkPtr->edges) {
        os << (first ? "" : ",");
        first = false;
        os << "{";
        os << jsonTr("fromVertex") << ":" << jsonTr(e.second->fromVertex);
        os << "," << jsonTr("toVertex") << ":" << jsonTr(e.second->toVertex);
        os << "," << jsonTr("record") << ":" << jsonTr(e.second->record);
        os << "," << jsonTr("order") << ":" << jsonTr(e.second->order);
        os << "}";
    }
    os << "]";
    os << "}";
    os << "\n";

    auto url = "/" + buildGraphIndexByLogfile(file) + "/_bulk";
    auto body = os.str();
    auto request = ESClient::instance().postRequest(ESClientRequestMethod::Post, url, body);
    request->waitFinish();
    if (request->phase() != Done) {
        TLOGERROR("do es request error: " << request->phaseMessage());
        return;
    }
    if (request->responseCode() != HTTP_STATUS_OK) {
        TLOGERROR("do es request error\n, \tRequest: " << body.substr(0, 2048) << "\n, \t" << request->responseBody());
        return;
    }
}
