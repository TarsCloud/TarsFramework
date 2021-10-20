#pragma once

#include "LogAggregation.h"
#include "LogReader.h"
#include <servant/Application.h>

class TraceService
{
  protected:
    struct LogReadAggregationPair
    {
        explicit LogReadAggregationPair(const std::string &logFile) : logReader(logFile)
        {
            aggregation = std::make_shared<LogAggregation>(logFile);
        };
        LogReader logReader;
        std::shared_ptr<LogAggregation> aggregation{};
    };

  public:
    TraceService() = default;

    bool loadTimerValue(const TC_Config &config, std::string &message)
    {
        firstCheckTimer = TC_Common::strto<int>(config.get("/tars/trace<FirstCheckTimer>", "3"));
        checkCycleTimer = TC_Common::strto<int>(config.get("/tars/trace<CheckCycleTimer>", "3"));
        closureOvertime = TC_Common::strto<int>(config.get("/tars/trace<OvertimeWhenClosure>", "3"));
        maxOvertime = TC_Common::strto<int>(config.get("/tars/trace<Overtime>", "100"));

        if (firstCheckTimer < 1 || firstCheckTimer > 600)
        {
            message = "FirstCheckTimer value should in [1,600]";
            return false;
        }

        if (checkCycleTimer < 1 || checkCycleTimer > 100)
        {
            message = "CheckCycleTimer value should in [1,100]";
            return false;
        }

        if (closureOvertime < 1 || closureOvertime > 100)
        {
            message = "OvertimeWhenClosure value should in [1,100]";
            return false;
        }

        if (maxOvertime < 100 || maxOvertime > 600)
        {
            message = "Overtime value should in [100,600]";
            return false;
        }

        for (auto &pair : pairs_)
        {
            pair.second->aggregation->setTimer(firstCheckTimer, checkCycleTimer, closureOvertime, maxOvertime);
        }

        message = "done";
        return true;
    }

    void initialize(TC_Config &conf);

  private:
    [[noreturn]] void watchFile();

    void onModify(const std::string &file);

    bool isExpectedFile(const std::string &file)
    {
        return file == buildLogFileName(false);
    }

    std::string buildLogFileName(bool abs)
    {
        return (abs ? (logDir_ + "/") : "") + logPrefix_ + TC_Common::nowdate2str() + ".log";
    }

    std::string getAbsLogFileName(const std::string &name)
    {
        return logDir_ + "/" + name;
    }

  private:
    std::string logDir_{};
    std::string logPrefix_{"_tars_._trace___t_trace__"};
    std::thread watchThread_;
    std::thread timerThread_;
    std::map<std::string, std::shared_ptr<LogReadAggregationPair>> pairs_;

    size_t firstCheckTimer{3};
    size_t checkCycleTimer{3};
    size_t closureOvertime{3};
    size_t maxOvertime{100};
};