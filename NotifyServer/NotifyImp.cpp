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

#include "NotifyImp.h"
#include "jmem/jmem_hashmap.h"
#include "NotifyServer.h"

extern TC_Config * g_pconf;

std::string Alarm = "[alarm]";
std::string Error = "[error]";
std::string Warn = "[warn]";
std::string Fail = "[fail]";
std::string Normal = "[normal]";

static std::string getNotifyLevel(const std::string& sNotifyMessage)
{

    if (sNotifyMessage.find(Alarm) != std::string::npos) return Alarm;
    if (sNotifyMessage.find(Error) != std::string::npos) return Error;
    if (sNotifyMessage.find(Warn) != std::string::npos) return Warn;
    if (sNotifyMessage.find(Fail) != std::string::npos) return Error;

    return Normal;
}

static std::string getNotifyLevel(int level)
{
    switch(level)
    {
    case NOTIFYERROR:
        return Error;
    case NOTIFYWARN:
        return Warn;
    default:
    case NOTIFYNORMAL:
        return Normal;
    }

    return Normal;
}

void NotifyImp::loadconf()
{
    TC_DBConf tcDBConf;
    tcDBConf.loadFromMap(g_pconf->getDomainMap("/tars/db"));

    _mysqlConfig.init(tcDBConf);

    map<string, string> m = g_pconf->getDomainMap("/tars/filter");
    for(map<string,string>::iterator it= m.begin();it != m.end();it++)
    {
        vector<string> vFilters = TC_Common::sepstr<string>(it->second,";|");
        _setFilter[it->first].insert(vFilters.begin(),vFilters.end()); 
    }
    
}

void NotifyImp::initialize()
{
    loadconf();
}

bool NotifyImp::IsNeedFilte(const string& sServerName,const string& sResult)
{
    if(_setFilter.find(sServerName) != _setFilter.end())
    {
        set<string>& setFilter = _setFilter.find(sServerName)->second;
        for(set<string>::iterator it = setFilter.begin(); it != setFilter.end();it++)
        {
            if(sResult.find(*it) != string::npos)
            {
                return true;
            }
        }    
    }

    const string sDefault = "default";
    if(_setFilter.find(sDefault) != _setFilter.end())
    {
        set<string>& setFilter = _setFilter.find(sDefault)->second;
        for(set<string>::iterator it = setFilter.begin(); it != setFilter.end();it++)
        {
            if(sResult.find(*it) != string::npos)
            {
                return true;
            }
        }        
    }

    return false;
}

void NotifyImp::reportNotifyInfo(const tars::ReportInfo & info, tars::TarsCurrentPtr current)
{
    string nodeId = info.sNodeName;
    if (nodeId.empty())
    {
        nodeId = current->getIp();
    }
    switch (info.eType)
    {
        case (NOTIFY):
        case (REPORT):
        default:
        {
            TLOGDEBUG(
                    "NotifyImp::reportNotifyInfo reportServer:" << info.sApp + "." + info.sServer << "|sSet:"
                                                                << info.sSet
                                                                << "|sContainer:" << info.sContainer << "|ip:"
                                                                << current->getHostName()
                                                                << "|nodeName:" << info.sNodeName << "|sThreadId:"
                                                                << info.sThreadId << "|sMessage:" << info.sMessage
                                                                << endl);

            if (IsNeedFilte(info.sApp + info.sServer, info.sMessage))
            {
                TLOGWARN(
                        "NotifyImp::reportNotifyInfo reportServer filter:" << info.sApp + "." + info.sServer << "|sSet:"
                                                                           << info.sSet << "|sContainer:"
                                                                           << info.sContainer
                                                                           << "|ip:" << current->getHostName()
                                                                           << "|nodeName:" << info.sNodeName
                                                                           << "|sThreadId:" << info.sThreadId
                                                                           << "|sMessage:" << info.sMessage << "|filted"
                                                                           << endl);

                return;
            }

            string sql;
            TC_Mysql::RECORD_DATA rd;

            rd["application"] = make_pair(TC_Mysql::DB_STR, info.sApp);
            rd["server_name"] = make_pair(TC_Mysql::DB_STR, info.sServer);
            rd["container_name"] = make_pair(TC_Mysql::DB_STR, info.sContainer);
            rd["server_id"] = make_pair(TC_Mysql::DB_STR, info.sApp + "." + info.sServer + "_" + nodeId);
            rd["node_name"] = make_pair(TC_Mysql::DB_STR, nodeId);
            rd["thread_id"] = make_pair(TC_Mysql::DB_STR, info.sThreadId);

            if (!info.sSet.empty())
            {
                vector<string> v = TC_Common::sepstr<string>(info.sSet, ".");
                if (v.size() != 3 || (v.size() == 3 && (v[0] == "*" || v[1] == "*")))
                {
                    TLOGERROR("NotifyImp::reportNotifyInfo bad set name:" << info.sSet << endl);
                } else
                {
                    rd["set_name"] = make_pair(TC_Mysql::DB_STR, v[0]);
                    rd["set_area"] = make_pair(TC_Mysql::DB_STR, v[1]);
                    rd["set_group"] = make_pair(TC_Mysql::DB_STR, v[2]);
                }

            } else
            {
                rd["set_name"] = make_pair(TC_Mysql::DB_STR, "");
                rd["set_area"] = make_pair(TC_Mysql::DB_STR, "");
                rd["set_group"] = make_pair(TC_Mysql::DB_STR, "");
            }

            if (info.eType == REPORT)
            {
                rd["command"] = make_pair(TC_Mysql::DB_STR, getNotifyLevel(info.sMessage));
            } else
            {
                rd["command"] = make_pair(TC_Mysql::DB_STR, getNotifyLevel(info.eLevel));
            }
            rd["result"] = make_pair(TC_Mysql::DB_STR, info.sMessage);
            rd["notifytime"] = make_pair(TC_Mysql::DB_INT, "now()");
            string sTable = "t_server_notifys";
            try
            {
                _mysqlConfig.insertRecord(sTable, rd);
            }
            catch (TC_Mysql_Exception &ex)
            {
                TLOGERROR("NotifyImp::reportNotifyInfo insert2Db exception:" << ex.what() << endl);
            }
            catch (exception &ex)
            {
                TLOGERROR("NotifyImp::reportNotifyInfo insert2Db exception:" << ex.what() << endl);
                string sInfo =
                        string("insert2Db exception") + "|" + ServerConfig::LocalIp + "|" + ServerConfig::Application +
                        "."
                        + ServerConfig::ServerName;
            }

            TLOGERROR("reportNotifyInfo unknown type:" << info.writeToJsonString() << endl);
            break;
        }
    }

    return;
}
