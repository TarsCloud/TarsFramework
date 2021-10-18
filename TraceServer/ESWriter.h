#pragma once

#include <memory>
#include <vector>
#include "RawLog.h"
#include "InternGraph.h"
#include "Link.h"

using namespace std;

class ESWriter {
public:

    static void createIndexTemplate();

    static void postRawLog(const string &logFile, const vector<shared_ptr<RawLog>> &rawLogs, size_t triedTimes = 0);

    static void
    postTrace(const string &file, const std::string &traceName, const shared_ptr<Trace> &tracePtr, uint64_t serverGraphId, uint64_t functionGraphID,
              size_t triedTimes = 0);

    static void postLink(const string &file, uint64_t linkId, const shared_ptr<Link> &linkPtr);
};