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

	TC_ThreadLock::Lock lock(_NodePrxLock);

	_mapNodePrxCache[nodeName] = nodePrx;

	return nodePrx;
}

void NodeManager::eraseNodePrx(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	_mapNodePrxCache.erase(nodeName);
}

unordered_map<string, NodeManager::UidTimeStr> NodeManager::getNodeList()
{
	TC_ThreadLock::Lock lock(_NodePrxLock);
	return _mapNodeId;
}

void NodeManager::createNodeCurrent(const string& nodeName, const string &sid, CurrentPtr &current)
{

	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapNodeId.find(nodeName);
	if(it != _mapNodeId.end())
	{
		it->second.timeStr = TC_Common::now2str("%Y-%m-%d %H:%M:%S");

//		TLOG_DEBUG("nodeName:" << nodeName << ", connection uid size:" << it->second.uids.size() << ", uid:" << current->getUId() << endl);

		auto ii = it->second.its.find(current->getUId());
		if(ii != it->second.its.end())
		{
			it->second.uids.erase(ii->second);
		}

		it->second.uids.push_front(current->getUId());
		it->second.its[current->getUId()] = it->second.uids.begin();
	}
	else
	{
		TLOG_DEBUG("nodeName:" << nodeName << ", uid:" << current->getUId() << endl);

		UidTimeStr &str = _mapNodeId[nodeName];
		str.timeStr = TC_Common::now2str("%Y-%m-%d %H:%M:%S");
		str.uids.push_front(current->getUId());
		str.its[current->getUId()] = str.uids.begin();
	}

	_mapIdNode[current->getUId()].insert(nodeName);
	_mapIdCurrent[current->getUId()] = current;
}

CurrentPtr NodeManager::getNodeCurrent(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapNodeId.find(nodeName);
	if(it != _mapNodeId.end())
	{
		auto uid = it->second.uids.begin();
		if(uid == it->second.uids.end())
		{
			TLOG_ERROR("nodeName:" << nodeName << ", connection uid size:" << it->second.uids.size() << " no alive connection." << endl);
			return  NULL;
		}
//		TLOG_DEBUG("nodeName:" << nodeName << ", connection uid size:" << it->second.uids.size() << ", uid:" << *uid << endl);

		return _mapIdCurrent.at(*uid);
	}

	TLOG_ERROR("nodeName:" << nodeName << " no alive connection." << endl);

	return NULL;
}

void NodeManager::eraseNodeCurrent(CurrentPtr &current)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapIdNode.find(current->getUId());
	if(it != _mapIdNode.end())
	{
		for(auto nodeName : it->second)
		{
			auto ii = _mapNodeId.find(nodeName);
			if(ii != _mapNodeId.end())
			{
				auto iu = ii->second.its.find(current->getUId());
				if(iu != ii->second.its.end())
				{
					ii->second.uids.erase(iu->second);
					ii->second.its.erase(iu);
				}
			}
		}
		_mapIdCurrent.erase(it->first);
		_mapIdNode.erase(it);
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
		ptr->_callback(ptr->_current, true, 0, "");
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

int NodeManager::reportResult(int requestId, const string &funcName, int ret,  const string &result, CurrentPtr current)
{
	auto ptr = _timeoutQueue.get(requestId, true);

	if(ptr)
	{
		ptr->_ret = ret;
		ptr->_result = result;

		if(!ptr->_current)
		{
			//同步调用, 唤醒
			std::unique_lock<std::mutex> lock(ptr->_m);
			ptr->_c.notify_one();
		}
		else
		{
			//异步调用, 回包
			ptr->_callback(ptr->_current, false, ptr->_ret, ptr->_result);
		}
	}
	else
	{
		TLOG_DEBUG("requestId:" << requestId << ", " << funcName << ", timeout" <<endl);

	}
	return 0;
}

int NodeManager::requestNode(const string & nodeName, string &out, CurrentPtr current, NodeManager::push_type push, NodeManager::callback_type callback, NodeManager::sync_type sync)
{
	CurrentPtr nodeCurrent = getNodeCurrent(nodeName);
	if(nodeCurrent)
	{
		NodeResultInfoPtr ptr = new NodeResultInfo(_timeoutQueue.generateId(), current, callback);

		_timeoutQueue.push(ptr, ptr->_requestId);

		if(current)
		{
			//异步
			push(nodeCurrent, ptr->_requestId);
		}
		else
		{
			//同步
			std::unique_lock<std::mutex> lock(ptr->_m);

			push(nodeCurrent, ptr->_requestId);

			if(cv_status::no_timeout == ptr->_c.wait_for(lock, std::chrono::milliseconds(_timeout)))
			{
				out = ptr->_result;
				return ptr->_callback(ptr->_current, false, ptr->_ret, ptr->_result);
			}
			else
			{
				return EM_TARS_CALL_NODE_TIMEOUT_ERR;
			}
		}

		return EM_TARS_SUCCESS;

	}
	else
	{
		NodePrx nodePrx = getNodePrx(nodeName);

		if(!nodePrx)
		{
			TLOG_ERROR("nodeName:" << nodeName << ", no long connection" <<endl);
			return EM_TARS_NODE_NO_CONNECTION;
		}

		if(current)
		{
			current->setResponse(false);
		}

		return sync(current, nodePrx, out);
	}
}

int NodeManager::pingNode(const string & nodeName, string &out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::pingNode push name:" << nodeName <<endl);

		NodePush::async_response_push_ping(nodeCurrent, requestId, nodeName);
	};

	NodeManager::callback_type callback = [nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::pingNode callback:" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_pingNode(current, timeout?false:true, buff);
		}

		return ret;
	};

	NodeManager::sync_type sync = [nodeName](CurrentPtr &current, NodePrx &nodePrx, string &out){

		TLOG_DEBUG("NodeManager::pingNode sync name:" << nodeName <<endl);

		nodePrx->tars_ping();

		TLOG_DEBUG("NodeManager::pingNode sync name:" << nodeName << ", succ" <<endl);

		out = "ping succ";

		if(current)
		{
			AdminReg::async_response_pingNode(current, true, out);
		}

		return 0;
	};

	return NodeManager::getInstance()->requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::shutdownNode(const string & nodeName, string &out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::shutdownNode push name:" << nodeName<<endl);
		NodePush::async_response_push_shutdown(nodeCurrent, requestId, nodeName);
	};

	NodeManager::callback_type callback = [nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::shutdownNode name:" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_shutdownNode(current, timeout?false:true, buff);
		}
		return ret;
	};

	NodeManager::sync_type sync = [nodeName](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
			string result;
			ret = nodePrx->shutdown(result);

			TLOG_DEBUG("NodeManager::shutdownNode sync ret:" << ret << ", result:" << result <<endl);

			out = result;
		}
		return ret;
	};

	return NodeManager::getInstance()->requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::getServerState(const string & application, const string & serverName, const string & nodeName, ServerStateDesc &desc, string &out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::getServerState push name:" << nodeName <<endl);
		NodePush::async_response_push_getStateInfo(nodeCurrent, requestId, nodeName, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName, desc](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getServerState :" << application << "." << serverName << "_" << nodeName << ", ret:" << ret <<endl);

		if(current)
		{
			if(!timeout)
			{
				TarsInputStream<> is;
				is.setBuffer(buff.c_str(), buff.length());

				ServerStateInfo info;
				string result;
				is.read(info, 0, true);
				is.read(result, 1, true);

				ServerStateDesc newDesc = desc;
				newDesc.processId = info.processId;
				newDesc.presentStateInNode = etos(info.serverState);

				TLOG_DEBUG("NodeManager::getServerState :" << application << "." << serverName << "_" << nodeName << ", " << newDesc.writeToJsonString() <<endl);

				AdminReg::async_response_getServerState(current, ret, newDesc, result);
			}
			else
			{
				AdminReg::async_response_getServerState(current, EM_TARS_CALL_NODE_TIMEOUT_ERR, desc, buff);
			}
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, desc](CurrentPtr &current, NodePrx &nodePrx, string &out){

		TLOG_DEBUG("NodeManager::getServerState sync:" << application << "." << serverName << "_" << nodeName <<endl);
		int ret = 0;
		if(current)
		{
			//异步
			NodePrxCallbackPtr cb = new AsyncNodePrxCallback(nodeName, desc, current);

			nodePrx->async_getStateInfo(cb, application, serverName);
		}
		else
		{
			ServerStateInfo info;
			string result;

			//同步
			ret = nodePrx->getStateInfo(application, serverName, info, result);

			if (ret == EM_TARS_SUCCESS)
			{
				ServerStateDesc newDesc = desc;
				newDesc.processId = info.processId;
				newDesc.presentStateInNode = etos(info.serverState);

				TarsOutputStream<BufferWriterString> os;
				newDesc.writeTo(os);
				out = os.getByteBuffer();
			}
		}

		return ret;
	};
	return NodeManager::getInstance()->requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::startServer(const string & application, const string & serverName, const string & nodeName, string &out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::startServer push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_startServer(nodeCurrent, requestId, nodeName, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::startServer: " << application << "." << serverName << "_" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_startServer(current, timeout?EM_TARS_CALL_NODE_TIMEOUT_ERR:ret, buff);
		}

		return ret;
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

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::stopServer(const string & application, const string & serverName, const string & nodeName, string &out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::stopServer push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_stopServer(nodeCurrent, requestId, nodeName, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::stopServer: " << application << "." << serverName << "_" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_stopServer(current, timeout?EM_TARS_CALL_NODE_TIMEOUT_ERR:ret, buff);
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
			ret = nodePrx->stopServer(application, serverName, out);
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::notifyServer(const string & application, const string & serverName, const string & nodeName, const string &command, string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName, command](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::notifyServer push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_notifyServer(nodeCurrent, requestId, nodeName, application, serverName, command);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::notifyServer: " << application << "." << serverName << "_" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_notifyServer(current, timeout?EM_TARS_CALL_NODE_TIMEOUT_ERR:ret, buff);
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, command](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
			ret = nodePrx->notifyServer(application, serverName, command, out);
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::patchPro(const tars::PatchRequest &req, string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [req](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::patchPro push :" << req.appname << "." << req.servername << "_" << req.nodename <<endl);
		NodePush::async_response_push_patchPro(nodeCurrent, requestId, req.nodename, req);
	};

	NodeManager::callback_type callback = [req](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::patchPro: " << req.appname << "." << req.servername << "_" << req.nodename << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_batchPatch(current, timeout?EM_TARS_CALL_NODE_TIMEOUT_ERR:ret, buff);
		}

		return ret;
	};

	NodeManager::sync_type sync = [req](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
			ret = nodePrx->patchPro(req, out);
		}

		return ret;
	};

	return requestNode(req.nodename, out, current, push, callback, sync);
}

int NodeManager::getPatchPercent(const string & application, const string & serverName, const string & nodeName, string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::getPatchPercent push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_getPatchPercent(nodeCurrent, requestId, nodeName, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getPatchPercent: " << application << "." << serverName << "_" << nodeName  << ", timeout:" << timeout << ", ret:" << ret <<endl);

		if(current)
		{
			if(!timeout)
			{
				TarsInputStream<> is;
				is.setBuffer(buff.c_str(), buff.length());

				PatchInfo info;
				info.readFrom(is);

				TLOG_DEBUG("NodeManager::getPatchPercent: " << application << "." << serverName << "_" << nodeName << ", " << info.writeToJsonString() <<endl);

				AdminReg::async_response_getPatchPercent(current, ret, info);
			}
			else
			{
				PatchInfo info;
				AdminReg::async_response_getPatchPercent(current, EM_TARS_CALL_NODE_TIMEOUT_ERR, info);
			}
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
				out = os.getByteBuffer();
			}
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::getLogData(const std::string & application, const std::string & serverName, const std::string & nodeName, const std::string &logFile, const std::string &cmd, map<string, string> &context,  string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName, logFile, cmd, context](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::getLogData push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_getLogData(nodeCurrent, requestId, nodeName, application, serverName, logFile, cmd, context);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getLogData: " << application << "." << serverName << "_" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			if(!timeout)
			{
				TarsInputStream<> is;
				is.setBuffer(buff.c_str(), buff.length());

				string fileData;
				is.read(fileData, 0, true);

				AdminReg::async_response_getLogData(current, ret, fileData);
			}
			else
			{
				AdminReg::async_response_getLogData(current, EM_TARS_CALL_NODE_TIMEOUT_ERR, "");
			}
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, logFile, cmd, context](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
			ret = nodePrx->getLogData(application, serverName, logFile, cmd, out, context);
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::getLogFileList(const std::string & application, const std::string & serverName, const std::string & nodeName, map<string, string> &context, string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName,context](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::getLogFileList push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_getLogFileList(nodeCurrent, requestId, nodeName, application, serverName, context);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getLogFileList: " << application << "." << serverName << "_" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			if(!timeout)
			{
				TarsInputStream<> is;
				is.setBuffer(buff.c_str(), buff.length());

				vector<std::string> logFileList;
				is.read(logFileList, 0, true);

				AdminReg::async_response_getLogFileList(current, ret, logFileList);
			}
			else
			{
				AdminReg::async_response_getLogFileList(current, EM_TARS_CALL_NODE_TIMEOUT_ERR, {});
			}
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, context](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
				out = os.getByteBuffer();
			}
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::getNodeLoad(const std::string & application, const std::string & serverName, const std::string & nodeName, int pid, string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName, pid](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::getNodeLoad push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_getNodeLoad(nodeCurrent, requestId, nodeName, application, serverName, pid);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::getNodeLoad: " << application << "." << serverName << "_" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			if(!timeout)
			{
				TarsInputStream<> is;
				is.setBuffer(buff.c_str(), buff.length());

				string fileData;
				is.read(fileData, 0, true);

				AdminReg::async_response_getNodeLoad(current, ret, fileData);
			}
			else
			{
				AdminReg::async_response_getNodeLoad(current, EM_TARS_CALL_NODE_TIMEOUT_ERR, "");
			}
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName, pid](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
			ret = nodePrx->getNodeLoad(application, serverName, pid, out);
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::loadServer(const std::string & application, const std::string & serverName, const std::string & nodeName, string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [application, serverName, nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::loadServer push :" << application << "." << serverName << "_" << nodeName <<endl);
		NodePush::async_response_push_loadServer(nodeCurrent, requestId, nodeName, application, serverName);
	};

	NodeManager::callback_type callback = [application, serverName, nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::loadServer: " << application << "." << serverName << "_" << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			AdminReg::async_response_loadServer(current, timeout?EM_TARS_CALL_NODE_TIMEOUT_ERR:ret, buff);
		}

		return ret;
	};

	NodeManager::sync_type sync = [application, serverName, nodeName](CurrentPtr &current, NodePrx &nodePrx, string &out){

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
			ret = nodePrx->loadServer(application, serverName, out);
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

int NodeManager::forceDockerLogin(const std::string & nodeName, string & out, tars::CurrentPtr current)
{
	NodeManager::push_type push = [nodeName](CurrentPtr &nodeCurrent, int requestId){
		TLOG_DEBUG("NodeManager::forceDockerLogin push :" << nodeName<<endl);
		NodePush::async_response_push_forceDockerLogin(nodeCurrent, requestId, nodeName);
	};

	NodeManager::callback_type callback = [nodeName](CurrentPtr &current, bool timeout, int ret, const string &buff)
	{
		TLOG_DEBUG("NodeManager::forceDockerLogin: " << nodeName << ", timeout:" << timeout << ", ret:" << ret << ", result:" << buff <<endl);

		if(current)
		{
			if(!timeout)
			{
				TarsInputStream<> is;
				is.setBuffer(buff.c_str(), buff.length());

				vector<string> result;
				is.read(result, 0, true);

				AdminReg::async_response_forceDockerLogin(current, ret, result);
			}
			else
			{
				AdminReg::async_response_forceDockerLogin(current, EM_TARS_CALL_NODE_TIMEOUT_ERR, {});
			}
		}

		return ret;
	};

	NodeManager::sync_type sync = [nodeName](CurrentPtr &current, NodePrx &nodePrx, string &out){

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

			if(ret == EM_TARS_SUCCESS)
			{
				TarsOutputStream<BufferWriterString> os;
				os.write(result, 0);
				out = os.getByteBuffer();
			}
		}

		return ret;
	};

	return requestNode(nodeName, out, current, push, callback, sync);
}

