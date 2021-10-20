#include "TopologyImp.h"
#include "ESReader.h"
#include "Link.h"

static void transformGraph(const InternalGraph &internalGraph, Graph &graph, bool copySpanInfo) {
    graph.es.reserve(internalGraph.es.size());
    for (auto &&item: internalGraph.es) {
        auto bes = Edge{};
        bes.fromVertex = item.fromVertex;
        bes.toVertex = item.toVertex;
        bes.callCount = getCountFromRecord(item.record);
        bes.callTime = getTimeFromRecord(item.record);
        if (copySpanInfo) {
            bes.spanId = item.span;
            bes.csTime = item.csTime;
            bes.srTime = item.srTime;
            bes.ssTime = item.ssTime;
            bes.crTime = item.crTime;
            bes.csData = item.csData;
            bes.srData = item.srData;
            bes.ssData = item.ssData;
            bes.crData = item.crData;
            bes.ret = item.ret;
        }
        graph.es.emplace_back(bes);
    }

    graph.vs.reserve(internalGraph.vs.size());
    for (auto &&item: internalGraph.vs) {
        auto bvs = Vertex{};
        bvs.vertex = item.name;
        bvs.callCount = getCountFromRecord(item.record);
        bvs.callTime = getTimeFromRecord(item.record);
        graph.vs.emplace_back(bvs);
    }
}

Int32 TopologyImp::graphFunction(const string &date, const string &functionName, vector<Graph> &graph, CurrentPtr current) {
    std::vector<InternalGraph> igs{};
    if (ESReader::getFunctionGraph(date, functionName, igs) != 0) {
        return -1;
    }
    graph.resize(igs.size());
    for (auto i = 0; i < igs.size(); ++i) {
        transformGraph(igs[i], graph[i], false);
    }
    return 0;
}

Int32 TopologyImp::graphServer(const string &date, const string &serverName, vector<Graph> &graph, CurrentPtr current) {
    std::vector<InternalGraph> igs{};
    if (ESReader::getServerGraph(date, serverName, igs) != 0) {
        return -1;
    }
    graph.resize(igs.size());
    for (auto i = 0; i < igs.size(); ++i) {
        transformGraph(igs[i], graph[i], false);
    }
    return 0;
}

Int32 TopologyImp::listFunction(const string &date, const string &serverName, vector<std::string> &fs, CurrentPtr current) {
    std::set<std::string> sfs{};
    if (ESReader::listFunction(date, serverName, sfs) != 0) {
        return -1;
    }
    fs.insert(fs.begin(), sfs.begin(), sfs.end());
    return 0;
}

Int32 TopologyImp::listTrace(const string &date, const string &serverName, vector<std::string> &ts, CurrentPtr current) {
    return ESReader::listTrace(date, 0, INT64_MAX, serverName, ts);
}

Int32
TopologyImp::listTraceSummary(const string &date, Int64 beginTime, Int64 endTime, const string &serverName, vector<Summary> &ss,
                              CurrentPtr current) {
    return ESReader::listTraceSummary(date, beginTime, endTime, serverName, ss);
}

Int32 TopologyImp::graphTrace(const string &date, const string &traceId, Graph &graph, CurrentPtr current) {
    InternalGraph ig;
    if (ESReader::getTraceGraph(date, traceId, ig) != 0) {
        return -1;
    }
    transformGraph(ig, graph, true);
    return 0;
}
