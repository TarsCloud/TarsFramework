#include <set>

#pragma once

struct InternalVertex {
    std::string name{};
    int64_t record;

    bool operator<(const InternalVertex &r) const {
        return name < r.name;
    }
};

struct InternalEdge {
    std::string fromVertex{};
    std::string toVertex{};
    int64_t record{};
    std::string span{};
    int64_t csTime{};
    int64_t srTime{};
    int64_t ssTime{};
    int64_t crTime{};
    std::string csData{};
    std::string srData{};
    std::string ssData{};
    std::string crData{};
    std::string ret;
    int order{};

    bool operator<(const InternalEdge &r) const {
        if (order < r.order) return true;
        if (order > r.order) return false;

        if (csTime < r.csTime) return true;
        if (csTime > r.csTime) return false;

        if (fromVertex < r.fromVertex) return true;
        if (fromVertex > r.fromVertex) return false;

        if (toVertex < r.toVertex) return true;
        if (toVertex > r.toVertex) return false;

        return false;
    }
};

struct InternalGraph {
    std::set<InternalVertex> vs;
    std::set<InternalEdge> es;
};