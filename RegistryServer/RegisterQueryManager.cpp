//
// Created by jarod on 2022/9/13.
//

#include "RegisterQueryManager.h"
#include "servant/QueryPushF.h"

void RegisterQueryManager::registerChange(const string &id, CurrentPtr current)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_uidToChangeIds[current->getUId()].insert(id);

	_changes[id][current->getUId()] = current;
}

void RegisterQueryManager::registerQuery(const string &id, CurrentPtr current)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_uidToQueryIds[current->getUId()].insert(id);

	_queries[id][current->getUId()] = current;

}

void RegisterQueryManager::closeQuery(CurrentPtr current)
{
	std::lock_guard<std::mutex> lock(_mutex);

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
	_ids.push_back(change);
}

void RegisterQueryManager::terminate()
{
	_terminate = true;
	_ids.notifyT();
}

unordered_map<int, CurrentPtr> RegisterQueryManager::getChanges(const string &id)
{
	std::lock_guard<std::mutex> lock(_mutex);

	auto it = _changes.find(id);

	if(it != _changes.end())
	{
		return it->second;
	}

	return unordered_map<int, CurrentPtr>();
}

unordered_map<int, CurrentPtr> RegisterQueryManager::getQueries(const string &id)
{
	std::lock_guard<std::mutex> lock(_mutex);

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
			QueryPushF::async_response_push_onChange(e.second, cache.first, cache.second.vActiveEndpoints, cache.second.vInactiveEndpoints);
		}

		auto queries = getQueries(cache.first);
		for (auto e: queries)
		{
			QueryPushF::async_response_push_onQuery(e.second, cache.first);
		}
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