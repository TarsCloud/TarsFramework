#pragma once

#include <string>
#include <vector>
#include "InternGraph.h"
#include "THead.h"

class ESReader {
public:
    static int listFunction(const string &date, const string &serverName, set<string> &fs);

    static int listTrace(const string &date, int64_t beginTime, int64_t endTime, const string &serverName, vector<string> &ts);

    static int listTraceSummary(const string &date, int64_t beginTime, int64_t endTime, const string &serverName, vector<Summary> &ss);

    static int getTraceGraph(const std::string &date, const std::string &traceName, InternalGraph &graph);

    static int getFunctionGraph(const string &date, const string &functionName, vector<InternalGraph> &graph);

    static int getServerGraph(const string &date, const string &serverName, vector<InternalGraph> &graph);
};
