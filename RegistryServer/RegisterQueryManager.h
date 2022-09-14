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
	void registerQuery(const string &id, CurrentPtr current);

	//
	void registerChange(const string &id, CurrentPtr current);

	//
	void closeQuery(CurrentPtr current);

	void pushObj(const ObjectsCache& allCache, const ObjectsCache &objectsCache);

	void terminate();

protected:
	virtual void run();

	void onPush(const ObjectsCache &objectsCache);

	unordered_map<int, CurrentPtr> getChanges(const string &id);

	unordered_map<int, CurrentPtr> getQueries(const string &id);

protected:

	//
	std::mutex			_mutex;

	//<id, <uid, current>>
	unordered_map<string, unordered_map<int, CurrentPtr>> _queries;

	//<uid, ids>
	unordered_map<int, unordered_set<string>> _uidToQueryIds;

	//<id, <uid, current>>
	unordered_map<string, unordered_map<int, CurrentPtr>> _changes;

	//<uid, ids>
	unordered_map<int, unordered_set<string>> _uidToChangeIds;

	bool _terminate = false;

	//需要通知的
	TC_ThreadQueue<ObjectsCache> _ids;
};


#endif //FRAMEWORK_REGISTERQUERYMANAGER_H
