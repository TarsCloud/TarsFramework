//
// Created by jarod on 2022/9/14.
//

#ifndef _OBJECTSCACHEMANAGER_H
#define _OBJECTSCACHEMANAGER_H

#include "util/tc_singleton.h"
#include "util/tc_thread.h"
#include "servant/Application.h"
#include "servant/QueryF.h"
#include "servant/QueryPushF.h"

using namespace tars;
typedef map<string, ObjectItem> ObjectsCache;

typedef map<string,map<string,vector<SetServerInfo> > > SetDivisionCache;

class ObjectsCacheManager  : public TC_Singleton<ObjectsCacheManager>
{
public:

	ObjectsCacheManager();

	void onChange(const ObjectsCache& objCache);
	void onUpdate(const ObjectsCache& objCache);
	void onChange( const std::string& id,  const ObjectItem &item);
	void onChangeGroupPriorityEntry( const map<tars::Int32, tars::GroupPriorityEntry>& group);
	void onChangeSetInfo( const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo);
	void onUpdateSetInfo( const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo);
	void onChangeServerGroupRule(const vector<map<string, string>> &serverGroupRule);

	map<std::string, map<std::string, vector<tars::SetServerInfo>>> getSetInfo();

	/**
	 * 是否拥有objectId
	 * @param id
	 * @return
	 */
	bool hasObjectId(const string &id);

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

	ObjectsCache &getReaderObjectsCache() { return _objectsCache.getReaderData(); }

	/**
     * 根据ip获取组id
     * @return int <0 失败 其它正常
     */
	int getGroupId(const string& ip);

    /**
     * 根据组名获取组id
     * @return int <0 失败 其它正常
     */
    int getGroupIdByName(const string& sGroupName);

protected:

	vector<EndpointF> findObjectById(const string& id);
	int findObjectById4All(const string& id, vector<EndpointF>& activeEp, vector<EndpointF>& inactiveEp);
	int findObjectByIdInSameSet(const string& sID, const vector<string>& vtSetInfo, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os);
	int findObjectByIdInSameSet(const string& sSetId, const vector<SetServerInfo>& vSetServerInfo, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os);
	int findObjectByIdInSameStation(const std::string& sID, const std::string& sStation, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os);
	int findObjectByIdInGroupPriority(const std::string& sID, const std::string& sIP, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os);
	vector<EndpointF> getEpsByGroupId(const vector<EndpointF>& vecEps, const GroupUseSelect GroupSelect, int iGroupId, ostringstream& os);
	vector<EndpointF> getEpsByGroupId(const vector<EndpointF>& vecEps, const GroupUseSelect GroupSelect, const map<int, bool>& setGroupID, ostringstream& os);

	string eFunTostr(const FUNID eFnId);

	void doDaylog(const FUNID eFnId,const string& id,const vector<EndpointF> &activeEp, const vector<EndpointF> &inactiveEp, const CurrentPtr& current,const ostringstream& os,const string& sSetid = "");

protected:

	//读写锁
	TC_ThreadRWLocker _rwMutex;

	//对象列表缓存
	TC_ReadersWriterData<ObjectsCache>    _objectsCache;

	//set划分缓存
	TC_ReadersWriterData<SetDivisionCache> _setDivisionCache;

	//优先级的序列
	TC_ReadersWriterData<std::map<int, GroupPriorityEntry>> _mapGroupPriority;

	//分组信息
	TC_ReadersWriterData<vector<map<string, string>>> _serverGroupRule;

	unordered_map<string,int> _serverGroupCache;
	unordered_map<string,int> _groupNameCache;

};


#endif //FRAMEWORK_OBJECTSCACHEMANAGER_H
