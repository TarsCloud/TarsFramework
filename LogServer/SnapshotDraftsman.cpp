

#include "SnapshotDraftsman.h"
#include "servant/RemoteLogger.h"

static std::string snapshotSaveDir_;

static std::string buildSnapFilePath(const std::string& logFile)
{
	assert(!snapshotSaveDir_.empty());
	if (!TC_File::makeDirRecursive(snapshotSaveDir_))
	{
		TLOGERROR("create snapshot save dir|" << snapshotSaveDir_ << "error" << endl;);
		throw std::runtime_error("create snapshot save dir error");
	}
	if (*snapshotSaveDir_.rbegin() == '/')
	{
		return snapshotSaveDir_ + TC_File::excludeFileExt(TC_File::extractFileName(logFile)) + ".snap";
	}
	return snapshotSaveDir_ + "/" + TC_File::excludeFileExt(TC_File::extractFileName(logFile)) + ".snap";
}

struct SelfReleaseThread : public std::enable_shared_from_this<SelfReleaseThread>
{
	void run(const std::function<void()>& f)
	{
		auto self = shared_from_this();
		thread_ = std::thread([self, f]()
				{
					f();
				}
		);
		thread_.detach();
	}

private:
	std::thread thread_;
};

void SnapshotDraftsman::createSnapshot(const std::shared_ptr<LogReader>& logReader, const std::shared_ptr<LogAggregation>& logAggregation)
{
	auto snapshot = std::make_shared<Snapshot>();
	logReader->dump(*snapshot);
	logAggregation->dump(*snapshot);
	auto newThread = std::make_shared<SelfReleaseThread>();
	newThread->run([snapshot]
	{
		TarsOutputStream<BufferWriter> os;
		snapshot->writeTo(os);
		auto snapFile = buildSnapFilePath(snapshot->fileName);
		TLOGINFO("snapshot file path: " << snapFile << endl;);
		TC_File::save2file(snapFile, std::string(os.getBuffer(), os.getLength()));
		TLOGINFO("snapshot success, path: " << snapFile << endl);
	});
}

void SnapshotDraftsman::restoreSnapshot(shared_ptr<LogReader>& logReader, shared_ptr<LogAggregation>& logAggregation)
{
	auto path = buildSnapFilePath(logReader->file());
	auto snapshotBody = TC_File::load2str(path);
	if (snapshotBody.empty())
	{
		TLOGERROR("got empty snapshot content, maybe snapshot not exist or something wrong" << endl);
		return;
	}
	TarsInputStream<BufferReader> in;
	in.setBuffer(snapshotBody.c_str(), snapshotBody.size());

	try
	{
		Snapshot snapshot{};
		snapshot.readFrom(in);
		logReader->restore(snapshot);
		logAggregation->restore(snapshot);
		TLOGERROR("restore from snapshot success, read seek is " << snapshot.seek << endl);
	}
	catch (const std::exception& e)
	{
		TLOGERROR("parser snapshot content error: " << e.what() << endl);
		return;
	}
}

void SnapshotDraftsman::setSavePath(const string& path)
{
	snapshotSaveDir_ = path;
}
