#pragma once

#include <string>
#include <memory>
#include <cstring>
// #include <unistd.h>

struct RawLog {
    std::string line;
    std::string trace;
    std::string span;
    std::string parent;
    std::string type;
    std::string master;
    std::string slave;
    std::string function;
    int64_t time{};
    std::string ret;
    std::string data;
};
