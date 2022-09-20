//
// Created by jarod on 2022/9/13.
//

#ifndef FRAMEWORK_REGISTERQUERYMANAGER_H
#define FRAMEWORK_REGISTERQUERYMANAGER_H

#include "util/tc_singleton.h"
#include "util/tc_thread.h"
#include "servant/Application.h"
#include "DbHandle.h"

using namespace tars;

class RegisterQueryManager : public TC_Singleton<RegisterQueryManager>, public TC_Thread
{
public:

	//
	void registerQuery(const string &id, const string &name, CurrentPtr current);

	//
	void registerChange(const vector<string> &id, const string &name, CurrentPtr current);

	//
	void closeQuery(CurrentPtr current);

	void pushObj(const ObjectsCache& allCache, const ObjectsCache &objectsCache);

	void pushGroupPriorityEntry( const map<tars::Int32, tars::GroupPriorityEntry>& group);
	void pushSetInfo( const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo);
	void pushServerGroupRule(const vector<map<string, string>> &serverGroupRule);

	void terminate();

protected:
	virtual void run();

	void onPush(const ObjectsCache &objectsCache);

	unordered_map<int, CurrentPtr> getChanges(const string &id);

	unordered_map<int, CurrentPtr> getQueries(const string &id);

protected:

	//
	TC_ThreadRWLocker _mutex;
//	std::mutex			_mutex;

	//<id, <uid, current>>
	unordered_map<string, unordered_map<int, CurrentPtr>> _queries;

	//<uid, ids>
	unordered_map<int, unordered_set<string>> _uidToQueryIds;

	//<id, <uid, current>>
	unordered_map<string, unordered_map<int, CurrentPtr>> _changes;

	//<uid, ids>
	unordered_map<int, unordered_set<string>> _uidToChangeIds;

	//change current
	 unordered_map<int, CurrentPtr> _changeCurrents;

	bool _terminate = false;

	//需要通知的
	TC_ThreadQueue<ObjectsCache> _ids;
};


#endif //FRAMEWORK_REGISTERQUERYMANAGER_H
