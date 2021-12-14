
#include "LogReader.h"
#include "LogAggregation.h"
#include "TraceData.h"
#include "TraceService.h"

class SnapshotDraftsman
{
public:
	static void createSnapshot(const std::shared_ptr<LogReader>& logReader, const std::shared_ptr<LogAggregation>& logAggregation);

	static void restoreSnapshot(std::shared_ptr<LogReader>& logReader, std::shared_ptr<LogAggregation>& logAggregation);

	static void setSavePath(const std::string& path);
};