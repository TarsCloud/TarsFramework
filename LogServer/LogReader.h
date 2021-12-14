#pragma once


#include <memory>
#include <vector>
#include <string>
#include "TraceData.h"

using namespace std;
using namespace internal;

constexpr size_t MAX_READ_SIZE = 1024 * 1024 * 5;
constexpr size_t MAX_LINE_SIZE = 1024 * 100;

class LogReader
{
public:
	explicit LogReader(string file);

	~LogReader()
	{
		::fclose(fd_);
	}

	const vector<shared_ptr<IRawLog>>& read();

	string file() const
	{
		return file_;
	}

	void dump(Snapshot& snapshot);

	void restore(Snapshot& snapshot);

private:
	void splitLine();

	void parseLine();

	bool reopenFD();

	const void* memmem(const void* l, size_t l_len, const void* s, size_t s_len);

private:
	const string file_;
	char buff_[MAX_READ_SIZE]{};
	char line_[MAX_LINE_SIZE]{};
	const char* dataBegin_{};
	const char* dataEnd_{};
	ssize_t inputPos_{};
	vector<shared_ptr<IRawLog>> v_{};
	ssize_t readSeek_{};
	FILE* fd_{ nullptr };
	time_t lastReadTime_{};
};