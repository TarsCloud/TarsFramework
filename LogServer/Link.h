#pragma once

#include <cstdint>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>
#include "InternGraph.h"
#include "RawLog.h"

using namespace std;

struct Span { ;
    string parent{};
    string master{};
    string slave{};
    string function{};
    int64_t csTime{};
    int64_t crTime{};
    int64_t ssTime{};
    int64_t srTime{};
    string csData{};
    string srData{};
    string ssData{};
    string crData{};
    string ret{};
    set<string> children{};

    void update(const shared_ptr<RawLog> &rawLog);
};

struct Trace {
    map<string, shared_ptr<Span>> spans;
    string tSpan;
    string tMaster;
    int64_t tsTime{};
    int64_t teTime{};
    time_t updateTime{};
public:
    void push(const shared_ptr<RawLog> &rawLog);

    InternalGraph graphServer();;

    InternalGraph graphFunction();;

private:
    void graphServer_(const string &parentServer, const string &span, InternalGraph &graph);

    void graphFunction_(int order, const string &parentFunction, const string &span, InternalGraph &graph);
};

struct Link {
    explicit Link(std::string type) : type(std::move(type)) {};
    std::string type;
    std::map<std::string, std::shared_ptr<InternalVertex>> vertexes{};
    std::map<std::string, std::shared_ptr<InternalEdge>> edges{};
public:
    void update(const InternalGraph &g, bool firstUpdate);
};

constexpr uint64_t COUNT_MASK = 40u;

inline int64_t getCountFromRecord(int64_t record) {
    return record >> COUNT_MASK;
}

inline int64_t getTimeFromRecord(int64_t record) {
    constexpr uint64_t OFFSET = 64ul - COUNT_MASK;
    return record << OFFSET >> OFFSET;
}

inline int64_t toRecord(int64_t beginTime, int64_t endTime) {
    if (beginTime == 0 || endTime == 0 || endTime < beginTime) {
        return 0;
    }
    return 1ll << COUNT_MASK | (endTime - beginTime);
}