//
// Created by jarod on 2022/9/14.
//

#ifndef FRAMEWORK_QUERYOBJECTSMANAGER_H
#define FRAMEWORK_QUERYOBJECTSMANAGER_H


#include "util/tc_singleton.h"
#include "util/tc_thread.h"
#include "servant/Application.h"
#include "servant/QueryF.h"
#include "servant/QueryPushF.h"

using namespace tars;


class QueryPushFImp : public QueryPushFPrxCallback
{
public:
	QueryPushFImp(const QueryFPrx &queryFPrx) : _queryFPrx(queryFPrx)
	{
	}

	virtual void onConnect(const TC_Endpoint& ep);
	virtual void callback_onChange(const std::string& id, const ObjectItem &item);
	virtual void callback_onChangeGroupPriorityEntry( const map<tars::Int32, tars::GroupPriorityEntry>& group);
	virtual void callback_onChangeSetInfo( const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo);
	virtual void callback_onChangeServerGroupRule(const vector<map<string, string>> &serverGroupRule);
protected:
	QueryFPrx _queryFPrx;
	std::mutex _mutex;
	vector<string> _objs;

};

class QueryObjectsManager  : public TC_Singleton<QueryObjectsManager>, public TC_Thread
{
public:

	QueryObjectsManager();

	/**
	 *
	 */
	void terminate();

	/**
	 * 根据id获取所有该对象的活动endpoint列表
	 */
	vector<EndpointF> findObjectById(const string & id, CurrentPtr current);

	/**
	 * 根据id获取所有对象,包括活动和非活动对象
	 */
	Int32 findObjectById4Any(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象所有endpoint列表
	 */
	Int32 findObjectById4All(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象同组endpoint列表
	 */
	Int32 findObjectByIdInSameGroup(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象指定归属地的endpoint列表
	 */
	Int32 findObjectByIdInSameStation(const std::string & id, const std::string & sStation, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象同set endpoint列表
	 */
	Int32 findObjectByIdInSameSet(const std::string & id,const std::string & setId,vector<EndpointF> &activeEp,vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 注册变化
	 * @param id
	 * @param current
	 * @return
	 */
	Int32 registerQuery(const std::string & id, const string &name, CurrentPtr current);

	/**
	 *
	 * @param current
	 */
	void closeQuery(CurrentPtr current);

	/**
	 * 注册变化(tarsnode这个接口其实不会被调用到, 除非tarsnode自己连接自己!)
	 * @param id
	 * @param current
	 * @return
	 */
	Int32 registerChange(const vector<std::string> & ids,  CurrentPtr current);

	/**
	 *
	 * @param id
	 */
	void addObjectId(const string &id);

	/**
	 *
	 * @param id
	 */
	void pushObjQuery(const string &id);

protected:
	virtual void run();

protected:

	//
	std::mutex			_mutex;

	bool _terminate = false;

	QueryFPrx		_queryFPrx;

	//关注的obj
	unordered_set<string> _objs;

	//<obj, <uid, current>>
	unordered_map<string, unordered_map<int, CurrentPtr>> _queries;

	//<uid, objs>
	unordered_map<int, unordered_set<string>> _uidToQueryIds;

	//读写锁
	TC_ThreadRWLocker _rwMutex;

};


#endif //FRAMEWORK_QUERYOBJECTSMANAGER_H
