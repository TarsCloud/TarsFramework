//
// Created by jarod on 2022/9/13.
//

#include "RegisterQueryManager.h"
#include "servant/QueryPushF.h"
#include "servant/RemoteLogger.h"

void RegisterQueryManager::registerChange(const vector<string> &ids, const string &name, CurrentPtr current)
{
	TLOGEX_DEBUG("push", name << ", " << current->getHostName() << " " << TC_Common::tostr(ids.begin(), ids.end(), " ") << endl);

	TC_RW_WLockT<TC_ThreadRWLocker> lock(_mutex);

	for(auto &id : ids)
	{
		_uidToChangeIds[current->getUId()].insert(id);

		_changes[id][current->getUId()] = current;
	}

	_changeCurrents[current->getUId()] = current;
}

void RegisterQueryManager::registerQuery(const string &id, const string &name, CurrentPtr current)
{
	TLOGEX_DEBUG("push", name << ", " << id << " " << current->getHostName() << endl);

	TC_RW_WLockT<TC_ThreadRWLocker> lock(_mutex);

	_uidToQueryIds[current->getUId()].insert(id);

	_queries[id][current->getUId()] = current;

}

void RegisterQueryManager::closeQuery(CurrentPtr current)
{
	TC_RW_WLockT<TC_ThreadRWLocker> lock(_mutex);

	{
		auto it = _uidToChangeIds.find(current->getUId());

		if (it != _uidToChangeIds.end())
		{
			for (auto e: it->second)
			{
				auto idIt = _changes.find(e);

				if (idIt != _changes.end())
				{
					idIt->second.erase(current->getUId());

					if (idIt->second.empty())
					{
						_changes.erase(idIt);
					}
				}
			}
		}

		_changeCurrents.erase(current->getUId());
	}

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

void RegisterQueryManager::pushObj(const ObjectsCache& allCache, const ObjectsCache &objectsCache)
{
	ObjectsCache change;

	for(auto e : objectsCache)
	{
		auto it = allCache.find(e.first);

		if(it != allCache.end())
		{
			if(it->second.vActiveEndpoints != e.second.vActiveEndpoints)
			{
				change.insert(e);
			}
		}
		else
		{
			change.insert(e);
		}
	}

	if(!change.empty())
	{
		TLOGEX_DEBUG("push", "change objects size:" << change.size() << endl);

		for(auto &c : change)
		{
			TLOGEX_DEBUG("push", "ids:" << c.first << ", active size:" << c.second.vActiveEndpoints.size() << ", inactive size:" << c.second.vInactiveEndpoints.size() << endl);
		}
		_ids.push_back(change);
	}
}

void RegisterQueryManager::terminate()
{
	_terminate = true;
	_ids.notifyT();
}

unordered_map<int, CurrentPtr> RegisterQueryManager::getChanges(const string &id)
{
	TC_RW_RLockT<TC_ThreadRWLocker> lock(_mutex);

	auto it = _changes.find(id);

	if(it != _changes.end())
	{
		return it->second;
	}

	return unordered_map<int, CurrentPtr>();
}

unordered_map<int, CurrentPtr> RegisterQueryManager::getQueries(const string &id)
{
	TC_RW_RLockT<TC_ThreadRWLocker> lock(_mutex);

	auto it = _queries.find(id);

	if(it != _queries.end())
	{
		return it->second;
	}

	return unordered_map<int, CurrentPtr>();
}

void RegisterQueryManager::onPush(const ObjectsCache &objectsCache)
{
	for(auto cache : objectsCache)
	{
		auto changes = getChanges(cache.first);
		for (auto e: changes)
		{
			QueryPushF::async_response_push_onChange(e.second, cache.first, cache.second);
		}

		auto queries = getQueries(cache.first);
		for (auto e: queries)
		{
			TLOGEX_DEBUG("push", cache.first << " " << e.second->getHostName() << endl);
			QueryPushF::async_response_push_onQuery(e.second, cache.first);
		}
	}
}

void RegisterQueryManager::pushGroupPriorityEntry( const map<tars::Int32, tars::GroupPriorityEntry>& group)
{
	if(group.empty())
	{
		return;
	}
	TC_RW_RLockT<TC_ThreadRWLocker> lock(_mutex);

	for(auto &e: _changeCurrents)
	{
		QueryPushF::async_response_push_onChangeGroupPriorityEntry(e.second, group);
	}
}

void RegisterQueryManager::pushSetInfo( const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo)
{
	if(setInfo.empty())
	{
		return;
	}
	TC_RW_RLockT<TC_ThreadRWLocker> lock(_mutex);
	for(auto &e: _changeCurrents)
	{
		QueryPushF::async_response_push_onChangeSetInfo(e.second, setInfo);
	}
}

void RegisterQueryManager::pushServerGroupRule(const vector<map<string, string>> &serverGroupRule)
{
	if(serverGroupRule.empty())
	{
		return;
	}

	TC_RW_RLockT<TC_ThreadRWLocker> lock(_mutex);
	for(auto &e: _changeCurrents)
	{
		QueryPushF::async_response_push_onChangeServerGroupRule(e.second, serverGroupRule);
	}
}

void RegisterQueryManager::run()
{
	while(!_terminate)
	{
		ObjectsCache data;
		if(_ids.pop_front(data, -1, true))
		{
			onPush(data);
		}
	}
}