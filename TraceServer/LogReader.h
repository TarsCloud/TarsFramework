#pragma once


#include <memory>
#include <vector>
#include "RawLog.h"

using namespace std;
constexpr ssize_t MAX_READ_SIZE = 1024 * 1024 * 2;
constexpr ssize_t MAX_LINE_SIZE = 1024 * 100;

class LogReader {
public:
    explicit LogReader(string file);

    ~LogReader() { close(fd_); }

    const vector<shared_ptr<RawLog>> &read();

    string file() const {
        return file_;
    }

private:
    void splitLine();

    void parseLine();

    bool reopenFD();

private:
    const string file_;
    char buff_[MAX_READ_SIZE]{};
    char line_[MAX_LINE_SIZE]{};
    const char *data_begin_{};
    const char *data_end_{};
    size_t input_pos_{};
    vector<shared_ptr<RawLog>> v_{};
    ssize_t readSeek_{};
    int fd_{-1};
    time_t last_read_time{};
};