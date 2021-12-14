#pragma once

#include <string>
#include <vector>
#include "TraceData.h"
#include "Topology.h"

using namespace internal;

class ESReader
{
public:
	static int listFunction(const string& date, const string& serverName, set<string>& fs);

	static int listTrace(const string& date, int64_t beginTime, int64_t endTime, const string& serverName, vector<string>& ts);

	static int listTraceSummary(const string& date, int64_t beginTime, int64_t endTime, const string& serverName, vector<Summary>& ss);

	static int getTrace(const string& date, const string& traceName, ITrace& trace);

	static int getFunctionGraph(const string& date, const string& functionName, vector<IGraph>& graph);

	static int getServerGraph(const string& date, const string& serverName, vector<IGraph>& graph);
};
