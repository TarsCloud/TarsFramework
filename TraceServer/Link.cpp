
#include "Link.h"
#include "RawLog.h"
#include <time.h>
#include <stdexcept>

#define ASSIGN_IF_EMPTY(A, B) (A)=A.empty()?(B):(A);

void Span::update(const std::shared_ptr<RawLog> &rawLog) {
    do {
        if (rawLog->type == "ts" || rawLog->type == "cs") {
            ASSIGN_IF_EMPTY(parent, rawLog->parent)
            ASSIGN_IF_EMPTY(master, rawLog->master)
            ASSIGN_IF_EMPTY(slave, rawLog->slave)
            ASSIGN_IF_EMPTY(function, rawLog->function)
            ASSIGN_IF_EMPTY(csData, rawLog->data)
            csTime = rawLog->time;
            break;
        }
        if (rawLog->type == "te" || rawLog->type == "cr") {
            ASSIGN_IF_EMPTY(parent, rawLog->parent)
            ASSIGN_IF_EMPTY(master, rawLog->master)
            ASSIGN_IF_EMPTY(slave, rawLog->slave)
            ASSIGN_IF_EMPTY(function, rawLog->function)
            ASSIGN_IF_EMPTY(ret, rawLog->ret)
            ASSIGN_IF_EMPTY(crData, rawLog->data)
            crTime = rawLog->time;
            break;
        }
        if (rawLog->type == "sr") {
            ASSIGN_IF_EMPTY(slave, rawLog->slave)
            ASSIGN_IF_EMPTY(function, rawLog->function)
            ASSIGN_IF_EMPTY(srData, rawLog->data)
            srTime = rawLog->time;
            break;
        }
        if (rawLog->type == "ss") {
            ASSIGN_IF_EMPTY(slave, rawLog->slave)
            ASSIGN_IF_EMPTY(function, rawLog->function)
            ASSIGN_IF_EMPTY(ret, rawLog->ret)
            ASSIGN_IF_EMPTY(ssData, rawLog->data)
            ssTime = rawLog->time;
            break;
        }
    } while (false);
}

void Trace::push(const std::shared_ptr<RawLog> &rawLog) {
    updateTime = time(nullptr);
    do {
        if (rawLog->type == "ts") {
            tsTime = rawLog->time;
            ASSIGN_IF_EMPTY(tSpan, rawLog->span)
            ASSIGN_IF_EMPTY(tMaster, rawLog->master)
            break;
        }
        if (rawLog->type == "te") {
            teTime = rawLog->time;
            ASSIGN_IF_EMPTY(tSpan, rawLog->span)
            ASSIGN_IF_EMPTY(tMaster, rawLog->master)
            break;
        }
        if (rawLog->type == "cs" || rawLog->type == "cr") {
            auto parentIterator = spans.find(rawLog->parent);
            if (parentIterator == spans.end()) {
                auto parentPtr = std::make_shared<Span>();
                spans[rawLog->parent] = parentPtr;
                parentPtr->children.insert(rawLog->span);
            } else {
                auto parentPtr = parentIterator->second;
                parentPtr->children.insert(rawLog->span);
            }
        }
    } while (false);

    auto spIterator = spans.find(rawLog->span);
    if (spIterator == spans.end()) {
        auto spPtr = std::make_shared<Span>();
        spans[rawLog->span] = spPtr;
        spPtr->update(rawLog);
    } else {
        auto spPtr = spIterator->second;
        spPtr->update(rawLog);
    }
}

InternalGraph Trace::graphServer() {
    if (tSpan.empty()) {
        return {};
    }
    InternalGraph graph;
    InternalVertex vertex{};
    vertex.name = tMaster;
    vertex.record = toRecord(tsTime, teTime);
    graph.vs.insert(vertex);

    graphServer_(tMaster, tSpan, graph);
    return graph;
}

void Trace::graphServer_(const string &parentServer, const string &span, InternalGraph &graph) {
    auto iterator = spans.find(span);
    if (iterator == spans.end()) {
        throw std::runtime_error("span not exist");
    }
    auto ptr = iterator->second;
    InternalVertex vertex{};
    vertex.name = ptr->slave;
    vertex.record = toRecord(ptr->srTime, ptr->ssTime);
    graph.vs.insert(vertex);

    InternalEdge edge{};
    edge.fromVertex = parentServer;
    edge.toVertex = ptr->slave;
    edge.record = toRecord(ptr->csTime, ptr->crTime);
    graph.es.insert(edge);
    for (auto &&child: ptr->children) {
        graphServer_(ptr->slave, child, graph);
    }
}

InternalGraph Trace::graphFunction() {
    if (tSpan.empty()) {
        return {};
    }
    InternalGraph graph{};
    InternalVertex vertex{};
    vertex.name = tMaster;
    vertex.record = toRecord(tsTime, teTime);
    graph.vs.insert(vertex);

    graphFunction_(1, tMaster, tSpan, graph);
    return graph;
}

void Trace::graphFunction_(int order, const string &parentFunction, const string &span, InternalGraph &graph) {
    auto iterator = spans.find(span);
    if (iterator == spans.end()) {
        throw std::runtime_error("span not exist");
    }
    auto ptr = iterator->second;
    auto function = ptr->slave + "." + ptr->function;
    InternalVertex vertex{};
    vertex.name = function;
    vertex.record = toRecord(ptr->srTime, ptr->ssTime);
    graph.vs.insert(vertex);

    InternalEdge edge{};
    edge.fromVertex = parentFunction;
    edge.toVertex = function;
    edge.order = order;
    edge.record = toRecord(ptr->csTime, ptr->crTime);
    edge.span = span;
    edge.csTime = ptr->csTime;
    edge.srTime = ptr->srTime;
    edge.ssTime = ptr->ssTime;
    edge.crTime = ptr->crTime;
    edge.csData = ptr->csData;
    edge.srData = ptr->srData;
    edge.ssData = ptr->ssData;
    edge.crData = ptr->crData;
    edge.ret = ptr->ret;
    graph.es.insert(edge);
    for (auto &&child: ptr->children) {
        graphFunction_(order + 1, function, child, graph);
    }
}

void Link::update(const InternalGraph &g, bool firstUpdate) {
    for (auto &&e: g.es) {
        auto ek = e.fromVertex + "-" + e.toVertex;
        auto iterator = edges.find(ek);
        if (iterator == edges.end()) {
            if (!firstUpdate) {
                throw std::runtime_error("bad graph hash");
            }
            auto edgePtr = std::make_shared<InternalEdge>(e);
            edges[ek] = edgePtr;
            continue;
        }
        auto &&edgePtr = iterator->second;
        edgePtr->record += e.record;
    }
    for (auto &&v: g.vs) {
        auto iterator = vertexes.find(v.name);
        if (iterator == vertexes.end()) {
            if (!firstUpdate) {
                throw std::runtime_error("bad graph hash");
            }
            auto vertexPtr = std::make_shared<InternalVertex>(v);
            vertexes[v.name] = vertexPtr;
            continue;
        }
        auto &&edgePtr = iterator->second;
        edgePtr->record += v.record;
    }
}
