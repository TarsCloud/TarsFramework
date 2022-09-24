//
// Created by jarod on 2022/9/14.
//

#include "ObjectsCacheManager.h"

//////////////////////////////////////////////////////////////////////////////////////////////

ObjectsCacheManager::ObjectsCacheManager()
{
}

void ObjectsCacheManager::onChange(const ObjectsCache& objCache)
{
	TLOGEX_DEBUG("push", "change size:" << objCache.size() << endl);
	_objectsCache.getWriterData() = objCache;
	_objectsCache.swap();
}

void ObjectsCacheManager::onUpdate(const ObjectsCache& objCache)
{
	if(objCache.empty())
	{
		return;
	}

	TLOGEX_DEBUG("push", "update objects size:" << objCache.size() << endl);

	for(auto e : objCache)
	{
		TLOGEX_DEBUG("push", "update objects size:" << e.first << ", active size:" << e.second.vActiveEndpoints.size() << ", inactive size:" << e.second.vInactiveEndpoints.size() << endl);
	}
	_objectsCache.getWriterData() = _objectsCache.getReaderData();
	ObjectsCache& tmpObjCache = _objectsCache.getWriterData();

	auto it = objCache.begin();
	for (; it != objCache.end(); it++)
	{
		//增量的时候加载的是服务的所有节点，因此这里直接替换
		tmpObjCache[it->first] = it->second;
	}

	_objectsCache.swap();
}

void ObjectsCacheManager::onChange( const std::string& id, const ObjectItem &item)
{
	TLOGEX_DEBUG("push", "change id:" << id << ", item size:" << item.vActiveEndpoints.size() << ", inactive size:" << item.vInactiveEndpoints.size() << endl);

	auto &data = _objectsCache.getWriterData();
	data[id] = item;

	_objectsCache.swap();

}

void ObjectsCacheManager::onChangeGroupPriorityEntry( const map<tars::Int32, tars::GroupPriorityEntry>& group)
{
	TLOGEX_DEBUG("push", "group size:" << group.size() << endl);

	_mapGroupPriority.getWriterData() = group;

	_mapGroupPriority.swap();
}

void ObjectsCacheManager::onChangeSetInfo( const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo)
{
	TLOGEX_DEBUG("push", "set size:" << setInfo.size() << endl);

	_setDivisionCache.getWriterData() = setInfo;

	_setDivisionCache.swap();
}

map<std::string, map<std::string, vector<tars::SetServerInfo>>> ObjectsCacheManager::getSetInfo()
{
	return _setDivisionCache.getReaderData();
}

void ObjectsCacheManager::onUpdateSetInfo( const map<std::string, map<std::string, vector<tars::SetServerInfo> > >& setInfo)
{
	if(setInfo.empty())
	{
		return;
	}
	TLOGEX_DEBUG("push", "set size:" << setInfo.size() << endl);

	_setDivisionCache.getWriterData() = _setDivisionCache.getReaderData();
	SetDivisionCache& tmpsetCache = _setDivisionCache.getWriterData();
	auto it = setInfo.begin();
	for (; it != setInfo.end(); it++)
	{
		//有set信息才更新
		if (it->second.size() > 0)
		{
			tmpsetCache[it->first] = it->second;
		}
		else if (tmpsetCache.count(it->first))
		{
			//这个服务的所有节点都没有启用set，删除缓存中的set信息
			tmpsetCache.erase(it->first);
		}
	}
	_setDivisionCache.swap();
}

void ObjectsCacheManager::onChangeServerGroupRule(const vector<map<string, string>> &serverGroupRule)
{
	TLOGEX_DEBUG("push", "serverGroupRule size:" << serverGroupRule.size() << endl);

	_serverGroupRule.getWriterData() = serverGroupRule;
	_serverGroupRule.swap();

	TC_RW_WLockT<TC_ThreadRWLocker> lock(_rwMutex);
	_serverGroupCache.clear();

//	_groupIdMap.getWriterData() = groupId;
//	_groupIdMap.swap();
//	_groupNameMap.getWriterData() = groupNameMap;
//	_groupNameMap.swap();
}

bool ObjectsCacheManager::hasObjectId(const string &id)
{
	return _objectsCache.getReaderData().find(id) != _objectsCache.getReaderData().end();
}

vector<EndpointF> ObjectsCacheManager::findObjectById(const string & id, CurrentPtr current)
{
	vector<EndpointF> eps = findObjectById(id);

	ostringstream os;
	doDaylog(FUNID_findObjectById,id,eps,vector<EndpointF>(),current,os);

	return eps;
}

Int32 ObjectsCacheManager::findObjectById4Any(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	int iRet = findObjectById4All(id, activeEp, inactiveEp);

	ostringstream os;
	doDaylog(FUNID_findObjectById4Any,id,activeEp,inactiveEp,current,os);

	return iRet;
}

Int32 ObjectsCacheManager::findObjectById4All(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	ostringstream os;

	int iRet = findObjectByIdInGroupPriority(id,current->getHostName() ,activeEp, inactiveEp,os);

	doDaylog(FUNID_findObjectById4All,id,activeEp,inactiveEp,current, os);

	return iRet;
}

Int32 ObjectsCacheManager::findObjectByIdInSameGroup(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	ostringstream os;
	TLOGINFO(__FUNCTION__ << ":" << __LINE__ << "|" << id << "|" << current->getHostName()  << endl);

	int iRet = findObjectByIdInGroupPriority(id, current->getHostName() , activeEp, inactiveEp, os);

	doDaylog(FUNID_findObjectByIdInSameGroup,id,activeEp,inactiveEp,current,os);

	return iRet;
}

Int32 ObjectsCacheManager::findObjectByIdInSameStation(const std::string & id, const std::string & sStation, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	ostringstream os;

	int iRet = findObjectByIdInSameStation(id, sStation, activeEp, inactiveEp, os);

	doDaylog(FUNID_findObjectByIdInSameStation,id,activeEp,inactiveEp,current,os);

	return iRet;
}

vector<EndpointF> ObjectsCacheManager::findObjectById(const string& id)
{
	ObjectsCache::iterator it;
	ObjectsCache& usingCache = _objectsCache.getReaderData();

	if ((it = usingCache.find(id)) != usingCache.end())
	{
		// 不能是引用，会改变原始缓存数据
		std::vector<tars::EndpointF> vtEp = it->second.vActiveEndpoints;

		return vtEp;
	}
	else
	{
		vector<EndpointF> activeEp;
		return activeEp;
	}
}

int ObjectsCacheManager::findObjectById4All(const string& id, vector<EndpointF>& activeEp, vector<EndpointF>& inactiveEp)
{
	TLOG_DEBUG(__FUNCTION__ << " id: " << id << endl);

	ObjectsCache::iterator it;
	ObjectsCache& usingCache = _objectsCache.getReaderData();

	if ((it = usingCache.find(id)) != usingCache.end())
	{
		activeEp   = it->second.vActiveEndpoints;
		inactiveEp = it->second.vInactiveEndpoints;

//		LOAD_BALANCE_INS->getDynamicWeight(id, activeEp);
	}
	else
	{
		activeEp.clear();
		inactiveEp.clear();
	}

	return  0;
}

int ObjectsCacheManager::getGroupId(const string& ip)
{
	{
		TC_RW_RLockT<TC_ThreadRWLocker> lock(_rwMutex);
		auto it = _serverGroupCache.find(ip);
		if (it != _serverGroupCache.end())
		{
			return it->second;
		}
	}

	auto &vServerGroupInfo = _serverGroupRule.getReaderData();

	bool bFind = false;

	int groupId = -1;
	for (auto it = vServerGroupInfo.begin(); it != vServerGroupInfo.end(); it++)
	{
		groupId = TC_Common::strto<int>(it->find("group_id")->second);
		string sOrder = it->find("ip_order")->second;
		vector<string> vAllowIp = TC_Common::sepstr<string>(it->find("allow_ip_rule")->second, ",;|");
		vector<string> vDennyIp = TC_Common::sepstr<string>(it->find("denny_ip_rule")->second, ",;|");

		if (sOrder == "allow_denny")
		{
			if (TC_Common::matchPeriod(ip, vAllowIp))
			{
				bFind = true;
				break;
			}
		}
		else if (sOrder == "denny_allow")
		{
			if (TC_Common::matchPeriod(ip, vDennyIp))
			{
				//在不允许的ip列表中则不属于本行所对应组  继续匹配查找
				continue;
			}
			if (TC_Common::matchPeriod(ip, vAllowIp))
			{
				bFind = true;
				break;
			}
		}
	}

	if (bFind == true)
	{
		TC_RW_WLockT<TC_ThreadRWLocker> lock(_rwMutex);

		_serverGroupCache[ip] = groupId;

		return groupId;
	}

	return -1;
//	map<string, int>& groupIdMap = _groupIdMap.getReaderData();
//	map<string, int>::iterator it = groupIdMap.find(ip);
//	if (it != groupIdMap.end())
//	{
//		return it->second;
//	}
//
//	uint32_t uip = stringIpToInt(ip);
//	string ipStar = Ip2StarStr(uip);
//	it = groupIdMap.find(ipStar);
//	if (it != groupIdMap.end())
//	{
//		return it->second;
//	}
//
//	return -1;
}

int ObjectsCacheManager::getGroupIdByName(const string& sGroupName)
{
	{
		TC_RW_RLockT<TC_ThreadRWLocker> lock(_rwMutex);
		auto it = _groupNameCache.find(sGroupName);
		if (it != _groupNameCache.end())
		{
			return it->second;
		}
	}

	auto &vServerGroupInfo = _serverGroupRule.getReaderData();

	int groupId = -1;
	for (auto it = vServerGroupInfo.begin(); it != vServerGroupInfo.end(); it++)
	{
		groupId = TC_Common::strto<int>(it->find("group_id")->second);

		TC_RW_WLockT<TC_ThreadRWLocker> lock(_rwMutex);

		_groupNameCache[it->find("group_name")->second] = groupId;
	}

	{
		TC_RW_RLockT<TC_ThreadRWLocker> lock(_rwMutex);
		auto it = _groupNameCache.find(sGroupName);
		if (it != _groupNameCache.end())
		{
			return it->second;
		}
	}

	return -1;

//    int iGroupId = -1;
//    try
//    {
//        if (sGroupName.empty())
//        {
//            return iGroupId;
//        }
//
//        map<string, int>& groupNameMap = _groupNameMap.getReaderData();
//        map<string, int>::iterator it = groupNameMap.find(sGroupName);
//        if (it != groupNameMap.end())
//        {
//            TLOGINFO("CDbHandle::getGroupIdByName: "<< sGroupName << "|" << it->second << endl);
//            return it->second;
//        }
//    }
//    catch (exception& ex)
//    {
//        TLOG_ERROR("CDbHandle::getGroupIdByName exception:" << ex.what() << endl);
//    }
//    catch (...)
//    {
//        TLOG_ERROR("CDbHandle::getGroupIdByName unknown exception" << endl);
//    }
//
//    TLOGINFO("CDbHandle::getGroupIdByName " << sGroupName << "|" << endl);
//    return -1;
}

vector<EndpointF> ObjectsCacheManager::getEpsByGroupId(const vector<EndpointF>& vecEps, const GroupUseSelect GroupSelect, int iGroupId, ostringstream& os)
{
	os << "|";
	vector<EndpointF> vResult;

	for (unsigned i = 0; i < vecEps.size(); i++)
	{
		os << vecEps[i].host << ":" << vecEps[i].port << "(" << vecEps[i].groupworkid << ");";
		if (GroupSelect == ENUM_USE_WORK_GROUPID && vecEps[i].groupworkid == iGroupId)
		{
			vResult.push_back(vecEps[i]);
		}
		if (GroupSelect == ENUM_USE_REAL_GROUPID && vecEps[i].grouprealid == iGroupId)
		{
			vResult.push_back(vecEps[i]);
		}
	}

	return vResult;
}

vector<EndpointF> ObjectsCacheManager::getEpsByGroupId(const vector<EndpointF>& vecEps, const GroupUseSelect GroupSelect, const map<int, bool>& setGroupID, ostringstream& os)
{
	os << "|";
	std::vector<EndpointF> vecResult;

	for (std::vector<EndpointF>::size_type i = 0; i < vecEps.size(); i++)
	{
		os << vecEps[i].host << ":" << vecEps[i].port << "(" << vecEps[i].groupworkid << ")";
		if (GroupSelect == ENUM_USE_WORK_GROUPID && setGroupID.count(vecEps[i].groupworkid) == 1)
		{
			vecResult.push_back(vecEps[i]);
		}
		if (GroupSelect == ENUM_USE_REAL_GROUPID && setGroupID.count(vecEps[i].grouprealid) == 1)
		{
			vecResult.push_back(vecEps[i]);
		}
	}

	return vecResult;
}

int ObjectsCacheManager::findObjectByIdInGroupPriority(const std::string& sID, const std::string& sIP, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os)
{
	vecActive.clear();
	vecInactive.clear();

	int iClientGroupID = getGroupId(sIP);
	os << "|(" << iClientGroupID << ")";
	if (iClientGroupID == -1)
	{
		return findObjectById4All(sID, vecActive, vecInactive);
	}

	ObjectsCache& usingCache = _objectsCache.getReaderData();
	ObjectsCache::iterator itObject = usingCache.find(sID);
	if (itObject == usingCache.end()) return 0;

	//首先在同组中查找
	{
		vecActive     = getEpsByGroupId(itObject->second.vActiveEndpoints, ENUM_USE_WORK_GROUPID, iClientGroupID, os);
		vecInactive    = getEpsByGroupId(itObject->second.vInactiveEndpoints, ENUM_USE_WORK_GROUPID, iClientGroupID, os);
		os << "|(In Same Group: " << iClientGroupID << " Active=" << vecActive.size() << " Inactive=" << vecInactive.size() << ")";
	}

	//启用分组，但同组中没有找到，在优先级序列中查找
	std::map<int, GroupPriorityEntry> & mapPriority = _mapGroupPriority.getReaderData();
	for (std::map<int, GroupPriorityEntry>::iterator it = mapPriority.begin(); it != mapPriority.end() && vecActive.empty(); it++)
	{
		if (it->second.setGroupID.count(iClientGroupID) == 0)
		{
			os << "|(Not In Priority " << it->second.sGroupID << ")";
			continue;
		}
		vecActive    = getEpsByGroupId(itObject->second.vActiveEndpoints, ENUM_USE_WORK_GROUPID, it->second.setGroupID, os);
		vecInactive    = getEpsByGroupId(itObject->second.vInactiveEndpoints, ENUM_USE_WORK_GROUPID, it->second.setGroupID, os);
		os << "|(In Priority: " << it->second.sGroupID << " Active=" << vecActive.size() << " Inactive=" << vecInactive.size() << ")";
	}

	//没有同组的endpoit,匹配未启用分组的服务
	if (vecActive.empty())
	{
		vecActive    = getEpsByGroupId(itObject->second.vActiveEndpoints, ENUM_USE_WORK_GROUPID, -1, os);
		vecInactive    = getEpsByGroupId(itObject->second.vInactiveEndpoints, ENUM_USE_WORK_GROUPID, -1, os);
		os << "|(In No Grouop: Active=" << vecActive.size() << " Inactive=" << vecInactive.size() << ")";
	}

	//在未分组的情况下也没有找到，返回全部地址(此时基本上所有的服务都已挂掉)
	if (vecActive.empty())
	{
		vecActive    = itObject->second.vActiveEndpoints;
		vecInactive    = itObject->second.vInactiveEndpoints;
		os << "|(In All: Active=" << vecActive.size() << " Inactive=" << vecInactive.size() << ")";
	}

//	LOAD_BALANCE_INS->getDynamicWeight(sID, vecActive);

	return 0;
}

int ObjectsCacheManager::findObjectByIdInSameStation(const std::string& sID, const std::string& sStation, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os)
{
	vecActive.clear();
	vecInactive.clear();

	//获得station所有组
	std::map<int, GroupPriorityEntry> & mapPriority         = _mapGroupPriority.getReaderData();
	std::map<int, GroupPriorityEntry>::iterator itGroup     = mapPriority.end();
	for (itGroup = mapPriority.begin(); itGroup != mapPriority.end(); itGroup++)
	{
		if (itGroup->second.sStation != sStation) continue;

		break;
	}

	if (itGroup == mapPriority.end())
	{
		os << "|not found station:" << sStation;
		return -1;
	}

	ObjectsCache& usingCache = _objectsCache.getReaderData();
	ObjectsCache::iterator itObject = usingCache.find(sID);
	if (itObject == usingCache.end()) return 0;

	//查找对应所有组下的IP地址
	vecActive    = getEpsByGroupId(itObject->second.vActiveEndpoints, ENUM_USE_REAL_GROUPID, itGroup->second.setGroupID, os);
	vecInactive    = getEpsByGroupId(itObject->second.vInactiveEndpoints, ENUM_USE_REAL_GROUPID, itGroup->second.setGroupID, os);

//	LOAD_BALANCE_INS->getDynamicWeight(sID, vecActive);

	return 0;
}

Int32 ObjectsCacheManager::findObjectByIdInSameSet(const std::string & id,const std::string & setId,vector<EndpointF> &activeEp,vector<EndpointF> &inactiveEp, CurrentPtr current)
{
	vector<string> vtSetInfo = TC_Common::sepstr<string>(setId,".");

	if (vtSetInfo.size()!=3 ||(vtSetInfo.size()==3&&(vtSetInfo[0]=="*"||vtSetInfo[1]=="*")))
	{
		TLOG_ERROR("QueryImp::findObjectByIdInSameSet:|set full name error[" << id << "_" << setId <<"]|" << current->getHostName()  << endl);
		return -1;
	}

	ostringstream os;
	int iRet = findObjectByIdInSameSet(id, vtSetInfo, activeEp, inactiveEp, os);
	if (-1 == iRet)
	{
		//未启动set，启动ip分组策略
		return findObjectByIdInSameGroup(id, activeEp, inactiveEp, current);
	}
	else if (-2 == iRet)
	{
		//启动了set，但未找到任何服务节点
		TLOG_ERROR("QueryImp::findObjectByIdInSameSet |no one server found for [" << id << "_" << setId <<"]|" << current->getHostName()  << endl);
		return -1;
	}
	else if (-3 == iRet)
	{
		//启动了set，但未找到任何地区set,严格上不应该出现此类情形,配置错误或主调设置错误会引起此类错误
		TLOG_ERROR("QueryImp::findObjectByIdInSameSet |no set area found [" << id << "_" << setId <<"]|" << current->getHostName()   << endl);
		return -1;
	}

	doDaylog(FUNID_findObjectByIdInSameSet,id,activeEp,inactiveEp,current,os,setId);

	return iRet;
}

int ObjectsCacheManager::findObjectByIdInSameSet(const string& sID, const vector<string>& vtSetInfo, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os)
{
	string sSetName   = vtSetInfo[0];
	string sSetArea   = vtSetInfo[0] + "." + vtSetInfo[1];
	string sSetId     = vtSetInfo[0] + "." + vtSetInfo[1] + "." + vtSetInfo[2];

	SetDivisionCache& usingSetDivisionCache = _setDivisionCache.getReaderData();
	SetDivisionCache::iterator it = usingSetDivisionCache.find(sID);
	if (it == usingSetDivisionCache.end())
	{
		//此情况下没启动set
		TLOGINFO("CDbHandle::findObjectByIdInSameSet:" << __LINE__ << "|" << sID << " haven't start set|" << sSetId << endl);
		return -1;
	}

	map<string, vector<SetServerInfo> >::iterator setNameIt = it->second.find(sSetName);
	if (setNameIt == (it->second).end())
	{
		//此情况下没启动set
		TLOGINFO("CDbHandle::findObjectByIdInSameSet:" << __LINE__ << "|" << sID << " haven't start set|" << sSetId << endl);
		return -1;
	}

	if (vtSetInfo[2] == "*")
	{
		//检索通配组和set组中的所有服务
		vector<SetServerInfo>  vServerInfo = setNameIt->second;
		for (size_t i = 0; i < vServerInfo.size(); i++)
		{
			if (vServerInfo[i].sSetArea == sSetArea)
			{
				if (vServerInfo[i].bActive)
				{
					vecActive.push_back(vServerInfo[i].epf);
				}
				else
				{
					vecInactive.push_back(vServerInfo[i].epf);
				}
			}
		}

		return (vecActive.empty() && vecInactive.empty()) ? -2 : 0;
	}
	else
	{
		// 1.从指定set组中查找
		int iRet = findObjectByIdInSameSet(sSetId, setNameIt->second, vecActive, vecInactive, os);
		if (iRet != 0 && vtSetInfo[2] != "*")
		{
			// 2. 步骤1中没找到，在通配组里找
			string sWildSetId =  vtSetInfo[0] + "." + vtSetInfo[1] + ".*";
			iRet = findObjectByIdInSameSet(sWildSetId, setNameIt->second, vecActive, vecInactive, os);
		}

		return iRet;
	}


}

int ObjectsCacheManager::findObjectByIdInSameSet(const string& sSetId, const vector<SetServerInfo>& vSetServerInfo, std::vector<EndpointF>& vecActive, std::vector<EndpointF>& vecInactive, std::ostringstream& os)
{
	for (size_t i = 0; i < vSetServerInfo.size(); ++i)
	{
		if (vSetServerInfo[i].sSetId == sSetId)
		{
			if (vSetServerInfo[i].bActive)
			{
				vecActive.push_back(vSetServerInfo[i].epf);
			}
			else
			{
				vecInactive.push_back(vSetServerInfo[i].epf);
			}
		}
	}

	int iRet = (vecActive.empty() && vecInactive.empty()) ? -2 : 0;
	return iRet;
}

void ObjectsCacheManager::doDaylog(const FUNID eFnId,const string& id,const vector<EndpointF> &activeEp, const vector<EndpointF> &inactiveEp, const CurrentPtr& current,const ostringstream& os,const string& sSetid)
{
		string sEpList;
		for (size_t i = 0; i < activeEp.size(); i++) {
			if (0 != i) {
				sEpList += ";";
			}
			sEpList += activeEp[i].host + ":" + TC_Common::tostr(activeEp[i].port) + ", " + TC_Common::tostr(activeEp[i].weight)
					   + ", " + TC_Common::tostr(activeEp[i].weightType);
		}

		sEpList += "|";

		for (size_t i = 0; i < inactiveEp.size(); i++) {
			if (0 != i) {
				sEpList += ";";
			}
			sEpList += inactiveEp[i].host + ":" + TC_Common::tostr(inactiveEp[i].port) + ", " + TC_Common::tostr(inactiveEp[i].weight)
					   + ", " + TC_Common::tostr(inactiveEp[i].weightType);
		}

		switch (eFnId) {
		case FUNID_findObjectById4All:
		case FUNID_findObjectByIdInSameGroup: {
			FDLOG("query_idc") << eFunTostr(eFnId) << "|" << current->getHostName() << "|" << current->getPort()
							   << "|" << id << "|" << sSetid << "|" << sEpList << os.str() << endl;
		}
			break;
		case FUNID_findObjectByIdInSameSet: {
			FDLOG("query_set") << eFunTostr(eFnId) << "|" << current->getHostName() << "|" << current->getPort()
							   << "|" << id << "|" << sSetid << "|" << sEpList << os.str() << endl;
		}
			break;
		case FUNID_findObjectById4Any:
		case FUNID_findObjectById:
		case FUNID_findObjectByIdInSameStation:
		default: {
			FDLOG("query") << eFunTostr(eFnId) << "|" << current->getHostName() << "|" << current->getPort() << "|"
						   << id << "|" << sSetid << "|" << sEpList << os.str() << endl;
		}
			break;
		}
}

string ObjectsCacheManager::eFunTostr(const FUNID eFnId)
{
	return etos(eFnId).substr(5);
}