#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <map>
#include <utility>
#include <vector>
#include <cassert>
#include "RawLog.h"
#include "InternGraph.h"
#include "Link.h"
#include <mutex>

class LogAggregation : public enable_shared_from_this<LogAggregation> {
public:
    explicit LogAggregation(std::string file) : file(std::move(file)) {};

    void pushRawLog(const std::shared_ptr<RawLog> &rl);

    void checkAndUpdateTrace(const string &trace, size_t &nextTimer);

    void checkAndUpdateGraph(uint64_t id, size_t &nextTimer);

    void setTimer(size_t first, size_t cycle, size_t closure, size_t max) {
        firstCheckTimer = first;
        checkCycleTimer = cycle;
        closureOvertime = closure;
        maxOvertime = max;
    }

public:
    const std::string file;
    std::mutex traceMutex{};
    std::unordered_map<std::string, std::shared_ptr<Trace>> traces;
    std::unordered_map<uint64_t, std::shared_ptr<Link>> graphs;
    size_t firstCheckTimer{3};
    size_t checkCycleTimer{3};
    size_t closureOvertime{3};
    size_t maxOvertime{100};
};