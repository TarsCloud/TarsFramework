/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#include "QueryImp.h"
//#include "RequestDecoder.h"
#include "QueryItem.h"
#include "servant/Application.h"

using namespace std;

//////////////////////////////////////////////////////
void QueryImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void QueryImp::destroy()
{
    //destroy servant here:
    //...
}

//sqlPart:  [dataid]=[tars_stat]   [method]=[query] [date1]=[20200304]  [date2]=[20200304]
// [groupCond]=[ group by f_tflag]  [groupField]=[f_tflag]
// [sumField]=[ sum(succ_count),  sum(timeout_count),  sum(exce_count),  sum(total_time)]
// [tflag1]=[0000]  [tflag2]=[2360]  [uid]=[5|]
// [whereCond]=[ where slave_name like 'tars.tarsstat' and f_date='20200304' and f_tflag>='0000' and f_tflag<='2360' and slave_name like 'tars.tarsstat']
int QueryImp::query(const tars::MonitorQueryReq &req, tars::MonitorQueryRsp &rsp, tars::TarsCurrentPtr current)
{
	current->setResponse(false);
	QueryItem * pItem = new QueryItem();
	pItem->sUid     = req.uid;
	pItem->current  = current;

	TLOGDEBUG("query:" << req.writeToJsonString() << endl);

	pItem->mQuery["uid"]    = req.uid;
	pItem->mQuery["dataid"] = req.dataid;
	pItem->mQuery["method"] = req.method;
	pItem->mQuery["date1"]  = req.date;
	pItem->mQuery["date2"]  = req.date;
	pItem->mQuery["tflag1"] = req.tflag1;
	pItem->mQuery["tflag2"] = req.tflag2;

	string where = " where 1=1 ";
	for(size_t i = 0; i < req.conditions.size(); i++)
	{
		string op;
		switch(req.conditions[i].op)
		{
			case EQ:
				op = "=";
				break;
			case GT:
				op = ">";
				break;
			case GTE:
				op = ">=";
				break;
			case LT:
				op = "<";
				break;
			case LTE:
				op = "<=";
				break;
			case LIKE:
				op = "like";
				break;
			default:
				continue;
		}
		where += " and " + req.conditions[i].field + " " + op + " '" + TC_Mysql::escapeString(req.conditions[i].val) + "'";
	}
	pItem->mQuery["whereCond"] = where;

	TLOGDEBUG("QueryImp::query uid:" << req.uid << ", where: " << where << endl);

	string sumField;
	for(size_t i = 0; i < req.indexs.size(); i++)
	{
		sumField += " sum(" + req.indexs[i] + ")";
		if(i != req.indexs.size() - 1)
		{
			sumField += ", ";
		}
	}
	if(!sumField.empty()) {
		pItem->mQuery["sumField"] = sumField;
	}

	TLOGDEBUG("QueryImp::query uid:" << req.uid << ", sumField: " << sumField << endl);

	string groupCond;
	for(size_t i = 0; i < req.groupby.size(); i++)
	{
		groupCond += req.groupby[i];
		if(i != req.groupby.size() - 1)
		{
			groupCond += ", ";
		}
	}
	if(!groupCond.empty()) {
		pItem->mQuery["groupField"] = groupCond;
		pItem->mQuery["groupCond"]  = " group by " + groupCond;
	}

	TLOGDEBUG("QueryImp::query uid:" << req.uid << ", groupCond: " << groupCond << endl);

//	pItem->mQuery   = decoder.getSql();
//
//	map<string,string> &mSqlPart    = decoder.getSql();
//
//	map<string,string>::iterator it;
//	for(it=mSqlPart.begin();it!=mSqlPart.end();it++)
//	{
//		TLOGDEBUG("QueryImp::mysql "<<it->first<<"|"<<it->second<<endl);
//	}
//	string sGroupField                = mSqlPart["groupField"];
//	vector<string> vGroupField        = TC_Common::sepstr<string>(sGroupField, ", ");

//	pItem->bFlag = true;
	g_app.getThreadPoolQueryDb()->put(pItem);

	return 0;

}

//int QueryImp::doRequest(tars::TarsCurrentPtr current, vector<char>& response)
//{
//    const vector<char>& request = current->getRequestBuffer();
//    string buf((const char*)(&request[0]), request.size());
//    string sUid = TC_Common::tostr(g_app.genUid()) + "|";
//
//    FDLOG("inout") << "QueryImp::doRequest sUid:" << sUid << "doRequest input:" <<  buf << endl;
//    TLOGDEBUG(sUid << "QueryImp::doRequest sUid:" << sUid << "doRequest input:" <<  buf << endl);
//
//    doQuery(sUid, buf, false, current);
//
//    return 0;
//}
//
//int QueryImp::doQuery(const string sUid, const string &sIn, bool bTarsProtocol, tars::TarsCurrentPtr current)
//{
//    try
//    {
//        size_t pos = sIn.rfind("}"); // find json end
//
//        string s("");
//
//        if (pos != string::npos)
//        {
//            s = sIn.substr(0, pos+1);
//        }
//        else
//        {
//            throw TC_Exception("bad query string");
//        }
//
//        RequestDecoder decoder(s);
//        TLOGDEBUG("QueryImp::doQuery"<<endl);
//        int ret = decoder.decode();
//
//        decoder.addUid(sUid); //传人uid，供打印使用
//
//        TLOGDEBUG("QueryImp::doQuery " << sUid << "decode json ret:" << ret << "|sqlPart: " << TC_Common::tostr(decoder.getSql()) << endl);
//
//        int64_t tStart    = 0;
//        int64_t tEnd    = 0;
//
//        if(ret == RequestDecoder::TIMECHECK)
//        {
//            string sResult("");
//
//            tStart    = TC_TimeProvider::getInstance()->getNowMs();
//
//            string lasttime = _proxy.getLastTime(decoder.getSql());
//            sResult += "lasttime:" + lasttime + "\n";
//            sResult += "endline\n";
//
//            tEnd = TC_TimeProvider::getInstance()->getNowMs();
//
//            current->sendResponse(sResult.c_str(), sResult.length());
//
//            FDLOG("inout") << "QueryImp::doQuery time_check sUid:" << sUid << "send result succ,size:"<< sResult.length() << "|timecost(ms):" << (tEnd-tStart) << endl;
//            TLOGDEBUG("QueryImp::doQuery time_check sUid:" << sUid << "send result succ,size:"<< sResult.length() << "|timecost(ms):" << (tEnd-tStart) << endl);
//        }
//
//        else if(ret == RequestDecoder::QUERY)
//        {
//            current->setResponse(false);
//            QueryItem * pItem = new QueryItem();
//            pItem->sUid     = sUid;
//            pItem->current  = current;
//            pItem->mQuery   = decoder.getSql();
//
//            map<string,string> &mSqlPart    = decoder.getSql();
//
//            map<string,string>::iterator it;
//            for(it=mSqlPart.begin();it!=mSqlPart.end();it++)
//            {
//                TLOGDEBUG("QueryImp::mysql "<<it->first<<"|"<<it->second<<endl);
//            }
//            string sGroupField                = mSqlPart["groupField"];
//            vector<string> vGroupField        = TC_Common::sepstr<string>(sGroupField, ", ");
//
//            pItem->bFlag = true;
//            g_app.getThreadPoolQueryDb()->put(pItem);
//
//            TLOGDEBUG("QueryImp::doQuery all dbcount." << endl);
//        }
//        else
//        {
//            TLOGERROR("QueryImp::doQuery " << sUid << "decode request failed\n" <<endl);
//
//            string sResult = "Ret:-1\ndecode request failed\n";
//            sResult += "endline\n";
//
//            current->sendResponse(sResult.c_str(), sResult.length());
//
//            FDLOG("inout") << "QueryImp::doQuery failed sUid:" << sUid << endl;
//        }
//    }
//    catch(exception &ex)
//    {
//        TLOGERROR("QueryImp::doQuery exception:" << ex.what() << endl);
//        string sResult = "Ret:-1\n" +  string(ex.what()) + "\nendline\n";
//        current->sendResponse(sResult.c_str(), sResult.length());
//
//        FDLOG("inout") << "QueryImp::doQuery exception sUid:" << sUid << endl;
//    }
//    return 0;
//}
