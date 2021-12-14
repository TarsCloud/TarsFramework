
#pragma once

#include <map>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <functional>

class TimerTaskQueue
{

private:
	TimerTaskQueue() = default;

public:
	static TimerTaskQueue& instance()
	{
		static TimerTaskQueue timerTaskQueue;
		return timerTaskQueue;
	}

	void pushTask(std::function<void()> t)
	{
		pushTimerTask(std::move(t), 0);
	}

	void pushTimerTask(std::function<void()> t, size_t seconds)
	{
		uint64_t key = time(nullptr) + seconds;
		key <<= 32u;
		key += seq_++;
		std::lock_guard<std::mutex> lockGuard(mutex_);
		map_[key] = std::move(t);
		condition_.notify_one();
	}

	void pushCycleTask(const std::function<void(const size_t& callTimes, size_t& nextTimer)>& t, size_t first, size_t cycle)
	{
		size_t callTimes = 0;
		auto cycleFun = [callTimes, t, cycle, this]()
		{
			_runCycleTask(t, callTimes, cycle);
		};
		pushTimerTask(cycleFun, first);
	}

	[[noreturn]] void run()
	{
		while (true)
		{
			std::unique_lock<std::mutex> uniqueLock(mutex_);
			while (!map_.empty())
			{
				auto iterator = map_.begin();
				uint64_t key = iterator->first;
				uint32_t runTimer = (key >> 32u);
				time_t nowTimer = time(nullptr);
				if (nowTimer >= runTimer)
				{
					auto fun = std::move(iterator->second);
					map_.erase(iterator);
					uniqueLock.unlock();
					fun();
					uniqueLock.lock();
				}
				else
				{
					size_t mapSize = map_.size();
					condition_.wait_for(uniqueLock, std::chrono::seconds(runTimer - nowTimer), [mapSize, this]()
					{
						return mapSize != map_.size();
					});
				}
			}
			condition_.wait(uniqueLock, [this]()
			{ return !map_.empty(); });
		}
	}

private:
	void _runCycleTask(const std::function<void(const size_t& callTimes, size_t& nextTimer)>& t, size_t callTimes, size_t cycle)
	{
		t(callTimes, cycle);
		if (cycle != 0)
		{
			callTimes += 1;
			pushTimerTask([callTimes, cycle, t, this]()
			{
				_runCycleTask(t, callTimes, cycle);
			}, cycle);
		}
	}

private:
	std::atomic<uint32_t> seq_{ 0 };
	std::mutex mutex_;
	std::condition_variable condition_;
	std::map<uint64_t, std::function<void()>> map_;
};
