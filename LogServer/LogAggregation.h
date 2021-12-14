#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <map>
#include <utility>
#include <vector>
#include <cassert>
#include <mutex>
#include "TraceData.h"

using namespace internal;
struct ITraceOperator;
struct IGraphOperator;

class LogAggregation : public enable_shared_from_this<LogAggregation>
{
public:
	explicit LogAggregation(std::string file) : file_(std::move(file))
	{
	};

	void pushRawLog(const std::shared_ptr<IRawLog>& rl);

	void checkAndUpdateITrace(const string& trace, size_t& nextTimer);

	void checkAndUpdateIGraph(int64_t id, size_t& nextTimer);

	void setTimer(size_t first, size_t cycle, size_t closure, size_t max)
	{
		firstCheckTimer_ = first;
		checkCycleTimer_ = cycle;
		closureOvertime_ = closure;
		maxOvertime_ = max;
	}

	void dump(Snapshot& snapshot);

	void restore(Snapshot& snapshot);

private:
	const std::string file_;
	std::unordered_map<std::string, std::shared_ptr<ITraceOperator>> itraceOperators_;
	std::unordered_map<int64_t, std::shared_ptr<IGraphOperator>> igraphOperators_{};
	size_t firstCheckTimer_{ 3 };
	size_t checkCycleTimer_{ 3 };
	size_t closureOvertime_{ 3 };
	size_t maxOvertime_{ 100 };
};