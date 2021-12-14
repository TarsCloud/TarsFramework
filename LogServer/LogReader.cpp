
#include "LogReader.h"
#include "servant/RemoteLogger.h"
#include <cstring>
#include <utility>

LogReader::LogReader(string file) : file_(std::move(file))
{
}

bool LogReader::reopenFD()
{
	if (fd_ != nullptr)
	{
		return true;
	}
	if (TC_File::isFileExist(file_))
	{
		fd_ = TC_Port::fopen(file_.c_str(), "rb");
		lastReadTime_ = TNOW;
		if (fd_ == nullptr)
		{
			auto err = std::string("open file ").append(file_).append("error: ").append(strerror(errno));
			return false;
		}
		if (::fseek(fd_, 0, SEEK_END) != 0)
		{
			auto err = std::string("fseek file ").append(file_).append("error: ").append(strerror(errno));
			throw std::runtime_error(err);
		}
		auto fileSize = ftell(fd_);
		if (readSeek_ > fileSize)
		{
			readSeek_ = 0;  //file had truncated;
			inputPos_ = 0;
		}
		if (readSeek_ < fileSize)
		{
			::fseek(fd_, readSeek_, SEEK_SET);
		}
		return true;
	}
	fd_ = nullptr;
	return false;
}

const std::vector<std::shared_ptr<IRawLog>>& LogReader::read()
{
	v_.clear();
	if (!reopenFD())
	{
		return v_;
	}
	ssize_t read_size = ::fread(buff_ + inputPos_, 1, MAX_READ_SIZE - inputPos_, fd_);
	time_t now = TNOW;
	if (read_size == 0)
	{
		if (lastReadTime_ + 60 < now)
		{
			TLOGERROR("will reopen file|" << file_ << endl);
			assert(readSeek_ >= inputPos_);
			readSeek_ -= inputPos_;
			inputPos_ = 0;
			::fclose(fd_);
			fd_ = nullptr;
		}
		return v_;
	}
	lastReadTime_ = now;
	readSeek_ += read_size;
	dataBegin_ = buff_;
	dataEnd_ = buff_ + inputPos_ + read_size;
	splitLine();
	return v_;
}

const void* LogReader::memmem(const void* l, size_t l_len, const void* s, size_t s_len)
{
	register char* cur, * last;
	const char* cl = (const char*)l;
	const char* cs = (const char*)s;

	if (l_len == 0 || s_len == 0)
	{
		return nullptr;
	}

	if (l_len < s_len)
	{
		return nullptr;
	}

	if (s_len == 1)
	{
		return memchr(l, (int)*cs, l_len);
	}

	last = (char*)cl + l_len - s_len;

	for (cur = (char*)cl; cur <= last; cur++)
	{
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
			return cur;
	}

	return nullptr;

}

void LogReader::splitLine()
{
	constexpr char LINE_END_FLAG[2] = { '|', '\n' };
	constexpr char LINE_END_FLAG_LENGTH = sizeof(LINE_END_FLAG);
	const char* line_begin = dataBegin_;
	while (true)
	{
		if (line_begin >= dataEnd_)
		{
			inputPos_ = 0;
			break;
		}
		size_t left_size = dataEnd_ - line_begin;
		const char* line_end_flag = static_cast<const char*>(LogReader::memmem(line_begin, left_size, LINE_END_FLAG, LINE_END_FLAG_LENGTH));
		if (line_end_flag == nullptr)
		{
			memmove(buff_, line_begin, left_size);
			inputPos_ = left_size;
			break;
		}
		const char* next_line = line_end_flag + LINE_END_FLAG_LENGTH;
		size_t line_size = next_line - line_begin;
		memset(line_ + line_size, '\0', 10);
		memcpy(line_, line_begin, std::min(line_size, MAX_LINE_SIZE));
		parseLine();
		line_begin = next_line;
	}
}

void LogReader::parseLine()
{
	auto ptr = std::make_shared<IRawLog>();
	const char* needle = "|";
	const char* next = line_;
	const char* buff_end = line_ + MAX_LINE_SIZE;
	for (auto i = 0; i < 14; ++i)
	{
		const char* end = static_cast<const char*>(LogReader::memmem(next, buff_end - next, needle, 1));
		if (end == nullptr)
		{
			throw std::runtime_error(std::string("unexpected string: ").append(line_));
		}
		switch (i)
		{
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
		case 9:
		{
			ptr->slave = std::string(next, end);
			size_t pointCount = 0;
			size_t pos = 0;
			while (true)
			{
				pos = ptr->slave.find('.', pos + 1);
				if (pos == std::string::npos)
				{
					break;
				}
				pointCount++;
				if (pointCount == 2)
				{
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
		default:
		{
		}
		}
		next = end + 1;
	}
	v_.emplace_back(ptr);
}

void LogReader::dump(Snapshot& snapshot)
{
	snapshot.fileName = file_;
	assert(readSeek_ >= inputPos_);
	snapshot.seek = readSeek_ - inputPos_;
}

void LogReader::restore(Snapshot& snapshot)
{
	if (snapshot.fileName != file_)
	{
		return;
	}
	readSeek_ = snapshot.seek;
	inputPos_ = 0;
}
