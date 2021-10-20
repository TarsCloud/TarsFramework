
#pragma once

#include <map>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <functional>

class TimerTaskQueue {

private:
    TimerTaskQueue() = default;

public:
    static TimerTaskQueue &instance() {
        static TimerTaskQueue timerTaskQueue;
        return timerTaskQueue;
    }

    void pushTask(std::function<void()> t) {
        pushTimerTask(std::move(t), 0);
    }

    void pushTimerTask(std::function<void()> t, size_t seconds) {
        uint64_t key = time(nullptr) + seconds;
        key <<= 32u;
        key += _seq++;
        std::lock_guard<std::mutex> lockGuard(_mutex);
        _map[key] = std::move(t);
        _condition.notify_one();
    }

    void pushCycleTask(const std::function<void(const size_t &callTimes, size_t &nextTimer)> &t, size_t first, size_t cycle) {
        size_t callTimes = 0;
        auto cycleFun = [callTimes, t, cycle, this]() {
            _runCycleTask(t, callTimes, cycle);
        };
        pushTimerTask(cycleFun, first);
    }

    [[noreturn]] void run() {
        while (true) {
            std::unique_lock<std::mutex> uniqueLock(_mutex);
            while (!_map.empty()) {
                auto iterator = _map.begin();
                uint64_t key = iterator->first;
                uint32_t runTimer = (key >> 32u);
                time_t nowTimer = time(nullptr);
                if (nowTimer >= runTimer) {
                    auto fun = std::move(iterator->second);
                    _map.erase(iterator);
                    uniqueLock.unlock();
                    fun();
                    uniqueLock.lock();
                } else {
                    size_t mapSize = _map.size();
                    _condition.wait_for(uniqueLock, std::chrono::seconds(runTimer - nowTimer), [mapSize, this]() {
                        return mapSize != _map.size();
                    });
                }
            }
            _condition.wait(uniqueLock, [this]() { return !_map.empty(); });
        }
    }

private:
    void _runCycleTask(const std::function<void(const size_t &callTimes, size_t &nextTimer)> &t, size_t callTimes, size_t cycle) {
        t(callTimes, cycle);
        if (cycle != 0) {
            callTimes += 1;
            pushTimerTask([callTimes, cycle, t, this]() {
                _runCycleTask(t, callTimes, cycle);
            }, cycle);
        }
    }

private:
    std::atomic<uint32_t> _seq{0};
    std::mutex _mutex;
    std::condition_variable _condition;
    std::map<uint64_t, std::function<void()>> _map;
};
