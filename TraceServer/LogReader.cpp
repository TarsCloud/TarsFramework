
#include "LogReader.h"
#include "servant/RemoteLogger.h"
#include <string.h>

LogReader::LogReader(const string &file) : file_(file) {
}

bool LogReader::reopenFD() {
    if (fd_ != NULL) {
        return true;
    }
    if (TC_File::isFileExist(file_)) {
        fd_ = TC_Port::fopen(file_.c_str(), "rb");
        // fd_ = ::open(file_.c_str(), O_RDONLY | O_SYNC);
        if (fd_ == NULL) {
            auto err = std::string("open file ").append(file_).append("error");
            throw std::runtime_error(err);
        }
        
        auto fileSize = ::fseek(fd_, 0, SEEK_END);
        if (readSeek_ > fileSize) {
            readSeek_ = 0;  //file had truncated;
        }
        if (readSeek_ < fileSize) {
            ::fseek(fd_, readSeek_, SEEK_SET);
        }
        return true;
    }
    fd_ = NULL;
    return false;
}

const std::vector<std::shared_ptr<RawLog>> &LogReader::read() {
    v_.clear();
    if (!reopenFD()) {
        return v_;
    }
    size_t read_size = ::fread(buff_ + input_pos_, 1, MAX_READ_SIZE - input_pos_, fd_);
    if (read_size == 0) {
        time_t now = time(nullptr);
        if (last_read_time + 60 < now) {
            ::fclose(fd_);
            fd_ = NULL;
        }
        return v_;
    }
    if (read_size < 0) {
        ::fclose(fd_);
        fd_ = NULL;
        return v_;
    }
    readSeek_ += read_size;
    data_begin_ = buff_;
    data_end_ = buff_ + input_pos_ + read_size;
    splitLine();
    return v_;
}

const void * LogReader::memmem(const void *l, size_t l_len, const void *s, size_t s_len)
{
    register char *cur, *last;
    const char *cl = (const char *)l;
    const char *cs = (const char *)s;

    if(l_len == 0 || s_len == 0)
    {
        return NULL;
    }

    if(l_len < s_len)
    {
        return NULL;
    }

    if(s_len == 1) {
        return memchr(l, (int)*cs, l_len);
    }

    last = (char *)cl + l_len - s_len;

    for(cur = (char *)cl; cur <= last; cur++)
    {
        if(cur[0]==cs[0] && memcmp(cur, cs, s_len) == 0)
            return cur;
    }

    return NULL;

}
void LogReader::splitLine() {
    constexpr char LINE_END_FLAG[2] = {'|', '\n'};
    constexpr char LINE_END_FLAG_LENGTH = sizeof(LINE_END_FLAG);
    const char *line_begin = data_begin_;
    while (true) {
        if (line_begin >= data_end_) {
            input_pos_ = 0;
            break;
        }
        size_t left_size = data_end_ - line_begin;
        const char *line_end_flag = static_cast<const char *>(LogReader::memmem(line_begin, left_size, LINE_END_FLAG, LINE_END_FLAG_LENGTH));
        if (line_end_flag == nullptr) {
            memmove(buff_, line_begin, left_size);
            input_pos_ = left_size;
            break;
        }
        const char *next_line = line_end_flag + LINE_END_FLAG_LENGTH;
        size_t line_size = next_line - line_begin;
        memset(line_ + line_size, '\0', 10);
        memcpy(line_, line_begin, std::min(line_size, MAX_LINE_SIZE));
        parseLine();
        line_begin = next_line;
    }
}

void LogReader::parseLine() {
    auto ptr = std::make_shared<RawLog>();
    ptr->line = line_;
    const char *needle = "|";
    const char *next = line_;
    const char *buff_end = line_ + MAX_LINE_SIZE;
    for (auto i = 0; i < 14; ++i) {
        const char *end = static_cast<const char *>(LogReader::memmem(next, buff_end - next, needle, 1));
        if (end == nullptr) {
            throw std::runtime_error(std::string("unexpected string: ").append(line_));
        }
        switch (i) {
            case 4:
                ptr->trace = std::string(next, end);
                break;
            case 5:
                ptr->span = std::string(next, end);
                break;
            case 6:
                ptr->parent = std::string(next, end);
                break;
            case 7:
                ptr->type = std::string(next, end);
                break;
            case 8:
                ptr->master = std::string(next, end);
                break;
            case 9: {
                ptr->slave = std::string(next, end);
                size_t pointCount = 0;
                size_t pos = 0;
                while (true) {
                    pos = ptr->slave.find('.', pos + 1);
                    if (pos == std::string::npos) {
                        break;
                    }
                    pointCount++;
                    if (pointCount == 2) {
                        ptr->slave = ptr->slave.substr(0, pos);
                        break;
                    }
                }
            }
                break;
            case 10:
                ptr->function = std::string(next, end);;
                break;
            case 11:
                ptr->time = strtoll(next, nullptr, 10);
                break;
            case 12:
                ptr->ret = std::string(next, end);;
                break;
            case 13:
                ptr->data = std::string(next, end);;
                break;
            default: {
            }
        }
        next = end + 1;
    }
    v_.emplace_back(ptr);
}
