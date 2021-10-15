
#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <asio/ip/tcp.hpp>
#include <asio/streambuf.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/connect.hpp>
#include <queue>
#include <mutex>
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <asio/posix/stream_descriptor.hpp>
#include <condition_variable>
#include "HttpParser.h"

enum class ESClientRequestMethod {
    Put,
    Post,
    Get,
};

enum ESClientRequestPhase {
    Pending,
    Running,
    Done,
    Error,
    Cancel,
};

class ESClientRequestEntry;

class ESClientRequest {
    friend class ESClient;

    friend class ESClientWorker;

public:
    void waitFinish() {
        std::unique_lock<std::mutex> uniqueLock(mutex_);
        return conditionVariable_.wait(uniqueLock, [this]() -> bool {
            return phase_ == Error || phase_ == Done;
        });
    }

    template<typename Rep, typename Period>
    bool waitFinish(const std::chrono::duration<Rep, Period> &time) {
        std::unique_lock<std::mutex> uniqueLock(mutex_);
        return conditionVariable_.wait_for(uniqueLock, time, [this]() -> bool {
            return phase_ == Error || phase_ == Done;
        });
    }

    const std::atomic<int> &phase() const {
        return phase_;
    }

    const std::string &phaseMessage() const {
        return phaseMessage_;
    }

    const char *responseBody() const;

    size_t responseSize() const;

    unsigned int responseCode() const;

private:
    void setPhase(int phase, std::string phaseMessage) {
        std::unique_lock<std::mutex> uniqueLock(mutex_);
        phase_ = phase;
        phaseMessage_.swap(phaseMessage);
        conditionVariable_.notify_all();
    }

private:

    ESClientRequest() = default;

    std::atomic<int> phase_{ESClientRequestPhase::Pending};
    std::string phaseMessage_;
    std::mutex mutex_;
    std::condition_variable conditionVariable_;
    std::shared_ptr<ESClientRequestEntry> entry_;
};

class ESClientWorker;

class ESClient {
public:
    static ESClient &instance() {
        static ESClient esClient;
        return esClient;
    }

    static void setServer(std::string host, int port) {
        ESHost = std::move(host);
        ESPort = port;
    }

    void start();

    void stop() {
        ioContext_.stop();
    }

    std::shared_ptr<ESClientRequest> postRequest(ESClientRequestMethod method, const std::string &url, const std::string &body);

private:
    static std::string buildPostRequest(const std::string &url, const std::string &body);

    static std::string buildPutRequest(const std::string &url, const std::string &body);

    static std::string buildGetRequest(const std::string &url, const std::string &body);

    ESClient() = default;

private:
    static std::string ESHost;
    static int ESPort;
    asio::io_context ioContext_{1};
    asio::posix::stream_descriptor eventStream_{ioContext_};
    std::queue<std::shared_ptr<ESClientRequest>> pendingQueue_{};
    std::vector<std::shared_ptr<ESClientWorker>> sessionVector_{};
    std::thread thread_;
};