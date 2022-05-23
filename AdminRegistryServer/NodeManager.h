//
// Created by jarod on 2022/5/5.
//

#ifndef FRAMEWORK_NODEMANAGER_H
#define FRAMEWORK_NODEMANAGER_H

#include "Node.h"
#include "AdminReg.h"
#include "util/tc_monitor.h"
#include "util/tc_singleton.h"
#include "util/tc_timeout_queue.h"
#include "servant/RemoteLogger.h"

using namespace tars;

class NodeManager : public TC_Singleton<NodeManager>, public TC_Thread
{
public:

	struct UidTimeStr
	{
		string timeStr;				//更新时间
		list<int>	uids;			//连接ID, 按照时间排序
		unordered_map<int, list<int>::iterator> its;
	};

	using push_type = std::function<void(CurrentPtr &, int requestId)>;
	using callback_type = std::function<int(CurrentPtr &, bool timeout, int ret, const string&)>;
	using sync_type = std::function<int(CurrentPtr &, NodePrx &, string &out)>;

	NodeManager();

	void terminate();

	NodePrx getNodePrx(const string& nodeName);
	void eraseNodePrx(const string& nodeName);

	void createNodeCurrent(const string& nodeName, const string &sid, CurrentPtr &current);
	void deleteNodeCurrent(const string& nodeName, const string &sid, CurrentPtr &current);
	CurrentPtr getNodeCurrent(const string& nodeName);
	void eraseNodeCurrent(CurrentPtr &current);

	unordered_map<string, UidTimeStr> getNodeList();

public:

	struct NodeResultInfo : public TC_HandleBase
	{
		NodeResultInfo(int requestId, CurrentPtr &current, NodeManager::callback_type callback) : _requestId(requestId), _current(current), _callback(callback)
		{
			if(current)
			{
				current->setResponse(false);
			}
		}

		int _requestId = 0;
		int _ret;
		string _result;
		CurrentPtr _current;
		NodeManager::callback_type _callback;
		std::mutex _m;
		std::condition_variable _c;
	};
	typedef TC_AutoPtr<NodeResultInfo> NodeResultInfoPtr;

	/**
	 * tarsnode 上报执行结果
	 * @param requestId
	 * @param out
	 * @param current
	 * @return
	 */
	int reportResult(int requestId, const string &funcName, int ret, const string &out, CurrentPtr current);

	/**
	 * 请求节点
	 * @param nodeName
	 * @param out: 老tarsnode下才使用
	 * @param current
	 * @param push
	 * @param callback
	 * @param sync
	 * @return
	 */
	int requestNode(const string & nodeName, string &out, CurrentPtr current, NodeManager::push_type push, NodeManager::callback_type callback, NodeManager::sync_type sync);

	/**
	 *
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int pingNode(const string & nodeName, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int shutdownNode(const string & nodeName, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int getServerState(const string & application, const string & serverName, const string & nodeName, ServerStateDesc &state, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int loadServer(const std::string & application, const std::string & serverName, const std::string & nodeName, string & out, tars::CurrentPtr current);

	/**
	 * 毁掉服务
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int destroyServer(const string & application, const string & serverName, const string & nodeName, string & out, tars::CurrentPtr current);

	/**
	 * 启动服务
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int startServer(const string & application, const string & serverName, const string & nodeName, string & out, tars::CurrentPtr current);

	/**
	 * 停止服务
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int stopServer(const string & application, const string & serverName, const string & nodeName, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int notifyServer(const string & application, const string & serverName, const string & nodeName, const string &command, string & out, tars::CurrentPtr current);

	/**
	 * 发布
	 * @param req
	 * @param out
	 * @param current
	 * @param syncCall
	 * @return
	 */
	int patchPro(const tars::PatchRequest &req, string & out, tars::CurrentPtr current);

	/**
	 * 获取发布进度
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int getPatchPercent(const string & application, const string & serverName, const string & nodeName, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param sFullCacheName
	 * @param sBackupPath
	 * @param sKey
	 * @param result
	 * @param current
	 * @return
	 */
	int delCache(const string & nodeName, const std::string & sFullCacheName, const std::string &sBackupPath, const std::string & sKey, std::string &result,CurrentPtr current);

	/**
	 *
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param logFile
	 * @param cmd
	 * @param out
	 * @param current
	 * @param syncCall
	 * @return
	 */
	int getLogData(const std::string & application, const std::string & serverName, const std::string & nodeName, const std::string & logFile, const std::string & cmd,  map<string, string> &context, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param out
	 * @param current
	 * @param syncCall
	 * @return
	 */
	int getLogFileList(const std::string & application, const std::string & serverName, const std::string & nodeName, map<string, string> &context, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param pid
	 * @param out
	 * @param current
	 * @return
	 */
	int getNodeLoad(const std::string & application, const std::string & serverName, const std::string & nodeName, int pid, string & out, tars::CurrentPtr current);

	/**
	 *
	 * @param nodeName
	 * @param out
	 * @param current
	 * @return
	 */
	int forceDockerLogin(const std::string & nodeName, string & out, tars::CurrentPtr current);

protected:
	virtual void run();

protected:
	bool _terminate = false;
	std::mutex _mutex;
	std::condition_variable _cond;

	TC_ThreadLock _NodePrxLock;

	//node节点代理列表
	map<string , NodePrx> _mapNodePrxCache;

	//一个连接上, 可能有多个nodeName的请求
	//<nodename, UidTimeStr>
	unordered_map<string, UidTimeStr> _mapNodeId;
	//<uid, nodeNames[]>
	unordered_map<int, unordered_set<string>> _mapIdNode;
	//<uid, current>
	unordered_map<int, CurrentPtr> _mapIdCurrent;

	int _timeout = 5000;

	TC_TimeoutQueue<NodeResultInfoPtr>	_timeoutQueue;
};


#endif //FRAMEWORK_NODEMANAGER_H
