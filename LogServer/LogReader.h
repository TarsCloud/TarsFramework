#pragma once


#include <memory>
#include <vector>
#include "RawLog.h"
#include <string>

using namespace std;

constexpr size_t MAX_READ_SIZE = 1024 * 1024 * 2;
constexpr size_t MAX_LINE_SIZE = 1024 * 100;

class LogReader {
public:
    explicit LogReader(const string &file);

    ~LogReader() { ::fclose(fd_); }

    const vector<shared_ptr<RawLog>> &read();

    string file() const {
        return file_;
    }

private:
    void splitLine();

    void parseLine();

    bool reopenFD();

    const void * memmem(const void *l, size_t l_len, const void *s, size_t s_len);

private:
    const string file_;
    char buff_[MAX_READ_SIZE]{};
    char line_[MAX_LINE_SIZE]{};
    const char *data_begin_{};
    const char *data_end_{};
    size_t input_pos_{};
    vector<shared_ptr<RawLog>> v_{};
    size_t readSeek_{};
    FILE* fd_{NULL};
    time_t last_read_time_{};
};