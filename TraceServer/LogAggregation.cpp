
#include <iostream>
#include "LogAggregation.h"
#include "TimerTaskQueue.h"
#include "ESWriter.h"
#include "servant/RemoteLogger.h"

static uint64_t hashGraph(const InternalGraph &graph) {
    std::ostringstream os{};
    for (auto &&e: graph.es) {
        os << e.fromVertex << e.toVertex << e.order;
    }
    for (auto &&v: graph.vs) {
        os << v.name;
    }
    return std::hash<std::string>()(os.str());
}

void LogAggregation::pushRawLog(const std::shared_ptr<RawLog> &rl) {
    auto trace = rl->trace;
    auto iterator = traces.find(trace);
    if (iterator == traces.end()) {
        auto tracePtr = std::make_shared<Trace>();
        tracePtr->push(rl);
        {
            std::lock_guard<std::mutex> lockGuard(traceMutex);
            traces[trace] = tracePtr;;
        }
        std::weak_ptr<LogAggregation> weakPtr = shared_from_this();
        auto callBack = [weakPtr, trace](const size_t &times, size_t &nextTime) {
            auto aggregation = weakPtr.lock();
            if (aggregation != nullptr) {
                aggregation->checkAndUpdateTrace(trace, nextTime);
            }
        };
        TimerTaskQueue::instance().pushCycleTask(callBack, firstCheckTimer, checkCycleTimer);
    } else {
        auto &&tracePtr = iterator->second;
        tracePtr->push(rl);
    }
}

void LogAggregation::checkAndUpdateTrace(const string &trace, size_t &nextTimer) {
    TLOGDEBUG("will check trace|" << trace << endl);
    auto iterator = traces.find(trace);
    if (iterator == traces.end()) {
        nextTimer = 0;
        return;
    }

    auto tracePtr = iterator->second;
    size_t overtime = closureOvertime;
    if (tracePtr->teTime != 0) {
        for (auto &&item: tracePtr->spans) {
            auto &&spanPtr = item.second;
            if (spanPtr->csTime == 0 || spanPtr->srTime == 0 || spanPtr->ssTime == 0 || spanPtr->crTime == 0) {
                overtime = maxOvertime;
                break;
            }
        }
    }

    if (tracePtr->updateTime + overtime >= time(nullptr)) {
        //todo ,we can set nextTimer here
        return;
    }

    nextTimer = false;
    TLOGDEBUG("trace|" << trace << " overtime, will release in memory" << endl);
    uint64_t sgID;
    {
        auto sg = tracePtr->graphServer();
        sgID = hashGraph(sg);
        auto sgLinkIterator = graphs.find(sgID);
        if (sgLinkIterator == graphs.end()) {
            auto linkPtr = std::make_shared<Link>("server");
            linkPtr->update(sg, true);
            graphs[sgID] = linkPtr;
            std::weak_ptr<LogAggregation> weakPtr = shared_from_this();
            auto callBack = [weakPtr, sgID](const size_t &times, size_t &nextTime) {
                auto aggregation = weakPtr.lock();
                if (aggregation != nullptr) {
                    aggregation->checkAndUpdateGraph(sgID, nextTime);
                }
            };
            TimerTaskQueue::instance().pushCycleTask(callBack, 1, 120);
        } else {
            auto linkPtr = sgLinkIterator->second;
            linkPtr->update(sg, false);
        }
    }
    uint64_t fgID;
    {
        auto fg = tracePtr->graphFunction();
        fgID = hashGraph(fg);
        auto fgLinkIterator = graphs.find(fgID);
        if (fgLinkIterator == graphs.end()) {
            auto graphPtr = std::make_shared<Link>("function");
            graphPtr->update(fg, true);
            graphs[fgID] = graphPtr;
            std::weak_ptr<LogAggregation> weakPtr = shared_from_this();
            auto callBack = [weakPtr, fgID](const size_t &times, size_t &nextTime) {
                auto aggregation = weakPtr.lock();
                if (aggregation != nullptr) {
                    aggregation->checkAndUpdateGraph(fgID, nextTime);
                }
            };
            TimerTaskQueue::instance().pushCycleTask(callBack, 1, 300);
        } else {
            auto linkPtr = fgLinkIterator->second;
            linkPtr->update(fg, false);
        }
    }

    ESWriter::postTrace(file, trace, tracePtr, sgID, fgID);
    {
        std::lock_guard<std::mutex> lockGuard(traceMutex);
        traces.erase(trace);
    }
}

void LogAggregation::checkAndUpdateGraph(uint64_t id, size_t &nextTimer) {
    auto iterator = graphs.find(id);
    if (iterator == graphs.end()) {
        throw std::runtime_error("should not happen");
    }
    auto &&graphPtr = iterator->second;
    TLOGDEBUG("will update graph " << id << endl);
    ESWriter::postLink(file, id, graphPtr);
}
