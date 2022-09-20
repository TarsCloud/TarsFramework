//
// Created by jarod on 2022/9/14.
//

#include "QueryObjectsManager.h"
#include "ObjectsCacheManager.h"

void QueryPushFImp::onConnect(const TC_Endpoint& ep)
{
	_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_registerChange(NULL, _objs, ClientConfig::ModuleName);
}

void QueryPushFImp::callback_onChange(const std::string& id, const ObjectItem &item)
{
	ObjectsCacheManager::getInstance()->onChange(id, item);

	QueryObjectsManager::getInstance()->pushObjQuery(id);
}

void QueryPushFImp::callback_onChangeGroupPriorityEntry(const map<tars::Int32, tars::GroupPriorityEntry>& group)
{
	ObjectsCacheManager::getInstance()->onChangeGroupPriorityEntry(group);
}

void QueryPushFImp::callback_onChangeSetInfo(const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo)
{
	ObjectsCacheManager::getInstance()->onChangeSetInfo(setInfo);
}

void QueryPushFImp::callback_onChangeServerGroupRule(const vector<map<string, string>> &serverGroupRule)
{
	ObjectsCacheManager::getInstance()->onChangeServerGroupRule(serverGroupRule);
}
//////////////////////////////////////////////////////////////////////////////////////////////

class QueryFImp : public QueryFPrxCallback
{
public:
	QueryFImp(function<void(const vector<tars::EndpointF> &ret)> func) : _func1(func){

	}

	QueryFImp(function<void(int ret, const vector<tars::EndpointF> &activeEp, const vector<EndpointF> &inactiveEp)> func) : _func2(func)
	{
	}

	QueryFImp(const std::string & id) : _id(id){}

	virtual ~QueryFImp(){}

	virtual void callback_findObjectById(const vector<tars::EndpointF>& ret)
	{
		_func1(ret);
	}

	virtual void callback_findObjectById_exception(tars::Int32 ret)
	{
		TLOG_ERROR("error ret:" << ret << endl);
	}

	virtual void callback_findObjectById4All(tars::Int32 ret,  const vector<tars::EndpointF>& activeEp,  const vector<tars::EndpointF>& inactiveEp)
	{
		_func2(ret, activeEp, inactiveEp);
	}

	virtual void callback_findObjectById4All_exception(tars::Int32 ret)
	{
		TLOG_ERROR("error ret:" << ret << endl);
	}

	virtual void callback_findObjectById4Any(tars::Int32 ret,  const vector<tars::EndpointF>& activeEp,  const vector<tars::EndpointF>& inactiveEp)
	{
		_func2(ret, activeEp, inactiveEp);
	}

	virtual void callback_findObjectById4Any_exception(tars::Int32 ret)
	{
		TLOG_ERROR("error ret:" << ret << endl);
	}

	virtual void callback_findObjectByIdInSameGroup(tars::Int32 ret, const vector<tars::EndpointF>& activeEp,  const vector<tars::EndpointF>& inactiveEp)
	{
		_func2(ret, activeEp, inactiveEp);
	}

	virtual void callback_findObjectByIdInSameGroup_exception(tars::Int32 ret)
	{
		TLOG_ERROR("error ret:" << ret << endl);
	}

	virtual void callback_findObjectByIdInSameSet(tars::Int32 ret, const vector<tars::EndpointF>& activeEp,  const vector<tars::EndpointF>& inactiveEp)
	{
		_func2(ret, activeEp, inactiveEp);
	}

	virtual void callback_findObjectByIdInSameSet_exception(tars::Int32 ret)
	{
		TLOG_ERROR("error ret:" << ret << endl);
	}

	virtual void callback_findObjectByIdInSameStation(tars::Int32 ret, const vector<tars::EndpointF>& activeEp,  const vector<tars::EndpointF>& inactiveEp)
	{
		_func2(ret, activeEp, inactiveEp);
	}

	virtual void callback_findObjectByIdInSameStation_exception(tars::Int32 ret)
	{
		TLOG_ERROR("error ret:" << ret << endl);
	}

	virtual void callback_registerChange(tars::Int32 ret)
	{
		QueryObjectsManager::getInstance()->addObjectId(_id);
	}

	virtual void callback_registerChange_exception(tars::Int32 ret)
	{
		TLOG_ERROR("error, ret:" << ret << endl);
	}

	virtual void callback_registerQuery(tars::Int32 ret)
	{  }
	virtual void callback_registerQuery_exception(tars::Int32 ret)
	{  }

protected:
	function<void(const vector<tars::EndpointF> &ret)> _func1;
	function<void(int ret, const vector<tars::EndpointF> &activeEp, const vector<EndpointF> &inactiveEp)> _func2;

	string _id;
};

//////////////////////////////////////////////////////////////////////////////////////////////

QueryObjectsManager::QueryObjectsManager()
{
	_queryFPrx = Application::getCommunicator()->stringToProxy<QueryFPrx>("tars.tarsregistry.QueryObj#push");
	_queryFPrx->tars_set_push_callback(new QueryPushFImp(_queryFPrx));
}

void QueryObjectsManager::terminate()
{
	_terminate = true;
}

void QueryObjectsManager::run()
{

}

vector<EndpointF> QueryObjectsManager::findObjectById(const string & id, CurrentPtr current)
{
	registerQuery(id, ClientConfig::ModuleName, current);

	if(ObjectsCacheManager::getInstance()->hasObjectId(id))
	{
		return ObjectsCacheManager::getInstance()->findObjectById(id, current);
	}
	else
	{
		current->setResponse(false);

		//从主控主动获取一次
		_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_findObjectById(new QueryFImp([=](const vector<tars::EndpointF> &ret){
			QueryF::async_response_findObjectById(current, ret);
		}), id);

		return vector<EndpointF>();
	}
}

Int32 QueryObjectsManager::findObjectById4Any(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	registerQuery(id, ClientConfig::ModuleName, current);
	if(ObjectsCacheManager::getInstance()->hasObjectId(id))
	{
		return ObjectsCacheManager::getInstance()->findObjectById4Any(id, activeEp, inactiveEp, current);
	}
	else
	{
		current->setResponse(false);

		_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_findObjectById4Any(new QueryFImp([=](int ret, const vector<tars::EndpointF> &activeEp, const vector<EndpointF> &inactiveEp){
			QueryF::async_response_findObjectById4Any(current, ret, activeEp, inactiveEp);
		}), id);

		return 0;
	}
}

Int32 QueryObjectsManager::findObjectById4All(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	registerQuery(id, ClientConfig::ModuleName, current);
	if(ObjectsCacheManager::getInstance()->hasObjectId(id))
	{
		return ObjectsCacheManager::getInstance()->findObjectById4All(id, activeEp, inactiveEp, current);
	}
	else
	{
		current->setResponse(false);

		_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_findObjectById4All(
				new QueryFImp([=](int ret, const vector<tars::EndpointF>& activeEp, const vector<EndpointF>& inactiveEp)
				{
					QueryF::async_response_findObjectById4All(current, ret, activeEp, inactiveEp);
				}), id);

		return 0;
	}
}

Int32 QueryObjectsManager::findObjectByIdInSameGroup(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	registerQuery(id, ClientConfig::ModuleName, current);
	if(ObjectsCacheManager::getInstance()->hasObjectId(id))
	{
		return ObjectsCacheManager::getInstance()->findObjectByIdInSameGroup(id, activeEp, inactiveEp, current);
	}
	else
	{
		current->setResponse(false);

		_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_findObjectByIdInSameGroup(
				new QueryFImp([=](int ret, const vector<tars::EndpointF>& activeEp, const vector<EndpointF>& inactiveEp)
				{
					QueryF::async_response_findObjectByIdInSameGroup(current, ret, activeEp, inactiveEp);
				}), id);

		return 0;
	}
}

Int32 QueryObjectsManager::findObjectByIdInSameStation(const std::string & id, const std::string & sStation, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	registerQuery(id, ClientConfig::ModuleName, current);
	if (ObjectsCacheManager::getInstance()->hasObjectId(id))
	{
		return ObjectsCacheManager::getInstance()->findObjectByIdInSameStation(id, sStation, activeEp, inactiveEp,
				current);
	}
	else
	{
		current->setResponse(false);

		_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_findObjectByIdInSameStation(
				new QueryFImp([=](int ret, const vector<tars::EndpointF>& activeEp, const vector<EndpointF>& inactiveEp)
				{
					QueryF::async_response_findObjectByIdInSameStation(current, ret, activeEp, inactiveEp);
				}), id, sStation);

		return 0;
	}
}

Int32 QueryObjectsManager::findObjectByIdInSameSet(const std::string & id,const std::string & setId,vector<EndpointF> &activeEp,vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	registerQuery(id, ClientConfig::ModuleName, current);

	if (ObjectsCacheManager::getInstance()->hasObjectId(id))
	{
		return ObjectsCacheManager::getInstance()->findObjectByIdInSameSet(id, setId, activeEp, inactiveEp, current);
	}
	else
	{
		current->setResponse(false);

		_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_findObjectByIdInSameSet(
				new QueryFImp([=](int ret, const vector<tars::EndpointF>& activeEp,
						const vector<EndpointF>& inactiveEp)
				{
					QueryF::async_response_findObjectByIdInSameSet(current, ret, activeEp, inactiveEp);
				}), id, setId);

		return 0;
	}
}


void QueryObjectsManager::closeQuery(CurrentPtr current)
{
	TC_RW_WLockT<TC_ThreadRWLocker> lock(_rwMutex);

	{
		auto it = _uidToQueryIds.find(current->getUId());

		if (it != _uidToQueryIds.end())
		{
			for (auto e: it->second)
			{
				auto idIt = _queries.find(e);

				if (idIt != _queries.end())
				{
					idIt->second.erase(current->getUId());

					if (idIt->second.empty())
					{
						_queries.erase(idIt);
					}
				}
			}
		}
	}
}

Int32 QueryObjectsManager::registerQuery(const std::string & id, const string& name, CurrentPtr current)
{
	bool flag = false;
	{
		TC_RW_WLockT<TC_ThreadRWLocker> lock(_rwMutex);

		if (_objs.find(id) == _objs.end())
		{
			flag = true;
		}

		_uidToQueryIds[current->getUId()].insert(id);

		_queries[id][current->getUId()] = current;
	}

	if(flag)
	{
		TLOGEX_DEBUG("push", "register change:" << id << endl);
		//对于tarsnode而言, 需要注册ip list的变更
		_queryFPrx->tars_hash((uint64_t)_queryFPrx.get())->async_registerChange(new QueryFImp(id), { id }, ClientConfig::ModuleName);
	}

	return 0;
}

void QueryObjectsManager::pushObjQuery(const string &id)
{
	unordered_map<int, CurrentPtr> data;
	{
		TC_RW_RLockT<TC_ThreadRWLocker> lock(_rwMutex);

		auto it = _queries.find(id);
		if (it != _queries.end())
		{
			data = it->second;
		}
	}

	TLOGEX_DEBUG("push", "push object change:" << id << ", client size: " << data.size() << endl);

	for(auto &e : data)
	{
		QueryPushF::async_response_push_onQuery(e.second, id);
	}

}

Int32 QueryObjectsManager::registerChange(const vector<std::string> & ids, CurrentPtr current)
{
	TLOG_ERROR("should not be called" << endl);
	return 0;
}

void QueryObjectsManager::addObjectId(const string &id)
{
	TC_RW_WLockT<TC_ThreadRWLocker> lock(_rwMutex);

	_objs.insert(id);
}