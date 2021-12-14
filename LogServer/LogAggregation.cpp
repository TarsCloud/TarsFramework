
#include <iostream>
#include "LogAggregation.h"
#include "TimerTaskQueue.h"
#include "ESWriter.h"
#include "servant/RemoteLogger.h"
#include <string>

using namespace internal;

#define ASSIGN_IF_EMPTY(A, B) (A)=((A).empty())?(B):(A);

static int64_t hashIGraph(const IGraph &graph) {
    std::ostringstream os{};
    for (auto &&e: graph.edges) {
        os << e.fromVertex << e.toVertex << e.order;
    }
    for (auto &&v: graph.vertexes) {
        os << v.vertex;
    }
    uint64_t v = std::hash<std::string>()(os.str());
    return (int64_t)(v >> 1);
}

struct ITraceOperator {
    ITraceOperator()
            : itrace{std::make_shared<ITrace>()} {
    };
    time_t updateTime{};
    std::shared_ptr <ITrace> itrace{};
};

struct IGraphOperator {
    IGraphOperator()
            : igraph{std::make_shared<IGraph>()} {
    };
    time_t updateTime{};
    std::shared_ptr <IGraph> igraph{};
};

static void insertIEdge(const IEdge &edge, vector <IEdge> &edges) {
    for (auto &&item: edges) {
        if (item.fromVertex == edge.fromVertex && item.toVertex == edge.toVertex && item.order == edge.order) {
            item.callCount += edge.callCount;
            item.callTime += edge.callTime;
            return;
        }
    }
    edges.emplace_back(edge);
}

static void insertIVertex(const IVertex &vertex, vector <IVertex> &vertexes) {
    for (auto &&item: vertexes) {
        if (item.vertex == vertex.vertex) {
            item.callCount += vertex.callCount;
            item.callTime += vertex.callTime;
            return;
        }
    }
    vertexes.emplace_back(vertex);
}

static void insertChild(const string &str, vector <string> &strings) {
    for (auto &&item: strings) {
        if (item == str) {
            return;
        }
    }
    strings.emplace_back(str);
}

static void
addCheckITraceTimerTask(const std::weak_ptr <LogAggregation> &weakPtr, const std::string &traceName, size_t firstTime,
                        size_t cycleTimer) {
    auto callBack = [weakPtr, traceName](const size_t &times, size_t &nextTimer) {
        auto aggregation = weakPtr.lock();
        if (aggregation != nullptr) {
            aggregation->checkAndUpdateITrace(traceName, nextTimer);
            return;
        }
        nextTimer = 0;
    };
    TimerTaskQueue::instance().pushCycleTask(callBack, firstTime, cycleTimer);
}

static void addCheckIGraphTimerTask(const std::weak_ptr <LogAggregation> &weakPtr, int64_t graphID, size_t firstTime,
                                    size_t cycleTimer) {
    auto callBack = [weakPtr, graphID](const size_t &times, size_t &nextTimer) {
        auto aggregation = weakPtr.lock();
        if (aggregation != nullptr) {
            aggregation->checkAndUpdateIGraph(graphID, nextTimer);
            return;
        }
        nextTimer = 0;
    };
    TimerTaskQueue::instance().pushCycleTask(callBack, firstTime, cycleTimer);
}

void updateISpan(const IRawLog *rawLog, ISpan *&span) {
    do {
        if (rawLog->type == "ts" || rawLog->type == "cs") {
            ASSIGN_IF_EMPTY(span->parent, rawLog->parent)
            ASSIGN_IF_EMPTY(span->master, rawLog->master)
            ASSIGN_IF_EMPTY(span->slave, rawLog->slave)
            ASSIGN_IF_EMPTY(span->function, rawLog->function)
            ASSIGN_IF_EMPTY(span->csData, rawLog->data)
            span->csTime = rawLog->time;
            break;
        }
        if (rawLog->type == "te" || rawLog->type == "cr") {
            ASSIGN_IF_EMPTY(span->parent, rawLog->parent)
            ASSIGN_IF_EMPTY(span->master, rawLog->master)
            ASSIGN_IF_EMPTY(span->slave, rawLog->slave)
            ASSIGN_IF_EMPTY(span->function, rawLog->function)
            ASSIGN_IF_EMPTY(span->ret, rawLog->ret)
            ASSIGN_IF_EMPTY(span->crData, rawLog->data)
            span->crTime = rawLog->time;
            break;
        }
        if (rawLog->type == "sr") {
            ASSIGN_IF_EMPTY(span->slave, rawLog->slave)
            ASSIGN_IF_EMPTY(span->function, rawLog->function)
            ASSIGN_IF_EMPTY(span->srData, rawLog->data)
            span->srTime = rawLog->time;
            break;
        }
        if (rawLog->type == "ss") {
            ASSIGN_IF_EMPTY(span->slave, rawLog->slave)
            ASSIGN_IF_EMPTY(span->function, rawLog->function)
            ASSIGN_IF_EMPTY(span->ret, rawLog->ret)
            ASSIGN_IF_EMPTY(span->ssData, rawLog->data)
            span->ssTime = rawLog->time;
            break;
        }
    } while (false);
}

void updateITrace(const IRawLog *rawLog, ITrace *trace) {
    do {
        ASSIGN_IF_EMPTY(trace->trace, rawLog->trace);

        if (rawLog->parent == rawLog->span) {
            if (rawLog->type == "cs") {
                trace->tsTime = rawLog->time;
                ASSIGN_IF_EMPTY(trace->tSpan, rawLog->span)
                ASSIGN_IF_EMPTY(trace->tMaster, rawLog->master)
                break;
            }

            if (rawLog->type == "cr") {
                trace->teTime = rawLog->time;
                ASSIGN_IF_EMPTY(trace->tSpan, rawLog->span)
                ASSIGN_IF_EMPTY(trace->tMaster, rawLog->master)
                break;
            }
        }

        if (rawLog->type == "ts") {
            trace->tsTime = rawLog->time;
            ASSIGN_IF_EMPTY(trace->tSpan, rawLog->span)
            ASSIGN_IF_EMPTY(trace->tMaster, rawLog->master)
            break;
        }

        if (rawLog->type == "te") {
            trace->teTime = rawLog->time;
            ASSIGN_IF_EMPTY(trace->tSpan, rawLog->span)
            ASSIGN_IF_EMPTY(trace->tMaster, rawLog->master)
            break;
        }

        if (rawLog->type == "cs" || rawLog->type == "cr") {
            ISpan *pParentSpan = nullptr;
            for (auto &&item: trace->spans) {
                if (rawLog->parent == item.span) {
                    pParentSpan = &item;
                    break;
                }
            }
            if (pParentSpan == nullptr) {
                trace->spans.emplace_back();
                pParentSpan = &(*trace->spans.rbegin());
            }
            ASSIGN_IF_EMPTY(pParentSpan->span, rawLog->parent);
            insertChild(rawLog->span, pParentSpan->children);
        }

    } while (false);

    ISpan *pSpan = nullptr;

    for (auto &&item: trace->spans) {
        if (rawLog->span == item.span) {
            pSpan = &item;
            break;
        }
    }
    if (pSpan == nullptr) {
        trace->spans.emplace_back();
        pSpan = &(*trace->spans.rbegin());
    }
    ASSIGN_IF_EMPTY(pSpan->span, rawLog->span);
    updateISpan(rawLog, pSpan);
}

void mergerIGraph(const IGraph *from, const std::string &type, IGraph *pGraph) {
    ASSIGN_IF_EMPTY(pGraph->type, type);
    for (auto &&v: from->vertexes) {
        insertIVertex(v, pGraph->vertexes);
    }
    for (auto &&e: from->edges) {
        insertIEdge(e, pGraph->edges);
    }
}

void updateTraceOperator(const IRawLog *rawLog, ITraceOperator *traceOperator) {
    traceOperator->updateTime = TNOW;
    updateITrace(rawLog, traceOperator->itrace.get());
};

void
graphTraceFunction_(const ITrace *trace, short order, const string &parentFunction, const string &span, IGraph &graph) {
    const ISpan *ptr{};
    for (auto &&item: trace->spans) {
        if (item.span == span) {
            ptr = &item;
            break;
        }
    }
    if (ptr == nullptr) {
        throw std::runtime_error("span not exist");
    }

    auto function = ptr->slave + "." + ptr->function;
    IVertex vertex{};
    vertex.vertex = function;
    if (ptr->srTime != 0 && ptr->ssTime != 0 && ptr->srTime <= ptr->ssTime) {
        vertex.callCount += 1;
        vertex.callTime += ptr->ssTime - ptr->srTime;
    }
    insertIVertex(vertex, graph.vertexes);

    IEdge edge{};
    edge.fromVertex = parentFunction;
    edge.toVertex = function;
    if (ptr->csTime != 0 && ptr->crTime != 0 && ptr->csTime <= ptr->crTime) {
        edge.callCount += 1;
        edge.callTime += ptr->crTime - ptr->csTime;
    }
    edge.order = order;
    insertIEdge(edge, graph.edges);
    for (auto &&child: ptr->children) {
        graphTraceFunction_(trace, order + 1, function, child, graph);
    }
}

void graphITraceFunction(const ITrace *trace, IGraph &graph) {
    if (trace->tSpan.empty()) {
        return;
    }
    IVertex vertex{};
    vertex.vertex = trace->tMaster;
    if (trace->tsTime != 0 && trace->teTime != 0 && trace->tsTime <= trace->teTime) {
        vertex.callCount += 1;
        vertex.callTime += trace->teTime - trace->tsTime;
    }
    insertIVertex(vertex, graph.vertexes);
    graphTraceFunction_(trace, 1, trace->tMaster, trace->tSpan, graph);
    std::sort(graph.vertexes.begin(), graph.vertexes.end());
    std::sort(graph.edges.begin(), graph.edges.end());
}

void graphITraceServer_(const ITrace *trace, const string &parentServer, const string &span, IGraph &graph) {
    const ISpan *ptr{};
    for (auto &&item: trace->spans) {
        if (item.span == span) {
            ptr = &item;
            break;
        }
    }
    if (ptr == nullptr) {
        throw std::runtime_error("span not exist");
    }

    IVertex vertex{};
    vertex.vertex = ptr->slave;
    if (ptr->srTime != 0 && ptr->ssTime != 0 && ptr->srTime <= ptr->ssTime) {
        vertex.callCount += 1;
        vertex.callTime += ptr->ssTime - ptr->srTime;
    }
    insertIVertex(vertex, graph.vertexes);

    IEdge edge{};
    edge.fromVertex = parentServer;
    edge.toVertex = ptr->slave;
    if (ptr->csTime != 0 && ptr->crTime != 0 && ptr->csTime <= ptr->crTime) {
        edge.callCount += 1;
        edge.callTime += ptr->crTime - ptr->csTime;
    }
    insertIEdge(edge, graph.edges);
    for (auto &&child: ptr->children) {
        graphITraceServer_(trace, ptr->slave, child, graph);
    }
}

void graphITraceServer(const ITrace *trace, IGraph &graph) {
    if (trace->tSpan.empty()) {
        return;
    }
    IVertex vertex{};
    vertex.vertex = trace->tMaster;
    if (trace->tsTime != 0 && trace->teTime != 0 && trace->tsTime <= trace->teTime) {
        vertex.callCount += 1;
        vertex.callTime += trace->teTime - trace->tsTime;
    }
    insertIVertex(vertex, graph.vertexes);
    graphITraceServer_(trace, trace->tMaster, trace->tSpan, graph);
    std::sort(graph.vertexes.begin(), graph.vertexes.end());
    std::sort(graph.edges.begin(), graph.edges.end());
}

void LogAggregation::pushRawLog(const std::shared_ptr <IRawLog> &rl) {
    auto trace = rl->trace;
    auto iterator = itraceOperators_.find(trace);
    if (iterator == itraceOperators_.end()) {
        auto itracePtr = std::make_shared<ITraceOperator>();
        updateTraceOperator(rl.get(), itracePtr.get());
        std::weak_ptr <LogAggregation> weakPtr = shared_from_this();
        addCheckITraceTimerTask(weakPtr, trace, firstCheckTimer_, checkCycleTimer_);
        itraceOperators_[trace] = itracePtr;
    } else {
        auto &&tracePtr = iterator->second;
        updateITrace(rl.get(), tracePtr->itrace.get());
    }
}

void LogAggregation::checkAndUpdateITrace(const string &trace, size_t &nextTimer) {
    TLOGDEBUG("will check trace|" << trace << endl);
    auto iterator = itraceOperators_.find(trace);
    if (iterator == itraceOperators_.end()) {
        nextTimer = 0;
        return;
    }

    auto traceOperator = iterator->second;
    size_t overtime = closureOvertime_;
    if (traceOperator->itrace->teTime != 0) {
        for (auto &&item: traceOperator->itrace->spans) {
            if (item.csTime == 0 || item.srTime == 0 || item.ssTime == 0 || item.crTime == 0) {
                overtime = maxOvertime_;
                break;
            }
        }
    }

    if (traceOperator->updateTime + overtime >= TNOW) {
        //todo ,we can set nextTimer here
        return;
    }

    nextTimer = false;
    TLOGDEBUG("trace|" << trace << " overtime, will release in memory" << endl);
    int64_t sgID;
    {
        IGraph sg{};
        graphITraceServer(traceOperator->itrace.get(), sg);
        sgID = hashIGraph(sg);
        auto sgIterator = igraphOperators_.find(sgID);
        if (sgIterator == igraphOperators_.end()) {
            auto graphOperator = std::make_shared<IGraphOperator>();
            mergerIGraph(&sg, "server", graphOperator->igraph.get());
            igraphOperators_[sgID] = graphOperator;
            std::weak_ptr <LogAggregation> weakPtr = shared_from_this();
            addCheckIGraphTimerTask(weakPtr, sgID, 1, 300);
        } else {
            auto graphOperator = sgIterator->second;
            mergerIGraph(&sg, "server", graphOperator->igraph.get());
        }
    }
    int64_t fgID;
    {
        IGraph fg{};
        graphITraceFunction(traceOperator->itrace.get(), fg);
        fgID = hashIGraph(fg);
        auto fgIterator = igraphOperators_.find(fgID);
        if (fgIterator == igraphOperators_.end()) {
            auto graphOperator = std::make_shared<IGraphOperator>();
            mergerIGraph(&fg, "function", graphOperator->igraph.get());
            std::weak_ptr <LogAggregation> weakPtr = shared_from_this();
            addCheckIGraphTimerTask(weakPtr, fgID, 1, 300);
            igraphOperators_[fgID] = graphOperator;
        } else {
            auto graphOperator = fgIterator->second;
            mergerIGraph(&fg, "function", graphOperator->igraph.get());
        }
    }

    traceOperator->itrace->sHash = sgID;
    traceOperator->itrace->fHash = fgID;
    ESWriter::postTrace(file_, trace, traceOperator->itrace, sgID, fgID);
    itraceOperators_.erase(trace);
}

void LogAggregation::checkAndUpdateIGraph(int64_t id, size_t &nextTimer) {
    auto iterator = igraphOperators_.find(id);
    if (iterator == igraphOperators_.end()) {
        throw std::runtime_error("should not happen");
    }
    auto &&graphPtr = iterator->second;
    TLOGDEBUG("will update graph " << id << endl);
    ESWriter::postGraph(file_, id, graphPtr->igraph);
}

void LogAggregation::dump(Snapshot &snapshot) {
    auto &gs = snapshot.graphs;
    for (auto &&item: igraphOperators_) {
        gs[item.first] = *item.second->igraph;
    }

    auto &ts = snapshot.traces;
    for (auto &&item: itraceOperators_) {
        ts[item.first] = *item.second->itrace;
    }
}

void LogAggregation::restore(Snapshot &snapshot) {
    if (snapshot.fileName != file_) {
        return;
    }
    itraceOperators_.clear();
    auto now = TNOW;
    std::weak_ptr <LogAggregation> weakPtr = shared_from_this();

    for (auto &&item: snapshot.traces) {
        auto traceOperator = std::make_shared<ITraceOperator>();
        traceOperator->updateTime = now;
        traceOperator->itrace = std::make_shared<ITrace>(std::move(item.second));
        itraceOperators_[item.first] = traceOperator;

        addCheckITraceTimerTask(weakPtr, item.first, checkCycleTimer_, checkCycleTimer_);
    }

    igraphOperators_.clear();
    for (auto &&item: snapshot.graphs) {
        auto graphOperator = std::make_shared<IGraphOperator>();
        graphOperator->updateTime = now;
        graphOperator->igraph = std::make_shared<IGraph>(std::move(item.second));
        igraphOperators_[item.first] = graphOperator;

        addCheckIGraphTimerTask(weakPtr, item.first, 1, 300);
    }
}
