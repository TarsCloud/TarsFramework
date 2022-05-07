//
// Created by jarod on 2022/5/5.
//

#include "NodeManager.h"
#include "NodePush.h"
#include "servant/Application.h"
#include "DbProxy.h"

class AsyncNodePrxCallback : public NodePrxCallback
{
public:
	AsyncNodePrxCallback(const string &nodeName, CurrentPtr &current) : _nodeName(nodeName), _current(current)
	{

	}

	AsyncNodePrxCallback(const string &nodeName, const ServerStateDesc &desc, CurrentPtr &current) : _nodeName(nodeName), _desc(desc), _current(current)
	{

	}

//
//	virtual void callback_destroyServer(tars::Int32 ret,  const std::string& result)
//	{
//		AdminReg::async_response_destroyServer(_current, ret, result);
//	}
//
//	virtual void callback_destroyServer_exception(tars::Int32 ret)

//	{ throw std::runtime_error("callback_destroyServer_exception() override incorrect."); }

	virtual void callback_forceDockerLogin(tars::Int32 ret,  const vector<std::string>& result)
	{
		AdminReg::async_response_forceDockerLogin(_current, ret, result);
	}

	virtual void callback_forceDockerLogin_exception(tars::Int32 ret)
	{
		TLOG_DEBUG("forceDockerLogin " << _nodeName << " error ret:" << ret <<endl);

		AdminReg::async_response_forceDockerLogin(_current, ret, {"docker login error, ret:" + TC_Common::tostr(ret)});
	}

	virtual void callback_getLogData(tars::Int32 ret,  const std::string& fileData)
	{
		AdminReg::async_response_getLogData(_current, ret, fileData);
	}

	virtual void callback_getLogData_exception(tars::Int32 ret)
	{
		AdminReg::async_response_getLogData(_current, ret, "get log data error, ret:" + TC_Common::tostr(ret));
	}

	virtual void callback_getLogFileList(tars::Int32 ret,  const vector<std::string>& logFileList)
	{
		AdminReg::async_response_getLogFileList(_current, ret, logFileList);

	}
	virtual void callback_getLogFileList_exception(tars::Int32 ret)
	{
		AdminReg::async_response_getLogFileList(_current, ret, {});
	}

	virtual void callback_getNodeLoad(tars::Int32 ret,  const std::string& fileData)
	{
		AdminReg::async_response_getNodeLoad(_current, ret, fileData);
	}

	virtual void callback_getNodeLoad_exception(tars::Int32 ret)
	{
		AdminReg::async_response_getNodeLoad(_current, ret, "");
	}

	virtual void callback_getPatchPercent(tars::Int32 ret,  const tars::PatchInfo& tPatchInfo)
	{
		AdminReg::async_response_getPatchPercent(_current, ret, tPatchInfo);
	}
	virtual void callback_getPatchPercent_exception(tars::Int32 ret)
	{
		AdminReg::async_response_getPatchPercent(_current, ret, PatchInfo());
	}

	virtual void callback_getStateInfo(tars::Int32 ret,  const tars::ServerStateInfo& info,  const std::string& result)
	{
		_desc.processId = info.processId;
		_desc.presentStateInNode = etos(info.serverState);

		AdminReg::async_response_getServerState(_current, ret, _desc, result);
	}

	virtual void callback_getStateInfo_exception(tars::Int32 ret)
	{
		AdminReg::async_response_getServerState(_current, ret, _desc, "");
	}

	virtual void callback_loadServer(tars::Int32 ret,  const std::string& result)
	{
		AdminReg::async_response_loadServer(_current, ret, result);
	}

	virtual void callback_loadServer_exception(tars::Int32 ret)
	{
		AdminReg::async_response_loadServer(_current, ret, "");
	}

	virtual void callback_notifyServer(tars::Int32 ret,  const std::string& result)
	{
		AdminReg::async_response_notifyServer(_current, ret, result);
	}
	virtual void callback_notifyServer_exception(tars::Int32 ret)
	{
		AdminReg::async_response_notifyServer(_current, ret, "");
	}

	virtual void callback_patchPro(tars::Int32 ret,  const std::string& result)
	{
		AdminReg::async_response_batchPatch(_current, ret, result);
	}

	virtual void callback_patchPro_exception(tars::Int32 ret)
	{
		AdminReg::async_response_batchPatch(_current, ret, "");
	}

	virtual void callback_shutdown(tars::Int32 ret,  const std::string& result)
	{
		AdminReg::async_response_shutdownNode(_current, ret, result);
	}

	virtual void callback_shutdown_exception(tars::Int32 ret)
	{
		AdminReg::async_response_shutdownNode(_current, ret, "");
	}

	virtual void callback_startServer(tars::Int32 ret,  const std::string& result)
	{
		AdminReg::async_response_startServer(_current, ret, result);
	}
	virtual void callback_startServer_exception(tars::Int32 ret)
	{
		AdminReg::async_response_startServer(_current, ret, "");
	}

	virtual void callback_stopServer(tars::Int32 ret,  const std::string& result)
	{
		AdminReg::async_response_stopServer(_current, ret, result);
	}
	virtual void callback_stopServer_exception(tars::Int32 ret)
	{
		AdminReg::async_response_stopServer(_current, ret, "");
	}

protected:
	string 	   _nodeName;
	ServerStateDesc	_desc;
	CurrentPtr _current;
};

NodeManager::NodeManager()
{
	_timeoutQueue.setTimeout(_timeout);
}

NodePrx NodeManager::getNodePrx(const string& nodeName)
{
	{
		TC_ThreadLock::Lock lock(_NodePrxLock);

		if (_mapNodePrxCache.find(nodeName) != _mapNodePrxCache.end())
		{
			return _mapNodePrxCache[nodeName];
		}
	}

	NodePrx nodePrx = DbProxy::getInstance()->getNodePrx(nodeName);

	{
		TC_ThreadLock::Lock lock(_NodePrxLock);
		_mapNodePrxCache[nodeName] = nodePrx;
	}

	return nodePrx;
}

void NodeManager::eraseNodePrx(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	_mapNodePrxCache.erase(nodeName);
}

void NodeManager::createNodeCurrent(const string& nodeName, CurrentPtr &current)
{
	eraseNodeCurrent(nodeName);

	TC_ThreadLock::Lock lock(_NodePrxLock);

	_mapNodeId[nodeName] = current->getUId();
	_mapIdNode[current->getUId()] = nodeName;
	_mapIdCurrent[current->getUId()] = current;
}

CurrentPtr NodeManager::getNodeCurrent(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapNodeId.find(nodeName);
	if(it != _mapNodeId.end())
	{
		return _mapIdCurrent.at(it->second);
	}

	return NULL;
}

void NodeManager::eraseNodeCurrent(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapNodeId.find(nodeName);
	if(it != _mapNodeId.end())
	{
		_mapIdNode.erase(it->second);
		_mapIdCurrent.erase(it->second);
	}

	_mapNodeId.erase(nodeName);
}

void NodeManager::eraseNodeCurrent(CurrentPtr &current)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapIdNode.find(current->getUId());
	if(it != _mapIdNode.end())
	{
		_mapNodeId.erase(it->second);
		_mapIdNode.erase(it->first);
		_mapIdCurrent.erase(it->first);
	}
}

void NodeManager::terminate()
{
	std::unique_lock<std::mutex> lock(_mutex);

	_terminate = true;

	_cond.notify_one();
}

void NodeManager::run()
{
	std::function<void(NodeResultInfoPtr&)> df = [](NodeResultInfoPtr &ptr){
		ptr->_callback(ptr->_current, EM_TARS_CALL_NODE_TIMEOUT_ERR, "timeout");
	};

	while(!_terminate)
	{
		{
			std::unique_lock<std::mutex> lock(_mutex);

			_cond.wait_for(lock, std::chrono::milliseconds(100));
		}

		_timeoutQueue.timeout(df);
	}
}

int NodeManager::reportResult(int requestId, const string &result, CurrentPtr current)
{
	auto ptr = _timeoutQueue.get(requestId, true);

	if(ptr)
	{
		if(!ptr->_current)
		{
			//同步调用, 唤醒
			std::unique_lock<std::mutex> lock(ptr->_m);
			ptr->_c.notify_one();
		}
		else
		{
			//异步调用, 回包
			ptr->_callback(ptr->_current, EM_TARS_SUCCESS, result);
		}
	}
	return 0;
}

int NodeManager::requestNode(const string & nodeName, CurrentPtr current, string &result, NodeManager::push_type push, NodeManager::callback_type callback, NodeManager::sync_type sync)
{
	CurrentPtr nodeCurrent = getNodeCurrent(nodeName);
	if(nodeCurrent)
	{
		NodeResultInfoPtr ptr = new NodeResultInfo(_timeoutQueue.generateId(), current, callback);

		_timeoutQueue.push(ptr, ptr->_requestId);

		if(current)
		{
			//异步
			push(nodeCurrent, 0, ptr->_requestId);
		}
		else
		{
			//同步
			std::unique_lock<std::mutex> lock(ptr->_m);

			push(nodeCurrent, 0, ptr->_requestId);

			if(cv_status::no_timeout== ptr->_c.wait_for(lock, std::chrono::milliseconds(_timeout)))
			{
				ptr->_callback(ptr->_current, EM_TARS_SUCCESS, result);
			}
		}

		return EM_TARS_SUCCESS;

	}
	else
	{
		try
		{
			NodePrx nodePrx = getNodePrx(nodeName);

			if(current)
			{
				current->setResponse(false);
			}

			return sync(current, nodePrx, result);
		}
		catch(exception &ex)
		{
			current->setResponse(true);

			result = ex.what();
			TLOG_ERROR(nodeName << ": " << result << endl);
			return EM_TARS_CALL_NODE_TIMEOUT_ERR;
		}
	}
}

int NodeManager::pingNode(const string & nodeName, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::pingNode push name:" << nodeName << ", ret:" << ret <<endl);

		NodePush::async_response_push_ping(nodeCurrent, ret, requestId);
	};

	NodeManager::callback_type callback = [nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::pingNode name:" << nodeName << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_pingNode(current, ret == 0, buff);
		}
	};

	NodeManager::sync_type sync = [nodeName](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::pingNode sync name:" << nodeName <<endl);

		nodePrx->tars_ping();

		result = "ping succ";

		TLOG_DEBUG("NodeManager::pingNode sync name:" << nodeName << ", succ" <<endl);

		if(current)
		{
			AdminReg::async_response_pingNode(current, true, result);
		}

		return 0;
	};

	return NodeManager::getInstance()->requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::shutdownNode(const string & nodeName, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::shutdownNode push name:" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_shutdown(nodeCurrent, ret, requestId);
	};

	NodeManager::callback_type callback = [nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::shutdownNode name:" << nodeName << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_shutdownNode(current, ret == 0, buff);
		}
	};

	NodeManager::sync_type sync = [nodeName](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::shutdownNode sync name:" << nodeName <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_shutdown(cb);
		}
		else
		{
			//同步
			ret = nodePrx->shutdown(result);
		}
		return ret;
	};

	return NodeManager::getInstance()->requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::getServerState(const string & application, const string & serverName, const string & nodeName, ServerStateDesc &state, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::getServerState push name:" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_getStateInfo(nodeCurrent, ret, requestId, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getServerState :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);

		if(current)
		{
			TarsInputStream<> is;
			is.setBuffer(buff.c_str(), buff.length());

			ServerStateDesc desc;
			string result;
			is.read(desc, 0, true);
			is.read(result, 1, true);

			AdminReg::async_response_getServerState(current, ret, desc, result);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, state](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::getServerState sync:" << application << "." << serverName << "_" << nodeName <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, state, current);

			nodePrx->async_getStateInfo(cb, application, serverName);
		}
		else
		{
			ServerStateInfo info;

			//同步
			ret = nodePrx->getStateInfo(application, serverName, info, result);

			if (ret == EM_TARS_SUCCESS)
			{
				TarsOutputStream<BufferWriterString> os;
				info.writeTo(os);
				result = os.getByteBuffer();
			}
		}

		return ret;
	};
	return NodeManager::getInstance()->requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::startServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::startServer push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_startServer(nodeCurrent, ret, requestId, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::startServer: " << application << "." << serverName << "_" << nodeName << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_startServer(current, ret, buff);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::startServer sync :" << application << "." << serverName << "_" << nodeName <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_startServer(cb, application, serverName);
		}
		else
		{
			//同步
			ret = nodePrx->startServer(application, serverName, result);
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::stopServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::stopServer push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_stopServer(nodeCurrent, ret, requestId, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::stopServer: " << application << "." << serverName << "_" << nodeName << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_stopServer(current, ret, buff);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::stopServer sync :" << application << "." << serverName << "_" << nodeName <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_stopServer(cb, application, serverName);
		}
		else
		{
			//同步
			ret = nodePrx->stopServer(application, serverName, result);
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::notifyServer(const string & application, const string & serverName, const string & nodeName, const string &command, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName, command](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::notifyServer push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_notifyServer(nodeCurrent, ret, requestId, application, serverName, command);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::notifyServer: " << application << "." << serverName << "_" << nodeName << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_notifyServer(current, ret, buff);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, command](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::notifyServer sync :" << application << "." << serverName << "_" << nodeName <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_notifyServer(cb, application, serverName, command);
		}
		else
		{
			//同步
			ret = nodePrx->notifyServer(application, serverName, command, result);
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::patchPro(const tars::PatchRequest &req, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [req](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::patchPro push :" << req.appname << "." << req.servername << "_" << req.nodename << ", ret:" << ret <<endl);
		NodePush::async_response_push_patchPro(nodeCurrent, ret, requestId, req);
	};

	NodeManager::callback_type callback = [req](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::patchPro: " << req.appname << "." << req.servername << "_" << req.nodename << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_batchPatch(current, ret, buff);
		}
	};

	NodeManager::sync_type sync = [req](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::patchPro sync :" << req.appname << "." << req.servername << "_" << req.nodename <<endl);

		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(req.nodename, current);

			nodePrx->async_patchPro(cb, req);
		}
		else
		{
			//同步
			ret = nodePrx->patchPro(req, result);
		}

		return ret;
	};

	return requestNode(req.nodename, current, result, push, callback, sync);
}

int NodeManager::getPatchPercent(const string & application, const string & serverName, const string & nodeName, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::getPatchPercent push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_getPatchPercent(nodeCurrent, ret, requestId, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getPatchPercent: " << application << "." << serverName << "_" << nodeName  << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			TarsInputStream<> is;
			is.setBuffer(buff.c_str(), buff.length());

			PatchInfo info;
			is.read(info, 0, true);

			AdminReg::async_response_getPatchPercent(current, ret, info);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::getPatchPercent sync :" << application << "." << serverName << "_" << nodeName  <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_getPatchPercent(cb, application, serverName);
		}
		else
		{
			PatchInfo info;

			//同步
			ret = nodePrx->getPatchPercent(application, serverName, info);

			if(ret == 0)
			{
				TarsOutputStream<BufferWriterString> os;
				info.writeTo(os);
				result = os.getByteBuffer();
			}
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::getLogData(const std::string & application, const std::string & serverName, const std::string & nodeName, const std::string &logFile, const std::string &cmd, map<string, string> &context,  string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName, logFile, cmd, context](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::getLogData push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_getLogData(nodeCurrent, ret, requestId, application, serverName, logFile, cmd, context);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getLogData: " << application << "." << serverName << "_" << nodeName  << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			TarsInputStream<> is;
			is.setBuffer(buff.c_str(), buff.length());

			string fileData;
			is.read(fileData, 0, true);

			AdminReg::async_response_getLogData(current, ret, fileData);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, logFile, cmd, context](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::getLogData sync :" << application << "." << serverName << "_" << nodeName << ", " << logFile << ", cmd:" << cmd <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_getLogData(cb, application, serverName, logFile, cmd, context);
		}
		else
		{
			//同步
			ret = nodePrx->getLogData(application, serverName, logFile, cmd, result, context);
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::getLogFileList(const std::string & application, const std::string & serverName, const std::string & nodeName, map<string, string> &context, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName,context](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::getLogFileList push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_getLogFileList(nodeCurrent, ret, requestId, application, serverName, context);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getLogFileList: " << application << "." << serverName << "_" << nodeName  << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			TarsInputStream<> is;
			is.setBuffer(buff.c_str(), buff.length());

			vector<std::string> logFileList;
			is.read(logFileList, 0, true);

			AdminReg::async_response_getLogFileList(current, ret, logFileList);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, context](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::getLogFileList sync :" << application << "." << serverName << "_" << nodeName  <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_getLogFileList(cb, application, serverName, context);
		}
		else
		{
			vector<std::string> logFileList;

			//同步
			ret = nodePrx->getLogFileList(application, serverName, logFileList, context);

			if(ret == EM_TARS_SUCCESS)
			{
				TarsOutputStream<BufferWriterString> os;
				os.write(logFileList, 0);
				result = os.getByteBuffer();
			}
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::getNodeLoad(const std::string & application, const std::string & serverName, const std::string & nodeName, int pid, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName, pid](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::getNodeLoad push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_getNodeLoad(nodeCurrent, ret, requestId, application, serverName, pid);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getNodeLoad: " << application << "." << serverName << "_" << nodeName  << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			TarsInputStream<> is;
			is.setBuffer(buff.c_str(), buff.length());

			string fileData;
			is.read(fileData, 0, true);

			AdminReg::async_response_getNodeLoad(current, ret, fileData);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, pid](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::getNodeLoad sync :" << application << "." << serverName << "_" << nodeName  <<endl);

		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_getNodeLoad(cb, application, serverName, pid);
		}
		else
		{
			//同步
			ret = nodePrx->getNodeLoad(application, serverName, pid, result);
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::loadServer(const std::string & application, const std::string & serverName, const std::string & nodeName, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::loadServer push :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_loadServer(nodeCurrent, ret, requestId, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::loadServer: " << application << "." << serverName << "_" << nodeName << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_loadServer(current, ret, buff);
		}
	};

	NodeManager::sync_type sync = [application, serverName, nodeName](CurrentPtr &current, NodePrx &nodePrx, string &result){

		TLOG_DEBUG("NodeManager::loadServer sync :" << application << "." << serverName << "_" << nodeName <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_loadServer(cb, application, serverName);
		}
		else
		{
			//同步
			ret = nodePrx->loadServer(application, serverName, result);
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

int NodeManager::forceDockerLogin(const std::string & nodeName, string & result, tars::CurrentPtr current)
{
	NodeManager::push_type push = [nodeName](CurrentPtr &nodeCurrent, int ret, int requestId){
		TLOG_DEBUG("NodeManager::forceDockerLogin push :" << nodeName << ", ret:" << ret <<endl);
		NodePush::async_response_push_forceDockerLogin(nodeCurrent, ret, requestId);
	};

	NodeManager::callback_type callback = [nodeName](CurrentPtr &current, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::forceDockerLogin: " << nodeName << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			TarsInputStream<> is;
			is.setBuffer(buff.c_str(), buff.length());

			vector<string> result;
			is.read(result, 0, true);

			AdminReg::async_response_forceDockerLogin(current, ret, result);
		}
	};

	NodeManager::sync_type sync = [nodeName](CurrentPtr &current, NodePrx &nodePrx, string &buff){

		TLOG_DEBUG("NodeManager::forceDockerLogin sync :" << nodeName <<endl);

		vector<string> result;
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, current);

			nodePrx->async_forceDockerLogin(cb);

		}
		else
		{
			//同步
			ret = nodePrx->forceDockerLogin(result);
		}

		return ret;
	};

	return requestNode(nodeName, current, result, push, callback, sync);
}

