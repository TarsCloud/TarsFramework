#pragma once

#include <memory>
#include <vector>
#include "TraceData.h"

using namespace std;
using namespace internal;

class ESWriter
{
public:

	static void createIndexTemplate();

	static void postRawLog(const string& logFile, const vector<shared_ptr<IRawLog>>& rawLogs, size_t triedTimes = 0);

	static void
	postTrace(const string& file, const string& traceName, const shared_ptr<ITrace>& tracePtr, uint64_t shash, uint64_t fhash, size_t triedTimes = 0);

	static void postGraph(const string& file, uint64_t graphHash, const shared_ptr<IGraph>& graph);
};