﻿/**
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

#include "DbProxy.h"
#include "AdminRegistryServer.h"
#include "util/tc_parsepara.h"
#include "ExecuteTask.h"
#include "util/tc_timeprovider.h"

TC_ThreadLock DbProxy::_mutex;


//vector<map<string, string>> DbProxy::_serverGroupRule;

//key-ip, value-组编号
//map<string, int> DbProxy::_serverGroupCache;

vector<tars::TC_Mysql*> DbProxy::_mysqlReg;
vector<TC_ThreadMutex*> DbProxy::_mysqlLocks;
static std::atomic<int>	g_index(0);

//保留历史发布包
int DbProxy::_patchHistory = 200;

#define MYSQL_LOCK	size_t nowIndex = ++g_index; TC_LockT<TC_ThreadMutex> Lock(*(_mysqlLocks[nowIndex % _mysqlLocks.size()]));
#define MYSQL_INDEX	_mysqlReg[nowIndex % _mysqlReg.size()]
int DbProxy::init(TC_Config *pconf)
{
    try
    {
		int connNums = TC_Common::strto<int>(pconf->get("/tars/db<conn_num>", "5"));
		connNums = (connNums < 1 ? 5 : connNums);
		connNums = (connNums > 100 ? 5 : connNums);
	    TLOG_DEBUG("connNums=" << connNums << endl);

        TC_DBConf tcDBConf;
        tcDBConf.loadFromMap(pconf->getDomainMap("/tars/db"));
	    TLOG_DEBUG("mysql param:" << TC_Common::tostr(pconf->getDomainMap("/tars/db")) << endl);

		for (int i = 0; i < connNums; ++i)
		{
			TC_Mysql* pMysql = new TC_Mysql(tcDBConf);
			_mysqlReg.push_back(pMysql);
			_mysqlLocks.push_back(new TC_ThreadMutex());
		}
	    TLOG_DEBUG("init finish, mysql num:" << _mysqlReg.size() << ", _mysqllock num:" << _mysqlLocks.size() << endl);
		assert((_mysqlLocks.size() == _mysqlReg.size()) && (_mysqlReg.size() > 0));

        _patchHistory = TC_Common::strto<int>(pconf->get("/tars/patch<patchHistory>", "200"));
        if(_patchHistory < 10)
        {
	        _patchHistory = 10;
        }

    }
    catch (TC_Config_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
    }

    return 0;
}

int DbProxy::addTaskReq(const TaskReq &taskReq)
{
    try
    {
	    for (size_t i = 0; i < taskReq.taskItemReq.size(); i++)
	    {
	    	//不是发布, 不需要记录
	    	if(taskReq.taskItemReq[i].command != "patch_tars" && taskReq.taskItemReq[i].command != "grace_patch_tars")
	    		return 0;
	    }

        for (size_t i = 0; i < taskReq.taskItemReq.size(); i++)
        {
            map<string, pair<TC_Mysql::FT, string> > data;
            data["task_no"]     = make_pair(TC_Mysql::DB_STR, taskReq.taskNo);
            data["item_no"]     = make_pair(TC_Mysql::DB_STR, taskReq.taskItemReq[i].itemNo);
            data["application"] = make_pair(TC_Mysql::DB_STR, taskReq.taskItemReq[i].application);
            data["server_name"] = make_pair(TC_Mysql::DB_STR, taskReq.taskItemReq[i].serverName);
            data["node_name"]   = make_pair(TC_Mysql::DB_STR, taskReq.taskItemReq[i].nodeName);
            data["command"]     = make_pair(TC_Mysql::DB_STR, taskReq.taskItemReq[i].command);
            data["parameters"]  = make_pair(TC_Mysql::DB_STR, TC_Parsepara(taskReq.taskItemReq[i].parameters).tostr());

			MYSQL_LOCK;
			MYSQL_INDEX->insertRecord("t_task_item", data);
        }

        {
            map<string, pair<TC_Mysql::FT, string> > data;
            data["task_no"]     = make_pair(TC_Mysql::DB_STR, taskReq.taskNo);
            data["serial"]      = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(taskReq.serial));
            data["create_time"] = make_pair(TC_Mysql::DB_INT, "now()");
            data["user_name"]   = make_pair(TC_Mysql::DB_STR, taskReq.userName);

			MYSQL_LOCK;
			MYSQL_INDEX->insertRecord("t_task", data);
        }
    }
    catch (exception &ex)
    {
        TLOG_ERROR("DbProxy::addTaskReq exception: " << ex.what() << endl);
        return -1;
    }

    return 0;
}

int DbProxy::getTaskRsp(const string &taskNo, TaskRsp &taskRsp)
{

    try
    {
		TC_Mysql::MysqlData item;
		{
			MYSQL_LOCK;
        	string sSql = "select * from t_task as t1, t_task_item as t2 where t1.task_no=t2.task_no and t2.task_no='"
            + MYSQL_INDEX->escapeString(taskNo) + "'";

			item = MYSQL_INDEX->queryRecord(sSql);
		}

        if (item.size() == 0)
        {
            TLOG_ERROR("DbProxy::getTaskRsp 't_task' not task: " << taskNo << endl);
            return -1;
        }

        taskRsp.taskNo = item[0]["task_no"];
        taskRsp.serial = TC_Common::strto<int>(item[0]["serial"]);
        taskRsp.userName = item[0]["user_name"];
        taskRsp.createTime = item[0]["create_time"];


        for (unsigned i = 0; i < item.size(); i++) 
        {
            TaskItemRsp rsp;
            rsp.startTime  = item[i]["start_time"];
            rsp.endTime    = item[i]["end_time"];
            rsp.status     = (EMTaskItemStatus)TC_Common::strto<int>(item[i]["status"]);
            rsp.statusInfo = etos(rsp.status);

            rsp.req.taskNo       = item[i]["task_no"];
            rsp.req.itemNo       = item[i]["item_no"];
            rsp.req.application  = item[i]["application"];
            rsp.req.serverName   = item[i]["server_name"];
            rsp.req.nodeName     = item[i]["node_name"];
            rsp.req.setName      = item[i]["set_name"];
            rsp.req.command      = item[i]["command"];
            rsp.req.parameters   = TC_Parsepara(item[i]["parameters"]).toMap();

            taskRsp.taskItemRsp.push_back(rsp);
        }
        
        ExecuteTask::getInstance()->checkTaskRspStatus(taskRsp);
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR("DbProxy::getTaskRsp exception"<<ex.what()<<endl);
        return -3;
    }

    return 0;
}

int DbProxy::getTaskHistory(const string & application, const string & serverName, const string & command, vector<TaskRsp> &taskRsp)
{

    try
    {
		TC_Mysql::MysqlData res;

		{
			MYSQL_LOCK;
			string sSql = "select t1.`create_time`, t1.`serial`, t1.`user_name`, t2.* from t_task as t1, t_task_item as t2 where t1.task_no=t2.task_no and t2.application='"
				+ MYSQL_INDEX->escapeString(application) + "' and t2.server_name='"
				+ MYSQL_INDEX->escapeString(serverName) + "' and t2.command='"
				+ MYSQL_INDEX->escapeString(command) + "' order by create_time desc, task_no";
			res = MYSQL_INDEX->queryRecord(sSql);
			TLOG_DEBUG(" size:" << res.size() << ", sql:" << sSql << endl);
		}
        for (unsigned i = 0; i < res.size(); i++)
        {
            string taskNo = res[i]["task_no"];

            //获取Item
            TaskItemRsp itemRsp;
            itemRsp.startTime = res[i]["start_time"];
            itemRsp.endTime   = res[i]["end_time"];
            itemRsp.status    = (EMTaskItemStatus)TC_Common::strto<int>(res[i]["status"]);
            itemRsp.statusInfo= etos(itemRsp.status);

            itemRsp.req.taskNo      = res[i]["task_no"];
            itemRsp.req.itemNo      = res[i]["item_no"];
            itemRsp.req.application = res[i]["application"];
            itemRsp.req.serverName  = res[i]["server_name"];
            itemRsp.req.nodeName    = res[i]["node_name"];
            itemRsp.req.setName     = res[i]["set_name"];
            itemRsp.req.command     = res[i]["command"];
            itemRsp.req.parameters  = TC_Parsepara(res[i]["parameters"]).toMap();

            if (taskRsp.empty() || taskRsp[taskRsp.size() - 1].taskNo != taskNo)
            {
                //新的TaskRsp
                TaskRsp rsp;
                rsp.taskNo   = res[i]["task_no"];
                rsp.serial   = TC_Common::strto<int>(res[i]["serial"]);
                rsp.userName = res[i]["user_name"];

                rsp.taskItemRsp.push_back(itemRsp);

                taskRsp.push_back(rsp);
            }
            else
            {
                taskRsp.back().taskItemRsp.push_back(itemRsp);
            }
        }
    }
    catch (exception &ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
        return -1;
    }

    return 0;
}

int DbProxy::setTaskItemInfo(const string & itemNo, const map<string, string> &info)
{
    string where = " where item_no='" + itemNo + "'";
    try
    {
        map<string, pair<TC_Mysql::FT, string> > data;
        data["item_no"] = make_pair(TC_Mysql::DB_STR, itemNo);
        map<string, string>::const_iterator it = info.find("start_time");
        if (it != info.end())
        {
            data["start_time"] = make_pair(TC_Mysql::DB_STR, it->second); 
        }
        it = info.find("end_time");
        if (it != info.end())
        {
            data["end_time"] = make_pair(TC_Mysql::DB_STR, it->second); 
        }
        it = info.find("status");
        if (it != info.end())
        {
            data["status"] = make_pair(TC_Mysql::DB_INT, it->second); 
        }
        it = info.find("log");
        if (it != info.end())
        {
            data["log"] = make_pair(TC_Mysql::DB_STR, it->second);
        }

		{
			MYSQL_LOCK;
			MYSQL_INDEX->updateRecord("t_task_item", data, where);
		}
    }
    catch (exception &ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
        return -1;
    }

    return 0;
}

int DbProxy::undeploy(const string & application, const string & serverName, const string & nodeName, const string &user, string &log)
{
    string where = "where application='" + application + "' and server_name='" + serverName + "' and node_name='" +nodeName + "'";

    try
    {

		MYSQL_LOCK;
        MYSQL_INDEX->deleteRecord("t_server_conf", where);

		MYSQL_INDEX->deleteRecord("t_adapter_conf", where);

        // 删除掉节点配置信息
        where = "where server_name='" + application + "." + serverName + "' and host='" + nodeName + "'";
        MYSQL_INDEX->deleteRecord("t_config_files", where);
    }
    catch (exception &ex)
    {
        log = ex.what();
        TLOG_ERROR(" exception: " << ex.what() << endl);
        return -1;
    }

    return 0;
}

map<string, string> DbProxy::getActiveNodeList(string& result)
{
    map<string, string> mapNodeList;
    try
    {
        string sSql =
                      "select node_name, node_obj from t_node_info "
                      "where present_state='active'";

		TC_Mysql::MysqlData res;

		{
			MYSQL_LOCK;
			res = MYSQL_INDEX->queryRecord(sSql);
		}
		
        TLOG_DEBUG(" (present_state='active') affected:" << res.size() << endl);
        for (unsigned i = 0; i < res.size(); i++)
        {
            mapNodeList[res[i]["node_name"]] = res[i]["node_obj"];
        }
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR("DbProxy::getActiveNodeList exception: " <<ex.what() << endl);
        return mapNodeList;
    }

    return  mapNodeList;
}

int DbProxy::setPatchInfo(const string& app, const string& serverName, const string& nodeName,
                          const string& version, const string& user)
{
    try
    {
		MYSQL_LOCK;
        string sSql =
                      "update t_server_conf "
                      "set patch_version = '" + MYSQL_INDEX->escapeString(version) + "', "
                      "   patch_user = '" + MYSQL_INDEX->escapeString(user) + "', "
                      "   patch_time = now() "
                      "where application='" + MYSQL_INDEX->escapeString(app) + "' "
                      "    and server_name='" + MYSQL_INDEX->escapeString(serverName) + "' "
                      "    and node_name='" + MYSQL_INDEX->escapeString(nodeName) + "' ";
         
        MYSQL_INDEX->execute(sSql);
        TLOG_DEBUG(" " << app << "." << serverName << "_" << nodeName
                  << " affected:" << MYSQL_INDEX->getAffectedRows() << endl);

        return 0;
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR("DbProxy::setPatchInfo " << app << "." << serverName << "_" << nodeName
                  << " exception: " << ex.what() << endl);
        return -1;
    }
}


int DbProxy::getNodeVersion(const string& nodeName, string& version, string& result)
{
    try
    {
		MYSQL_LOCK;
        string sSql =
                      "select tars_version from t_node_info "
                      "where node_name='" + MYSQL_INDEX->escapeString(nodeName) + "'";

        TC_Mysql::MysqlData res = MYSQL_INDEX->queryRecord(sSql);
        TLOG_DEBUG(" (node_name='" << nodeName << "') affected:" << res.size() << endl);
        if (res.size() > 0)
        {
            version = res[0]["tars_version"];
            return 0;

        }
        result = "node_name(" + nodeName + ") int table t_node_info not exist";
    }
    catch (TC_Mysql_Exception& ex)
    {
        result = string(__FUNCTION__) + " exception: " + ex.what();
        TLOG_ERROR(result << endl);
    }
    return  -1;
}

int DbProxy::updateServerState(const string& app, const string& serverName, const string& nodeName,
                               const string& stateFields, tars::ServerState state, int processId)
{
    try
    {
        int64_t iStart = TC_TimeProvider::getInstance()->getNowMs();
        if (stateFields != "setting_state" && stateFields != "present_state")
        {
            TLOG_DEBUG(app << "." << serverName << "_" << nodeName
                      << " not supported fields:" << stateFields << endl);
            return -1;
        }

        string sProcessIdSql = (stateFields == "present_state" ?
                                (", process_id = " + TC_Common::tostr<int>(processId) + " ") : "");

		MYSQL_LOCK;
        string sSql =
                      "update t_server_conf "
                      "set " + stateFields + " = '" + etos(state) + "' " + sProcessIdSql +
                      "where application='" + MYSQL_INDEX->escapeString(app) + "' "
                      "    and server_name='" + MYSQL_INDEX->escapeString(serverName) + "' "
                      "    and node_name='" + MYSQL_INDEX->escapeString(nodeName) + "' ";

        MYSQL_INDEX->execute(sSql);
        TLOG_DEBUG(" " << app << "." << serverName << "_" << nodeName << ", " << etos(state) << ", processId:" << processId
			<< " affected:" << MYSQL_INDEX->getAffectedRows()
                  << "|cost:" << (TC_TimeProvider::getInstance()->getNowMs() - iStart) << endl);
        return 0;

    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << app << "." << serverName << "_" << nodeName
                  << " exception: " << ex.what() << endl);
        return -1;
    }
}
//
//int DbProxy::gridPatchServer(const string& app, const string& servername, const string& nodename, const string& status)
//{
//    try
//    {
//		MYSQL_LOCK;
//        string sSql("update t_server_conf");
//        sSql += " set grid_flag='";
//        sSql += status;
//        sSql += "' where application='";
//        sSql += MYSQL_INDEX->escapeString(app);
//        sSql += "' and server_name='";
//        sSql += MYSQL_INDEX->escapeString(servername);
//        sSql += "' and node_name='";
//        sSql += MYSQL_INDEX->escapeString(nodename);
//        sSql += "'";
//
//        int64_t iStart = TC_TimeProvider::getInstance()->getNowMs();
//
//        MYSQL_INDEX->execute(sSql);
//
//        TLOG_DEBUG("|app:" << app << "|server:" << servername << "|node:" << nodename
//                  << "|affected:" << MYSQL_INDEX->getAffectedRows() << "|cost:" << (TC_TimeProvider::getInstance()->getNowMs() - iStart) << endl);
//
//        return 0;
//
//    }
//    catch (TC_Mysql_Exception& ex)
//    {
//        TLOG_ERROR("|app:" << app << "|server:" << servername << "|node:" << nodename
//                  << "|exception: " << ex.what() << endl);
//        return -1;
//    }
//}

vector<ServerDescriptor> DbProxy::getServers(const string& app, const string& serverName, const string& nodeName, bool withDnsServer)
{
    string sSql;
    vector<ServerDescriptor>  vServers;
    unsigned num = 0;
    int64_t iStart = TC_TimeProvider::getInstance()->getNowMs();

    try
    {
		TC_Mysql::MysqlData res;
		{
			MYSQL_LOCK;
        //server详细配置
        string sCondition;
			sCondition += "server.node_name='" + MYSQL_INDEX->escapeString(nodeName) + "'";
			if (app != "")        sCondition += " and server.application='" + MYSQL_INDEX->escapeString(app) + "' ";
			if (serverName != "") sCondition += " and server.server_name='" + MYSQL_INDEX->escapeString(serverName) + "' ";
			if (withDnsServer == false) sCondition += " and server.server_type !='tars_dns' "; //不获取dns服务

//        "    allow_ip, max_connections, servant, queuecap, queuetimeout,protocol,handlegroup,shmkey,shmcap,"

			sSql =
               "select server.application, server.server_name, server.node_name, server.run_type, base_path, "
               "    exe_path, setting_state, present_state, adapter_name, thread_num, async_thread_num, endpoint,"
               "    profile,template_name, "
               "    allow_ip, max_connections, servant, queuecap, queuetimeout,protocol,handlegroup,"
               "    patch_version, patch_time, patch_user, "
               "    server_type, start_script_path, stop_script_path, monitor_script_path,config_center_port ,"
               "     enable_set, set_name, set_area, set_group, bak_flag "
               "from t_server_conf as server "
               "    left join t_adapter_conf as adapter using(application, server_name, node_name) "
               "where " + sCondition;

			res = MYSQL_INDEX->queryRecord(sSql);
		}

        num = res.size();
        //对应server在vector的下标
        map<string, int> mapAppServerTemp;

        //获取模版profile内容
        map<string, string> mapProfile;

        //分拆数据到server的信息结构里
        for (unsigned i = 0; i < res.size(); i++)
        {
            string sServerId = res[i]["application"] + "." + res[i]["server_name"]
                               + "_" + res[i]["node_name"];

            if (mapAppServerTemp.find(sServerId) == mapAppServerTemp.end())
            {
                //server配置
                ServerDescriptor server;
                server.application  = res[i]["application"];
                server.serverName   = res[i]["server_name"];
                server.nodeName     = res[i]["node_name"];
                server.basePath     = res[i]["base_path"];
                server.exePath      = res[i]["exe_path"];
                server.settingState = res[i]["setting_state"];
                server.presentState = res[i]["present_state"];
                server.patchVersion = res[i]["patch_version"];
                server.patchTime    = res[i]["patch_time"];
                server.patchUser    = res[i]["patch_user"];
                server.profile      = res[i]["profile"];
                server.serverType   = res[i]["server_type"];
                server.startScript  = res[i]["start_script_path"];
                server.stopScript   = res[i]["stop_script_path"];
                server.monitorScript    = res[i]["monitor_script_path"];
                server.configCenterPort = TC_Common::strto<int>(res[i]["config_center_port"]);
				server.runType      = res[i]["run_type"];
//                server.bakFlag      = TC_Common::strto<int>(res[i]["bak_flag"]);

                server.setId = "";
                if (TC_Common::lower(res[i]["enable_set"]) == "y")
                {
                    server.setId = res[i]["set_name"] + "." +  res[i]["set_area"] + "." + res[i]["set_group"];
                }

                //获取父模版profile内容
                if (mapProfile.find(res[i]["template_name"]) == mapProfile.end())
                {
                    string sResult;
                    mapProfile[res[i]["template_name"]] = getProfileTemplate(res[i]["template_name"], sResult);
                }

                TC_Config tParent, tProfile;
                tParent.parseString(mapProfile[res[i]["template_name"]]);
                tProfile.parseString(server.profile);
                int iDefaultAsyncThreadNum = 3;
                if (server.serverType == "tars_nodejs")
                {
                    iDefaultAsyncThreadNum = 0;
                }
                int iConfigAsyncThreadNum  = TC_Common::strto<int>(TC_Common::trim(res[i]["async_thread_num"]));
                iDefaultAsyncThreadNum     = iConfigAsyncThreadNum > iDefaultAsyncThreadNum ? iConfigAsyncThreadNum : iDefaultAsyncThreadNum;
                server.asyncThreadNum      = TC_Common::strto<int>(tProfile.get("/tars/application/client<asyncthread>", TC_Common::tostr(iDefaultAsyncThreadNum)));
                tParent.joinConfig(tProfile, true);
                server.profile = tParent.tostr();

                mapAppServerTemp[sServerId] = vServers.size();
                vServers.push_back(server);
            }

            //adapter配置
            AdapterDescriptor adapter;
            adapter.adapterName = res[i]["adapter_name"];
            if (adapter.adapterName == "")
            {
                //adapter没配置，left join 后为 NULL,不放到adapters map
                continue;
            }

            adapter.threadNum       = res[i]["thread_num"];
            adapter.endpoint        = res[i]["endpoint"];
            adapter.maxConnections  = TC_Common::strto<int>(res[i]["max_connections"]);
            adapter.allowIp         = res[i]["allow_ip"];
            adapter.servant         = res[i]["servant"];
            adapter.queuecap        = TC_Common::strto<int>(res[i]["queuecap"]);
            adapter.queuetimeout    = TC_Common::strto<int>(res[i]["queuetimeout"]);
            adapter.protocol        = res[i]["protocol"];
            adapter.handlegroup     = res[i]["handlegroup"];

            vServers[mapAppServerTemp[sServerId]].adapters[adapter.adapterName] = adapter;
        }
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << app << "." << serverName << "_" << nodeName
                  << " exception: " << ex.what() << "|" << sSql << endl);
        return vServers;
    }
    catch (TC_Config_Exception& ex)
    {
        TLOG_ERROR(" " << app << "." << serverName << "_" << nodeName
                  << " TC_Config_Exception exception: " << ex.what() << endl);
        throw TarsException(string("TC_Config_Exception exception: ") + ex.what());
    }

    TLOG_DEBUG(" " << app << "." << serverName << "_" << nodeName
              << " getServers affected:" << num
              << "|cost:" << (TC_TimeProvider::getInstance()->getNowMs() - iStart) << endl);

    return  vServers;

}

int DbProxy::getNodeList(const string& app, const string& serverName, vector<string>& nodeNames)
{
    int ret = 0;
    nodeNames.clear();
    try
    {
		MYSQL_LOCK;
        string sSql = "select node_name from t_server_conf where application='" + MYSQL_INDEX->escapeString(app) + "' and server_name='" + MYSQL_INDEX->escapeString(serverName) + "'";

        TC_Mysql::MysqlData res = MYSQL_INDEX->queryRecord(sSql);

        for (unsigned i = 0; i < res.size(); i++)
        {
            nodeNames.push_back(res[i]["node_name"]);
        }
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
        ret = -1;
    }
    return ret;
}


string DbProxy::getProfileTemplate(const string& sTemplateName, string& sResultDesc)
{
    map<string, int> mapRecursion;
    return getProfileTemplate(sTemplateName, mapRecursion, sResultDesc);
}

string DbProxy::getProfileTemplate(const string& sTemplateName, map<string, int>& mapRecursion, string& sResultDesc)
{
    try
    {
		TC_Mysql::MysqlData res;
		{
			MYSQL_LOCK;
			string sSql = "select template_name, parents_name, profile from t_profile_template "
				"where template_name='" + MYSQL_INDEX->escapeString(sTemplateName) + "'";

			res = MYSQL_INDEX->queryRecord(sSql);
		}

        if (res.size() == 0)
        {
            sResultDesc += "(" + sTemplateName + ":template not found)";
            return "";
        }

        TC_Config confMyself, confParents;
        confMyself.parseString(res[0]["profile"]);
        //mapRecursion用于避免重复继承
        mapRecursion[res[0]["template_name"]] = 1;

        if (res[0]["parents_name"] != "" && mapRecursion.find(res[0]["parents_name"]) == mapRecursion.end())
        {
            confParents.parseString(getProfileTemplate(res[0]["parents_name"], mapRecursion, sResultDesc));
            confMyself.joinConfig(confParents, false);
        }
        sResultDesc += "(" + sTemplateName + ":OK)";

        TLOG_DEBUG(" " << sTemplateName << " " << sResultDesc << endl);

        return confMyself.tostr();
    }
    catch (TC_Mysql_Exception& ex)
    {
        sResultDesc += "(" + sTemplateName + ":" + ex.what() + ")";
        TLOG_ERROR(" exception: " << ex.what() << endl);
    }
    catch (TC_Config_Exception& ex)
    {
        sResultDesc += "(" + sTemplateName + ":" + ex.what() + ")";
        TLOG_ERROR(" TC_Config_Exception exception: " << ex.what() << endl);
    }

    return  "";
}

vector<string> DbProxy::getAllApplicationNames(string& result)
{
    vector<string> vApps;
    try
    {
		TC_Mysql::MysqlData res;
		{
			MYSQL_LOCK;
			string sSql = "select distinct application from t_server_conf";

			res = MYSQL_INDEX->queryRecord(sSql);
		}

        TLOG_DEBUG(" affected:" << res.size() << endl);

        for (unsigned i = 0; i < res.size(); i++)
        {
            vApps.push_back(res[i]["application"]);
        }
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
        return vApps;
    }

    return vApps;

}

vector<vector<string> > DbProxy::getAllServerIds(string& result)
{
    vector<vector<string> > vServers;
    try
    {
		TC_Mysql::MysqlData res;
		MYSQL_LOCK;
        string sSql = "select application, server_name, node_name, setting_state, present_state,server_type from t_server_conf";

        res = MYSQL_INDEX->queryRecord(sSql);
        TLOG_DEBUG(" affected:" << res.size() << endl);

        for (unsigned i = 0; i < res.size(); i++)
        {
            vector<string> server;
            server.push_back(res[i]["application"] + "." + res[i]["server_name"] +  "_" + res[i]["node_name"]);
            server.push_back(res[i]["setting_state"]);
            server.push_back(res[i]["present_state"]);
            server.push_back(res[i]["server_type"]);
            vServers.push_back(server);
        }
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
        return vServers;
    }

    return vServers;

}
//
//int DbProxy::getGroupId(const string& ip)
//{
//    bool bFind      = false;
//    int iGroupId    = -1;
//    string sOrder;
//    string sAllowIpRule;
//    string sDennyIpRule;
//    vector<map<string, string> > vServerGroupInfo;
//    try
//    {
//        {
//            TC_ThreadLock::Lock lock(_mutex);
//            map<string, int>::iterator it = _serverGroupCache.find(ip);
//            if (it != _serverGroupCache.end())
//            {
//                return it->second;
//            }
//            vServerGroupInfo = _serverGroupRule;
//        }
//
//        for (unsigned i = 0; i < vServerGroupInfo.size(); i++)
//        {
//            iGroupId                    = TC_Common::strto<int>(vServerGroupInfo[i]["group_id"]);
//            sOrder                      = vServerGroupInfo[i]["ip_order"];
//            sAllowIpRule                = vServerGroupInfo[i]["allow_ip_rule"];
//            sDennyIpRule                = vServerGroupInfo[i]["denny_ip_rule"];
//            vector<string> vAllowIp     = TC_Common::sepstr<string>(sAllowIpRule, ",|;");
//            vector<string> vDennyIp     = TC_Common::sepstr<string>(sDennyIpRule, ",|;");
//            if (sOrder == "allow_denny")
//            {
//                if (TC_Common::matchPeriod(ip, vAllowIp))
//                {
//                    bFind = true;
//                    break;
//                }
//            }
//            else if (sOrder == "denny_allow")
//            {
//                if (TC_Common::matchPeriod(ip, vDennyIp))
//                {
//                    //在不允许的ip列表中则不属于本行所对应组  继续匹配查找
//                    continue;
//                }
//                if (TC_Common::matchPeriod(ip, vAllowIp))
//                {
//                    bFind = true;
//                    break;
//                }
//            }
//        }
//
//        if (bFind == true)
//        {
//            TC_ThreadLock::Lock lock(_mutex);
//            _serverGroupCache[ip] = iGroupId;
//
//            TLOGINFO("get groupId succ|ip|" << ip
//                     << "|group_id|" << iGroupId << "|ip_order|" << sOrder
//                     << "|allow_ip_rule|" << sAllowIpRule
//                     << "|denny_ip_rule|" << sDennyIpRule
//                     << "|ServerGroupCache|" << TC_Common::tostr(_serverGroupCache) << endl);
//
//            return iGroupId;
//        }
//    }
//    catch (TC_Mysql_Exception& ex)
//    {
//        TLOG_ERROR(" exception: " << ex.what() << endl);
//    }
//    catch (exception& ex)
//    {
//        TLOG_ERROR(" " << ex.what() << endl);
//    }
//    return -1;
//}

NodePrx DbProxy::getNodePrx(const string& nodeName)
{
    try
    {
		TC_Mysql::MysqlData res;
		{
			MYSQL_LOCK;
			string sSql = "select node_obj "
                      "from t_node_info "
				"where node_name='" + MYSQL_INDEX->escapeString(nodeName) + "' and present_state='active'";

			res = MYSQL_INDEX->queryRecord(sSql);
		}
        TLOG_DEBUG(" '" << nodeName << "' affected:" << res.size() << endl);

        if (res.size() == 0)
        {
            throw TarsNodeNotRegistryException("node '" + nodeName + "' not registered  or heartbeart timeout,please check for it");
        }

		if(res[0]["node_obj"].empty())
		{
			return NULL;
		}

        NodePrx nodePrx;
        g_app.getCommunicator()->stringToProxy(res[0]["node_obj"], nodePrx);

        return nodePrx;

    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << nodeName << " exception: " << ex.what() << endl);
        throw TarsNodeNotRegistryException(string("get node record from db error:") + ex.what());
    }
    catch (tars::TarsException& ex)
    {
        TLOG_ERROR(" " << nodeName << " exception: " << ex.what() << endl);
        throw ex;
    }

}

int DbProxy::getFramework(vector<tars::FrameworkServer> &servers)
{
	try
	{
		MYSQL_LOCK;
		string sSql = "select * from t_adapter_conf where application = 'tars'";

		TC_Mysql::MysqlData data = MYSQL_INDEX->queryRecord(sSql);
		for(size_t i = 0; i < data.size(); i++)
		{
			FrameworkServer server;
			server.serverName   = data[i]["server_name"];
			server.nodeName     = data[i]["node_name"];
			server.objName      = data[i]["servant"] + "@" + data[i]["endpoint"];

			servers.push_back(server);
		}

		sSql = "select * from t_node_info";
		data = MYSQL_INDEX->queryRecord(sSql);
		for(size_t i = 0; i < data.size(); i++)
		{
			FrameworkServer server;
			server.serverName   = "tarsnode";
			server.nodeName     = data[i]["node_name"];
			server.objName      = data[i]["node_obj"];

			servers.push_back(server);
		}
		return 0;
	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR(" exception: " << ex.what() << endl);
		return -1;
	}
}
//
//int DbProxy::checkRegistryTimeout(unsigned uTimeout)
//{
//    try
//    {
//		MYSQL_LOCK;
//        string sSql = "update t_registry_info "
//                      "set present_state='inactive' "
//                      "where last_heartbeat < date_sub(now(), INTERVAL " + tars::TC_Common::tostr(uTimeout) + " SECOND)";
//
//        MYSQL_INDEX->execute(sSql);
//        TLOG_DEBUG(" (" << uTimeout  << "s) affected:" << MYSQL_INDEX->getAffectedRows() << endl);
//
//        return MYSQL_INDEX->getAffectedRows();
//
//    }
//    catch (TC_Mysql_Exception& ex)
//    {
//        TLOG_ERROR(" exception: " << ex.what() << endl);
//        return -1;
//    }
//
//}
//
//int DbProxy::updateRegistryInfo2Db(bool bRegHeartbeatOff)
//{
//    if (bRegHeartbeatOff)
//    {
//        TLOG_DEBUG("updateRegistryInfo2Db not need to update reigstry status !" << endl);
//        return 0;
//    }
//
//    map<string, string>::iterator iter;
//    map<string, string> mapServantEndpoint = g_app.getServantEndpoint();
//    if (mapServantEndpoint.size() == 0)
//    {
//        TLOG_ERROR("fatal error, get registry servant failed!" << endl);
//        return -1;
//    }
//
//    try
//    {
//		MYSQL_LOCK;
//        string sSql = "replace into t_registry_info (locator_id, servant, endpoint, last_heartbeat, present_state, tars_version)  values ";
//
//        string sVersion = Application::getTarsVersion() + "_" + SERVER_VERSION;
//        for (iter = mapServantEndpoint.begin(); iter != mapServantEndpoint.end(); iter++)
//        {
//            TC_Endpoint locator;
//            locator.parse(iter->second);
//
//            sSql += (iter == mapServantEndpoint.begin() ? string("") : string(", ")) +
//                    "('" + locator.getHost() + ":" + TC_Common::tostr<int>(locator.getPort()) + "', "
//                    "'" + iter->first + "', '" + iter->second + "', now(), 'active', " +
//                    "'" + MYSQL_INDEX->escapeString(sVersion) + "')";
//        }
//
//        MYSQL_INDEX->execute(sSql);
//
//    }
//    catch (TC_Mysql_Exception& ex)
//    {
//        TLOG_ERROR(" exception: " << ex.what() << endl);
//        return -1;
//    }
//    catch (exception& ex)
//    {
//        TLOG_ERROR(" exception: " << ex.what() << endl);
//        return -1;
//    }
//
//    return 0;
//}
//
//int DbProxy::loadIPPhysicalGroupInfo()
//{
//    try
//    {
//		TC_Mysql::MysqlData res;
//		{
//			MYSQL_LOCK;
//			string sSql = "select group_id,ip_order,allow_ip_rule,denny_ip_rule,group_name from t_server_group_rule "
//                      "order by group_id";
//			res = MYSQL_INDEX->queryRecord(sSql);
//		}
//		if( res.size() > 0)
//		{
//			TLOG_DEBUG(" get server group from db, records affected:" << res.size() << endl);
//		}
//
//        TC_ThreadLock::Lock lock(_mutex);
//        _serverGroupRule.clear();
//        _serverGroupRule = res.data();
//
//        _serverGroupCache.clear();  //规则改变 清除以前缓存
//    }
//    catch (TC_Mysql_Exception& ex)
//    {
//        TLOG_ERROR(" exception: " << ex.what() << endl);
//    }
//    catch (exception& ex)
//    {
//        TLOG_ERROR(" " << ex.what() << endl);
//    }
//    return -1;
//}

int DbProxy::getInfoByPatchId(const string &patchId, string &patchFile, string &md5)
{
    try
    {
		MYSQL_LOCK;
        string sSql = "select tgz, md5 from t_server_patchs where id=" + patchId;
        TC_Mysql::MysqlData res = MYSQL_INDEX->queryRecord(sSql);

        if (res.size() == 0)
        {
            TLOG_ERROR(" get patch tgz, md5 from db error, no records! patchId=" << patchId << endl);
            return -1;
        }

        patchFile = res[0]["tgz"];
        md5 = res[0]["md5"];

        return 0;
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
    }
    catch (exception& ex)
    {
        TLOG_ERROR(" " << ex.what() << endl);
    }
    return -1;
}

int DbProxy::updatePatchByPatchId(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ)
{
    try
    {
		MYSQL_LOCK;
        string sql = "update t_server_patchs set publish='1',publish_user='" + user 
            + "',publish_time=now(),lastuser='"+ user +"' where id=" + patchId;

        MYSQL_INDEX->execute(sql);

        return 0;
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
    }
    catch (exception& ex)
    {
        TLOG_ERROR(" " << ex.what() << endl);
    }
    return -1;
}

vector<string> DbProxy::deleteHistorys(const string &application, const string &serverName)
{
    vector<string> tgz;

    try
    {
		MYSQL_LOCK;
        string where = "server = '" + MYSQL_INDEX->escapeString(application + "." + serverName) + "'";
        string sql = "select id from t_server_patchs where " + where + " order by id desc limit " + TC_Common::tostr(_patchHistory) + ", 1";

        auto data = MYSQL_INDEX->queryRecord(sql);

        if(data.size() > 0)
        {
            auto dels = MYSQL_INDEX->queryRecord("select tgz from t_server_patchs where " + where + " and id < " + data[0]["id"]);

            for(size_t i = 0; i < dels.size(); i++)
            {
                tgz.push_back(dels[i]["tgz"]);
            }

            MYSQL_INDEX->execute("delete from t_server_patchs where " + where + " and id < " + data[0]["id"]);
        }
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" exception: " << ex.what() << endl);
    }
    catch (exception& ex)
    {
        TLOG_ERROR(" " << ex.what() << endl);
    }
    return tgz;
}

int DbProxy::updateServerFlowState(const string & app, const string & serverName, const vector<string>& nodeList, bool bActive)
{
    try
    {
        int64_t iStart = TC_TimeProvider::getInstance()->getNowMs();
       
        if (nodeList.size() == 0)
        {
            TLOG_ERROR("updateServerFlowState nodeList is empty!" << endl);
            return -1;
        }
        string sStatus = (bActive ? "active": "inactive");
        string nodeListIn = "(";

        MYSQL_LOCK;
        for (size_t i = 0; i < nodeList.size(); i++)
        {
            if (i != 0)
            {
                nodeListIn += ", '" + MYSQL_INDEX->escapeString(nodeList[i]) + "' ";
            }
            else
            {
                nodeListIn += " '" + MYSQL_INDEX->escapeString(nodeList[i]) + "' ";
            }
        }
        nodeListIn += ")";
        
        string sSql = "update t_server_conf "
                      "set flow_state = '" + sStatus + "' "
                      "where application='" + MYSQL_INDEX->escapeString(app) + "' "
                      "    and server_name='" + MYSQL_INDEX->escapeString(serverName) + "' "
                      "    and node_name in " + nodeListIn;

        MYSQL_INDEX->execute(sSql);
        TLOG_DEBUG(" " << app << "." << serverName << "_" << nodeListIn
			<< " affected:" << MYSQL_INDEX->getAffectedRows()
                  << "|cost:" << (TC_TimeProvider::getInstance()->getNowMs() - iStart) << endl);
        return (int)(MYSQL_INDEX->getAffectedRows());

    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << app << "." << serverName << "_" << TC_Common::tostr(nodeList)
                  << " exception: " << ex.what() << endl);
    }
    return -1;
}

int DbProxy::updateServerFlowStateOne(const string & app, const string & serverName, const string& nodeName, bool bActive)
{
    try
    {
        int64_t iStart = TC_TimeProvider::getInstance()->getNowMs();
       
        string sStatus = (bActive ? "active": "inactive");

        MYSQL_LOCK;
        
        string sSql = "update t_server_conf "
                      "set flow_state = '" + sStatus + "' "
                      "where application='" + MYSQL_INDEX->escapeString(app) + "' "
                      "    and server_name='" + MYSQL_INDEX->escapeString(serverName) + "' "
                      "    and node_name = '" + nodeName + "'";

        MYSQL_INDEX->execute(sSql);
        size_t ar = MYSQL_INDEX->getAffectedRows();
        TLOG_DEBUG(" " << app << "." << serverName << "_" << nodeName
			<< " affected:" << ar
                  << "|cost:" << (TC_TimeProvider::getInstance()->getNowMs() - iStart) << endl);
        return (int)ar;
    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << app << "." << serverName << "_" << nodeName
                  << " exception: " << ex.what() << endl);
    }
    return -1;
}


int DbProxy::uninstallServer(const string &sApplication, const string &sServerName, const string &nodeName, string &result)
{
    try
    {
        MYSQL_LOCK

        string sSql = "update t_server_conf set registry_timestamp='" + TC_Common::tm2str(TNOW) + "' where application='" + MYSQL_INDEX->escapeString(sApplication) + "' and server_name='" + MYSQL_INDEX->escapeString(sServerName) + "'";
        MYSQL_INDEX->execute(sSql);

        //删除tars服务本身的记录
        string sWhere = "where application='" + MYSQL_INDEX->escapeString(sApplication) + "' and server_name='" + MYSQL_INDEX->escapeString(sServerName) + "' and node_name='" + MYSQL_INDEX->escapeString(nodeName) + "'";
        MYSQL_INDEX->deleteRecord("t_adapter_conf", sWhere);
        MYSQL_INDEX->deleteRecord("t_server_conf", sWhere);

        //删除节点配置
        sWhere = "where server_name ='" + MYSQL_INDEX->escapeString(sApplication + "."+sServerName)+ "' and host='"+ MYSQL_INDEX->escapeString(nodeName) + "' and level=3";
        MYSQL_INDEX->deleteRecord("t_config_files", sWhere);

        string sql = "select * from t_server_conf where application='" + MYSQL_INDEX->escapeString(sApplication) + "' and server_name='" + MYSQL_INDEX->escapeString(sServerName) + "'";
        TC_Mysql::MysqlData data = MYSQL_INDEX->queryRecord(sql);
        if (data.size() == 0)
        {
            sWhere = "where server_name ='" + sApplication + "."+sServerName+ "' and level=2";
            MYSQL_INDEX->deleteRecord("t_config_files", sWhere);
        }

        return 0;

    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << sApplication << "." << sServerName << "_" << nodeName
                  << " exception: " << ex.what() << endl);
    }
    return -1;
}

int DbProxy::hasServer(const string &sApplication, const string &sServerName, bool &has)
{
   try
    {
        MYSQL_LOCK

        string sSql = "select id from t_server_conf where application='" + MYSQL_INDEX->escapeString(sApplication) + "' and server_name='" + MYSQL_INDEX->escapeString(sServerName) + "'";
        has = MYSQL_INDEX->existRecord(sSql);

        return 0;

    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << sApplication << "." << sServerName
                  << " exception: " << ex.what() << endl);
    }
    return -1;
}

int DbProxy::insertServerConf(const ServerConf &conf, bool replace)
{
   	try
    {
        map<string, pair<TC_Mysql::FT, string> > m;
        m["application"]    = make_pair(TC_Mysql::DB_STR, conf.application);
        m["server_name"]    = make_pair(TC_Mysql::DB_STR, conf.serverName);
        m["node_name"]      = make_pair(TC_Mysql::DB_STR, conf.nodeName);
        m["exe_path"]       = make_pair(TC_Mysql::DB_STR, conf.exePath);
        m["template_name"]  = make_pair(TC_Mysql::DB_STR, conf.profile);
        m["posttime"]       = make_pair(TC_Mysql::DB_STR, TC_Common::now2str("%Y-%m-%d %H:%M:%S"));
        m["tars_version"]   = make_pair(TC_Mysql::DB_STR, TARS_VERSION);
        m["server_type"]    = make_pair(TC_Mysql::DB_STR, conf.serverType);
		m["base_path"]    = make_pair(TC_Mysql::DB_STR, conf.basePath);
		m["setting_state"]    = make_pair(TC_Mysql::DB_STR, conf.settingState);
		m["monitor_script_path"]    = make_pair(TC_Mysql::DB_STR, conf.monitorScript);
		m["start_script_path"]    = make_pair(TC_Mysql::DB_STR, conf.startScript);
		m["stop_script_path"]    = make_pair(TC_Mysql::DB_STR, conf.stopScript);
		m["enable_group"]    = make_pair(TC_Mysql::DB_STR, conf.enableGroup);
		m["ip_group_name"]    = make_pair(TC_Mysql::DB_STR, conf.ipGroupName);

		MYSQL_LOCK

		if(replace)
		{
			MYSQL_INDEX->replaceRecord("t_server_conf", m);
		}
		else
		{
			MYSQL_INDEX->insertRecord("t_server_conf", m);
		}

        return 0;

    }
    catch (TC_Mysql_Exception& ex)
    {
        TLOG_ERROR(" " << conf.application << "." << conf.serverName
                  << " exception: " << ex.what() << endl);
    }
    return -1;
}

int DbProxy::insertAdapterConf(const string &application, const string &serverName, const string &nodeName, const AdapterConf &conf, bool replace)
{
	try
	{
		map<string, pair<TC_Mysql::FT, string> > m;
		m["application"]        = make_pair(TC_Mysql::DB_STR, application);
		m["server_name"]        = make_pair(TC_Mysql::DB_STR, serverName);
		m["node_name"]          = make_pair(TC_Mysql::DB_STR, nodeName);
		m["adapter_name"]       = make_pair(TC_Mysql::DB_STR, conf.adapterName);
		m["thread_num"]         = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(conf.threadNum));
		m["endpoint"]           = make_pair(TC_Mysql::DB_STR, conf.endpoint);
		m["max_connections"]    = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(conf.maxConnections));
		m["servant"]            = make_pair(TC_Mysql::DB_STR, conf.servant);
		m["queuecap"]           = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(conf.queuecap));
		m["queuetimeout"]       = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(conf.queuetimeout));
		m["posttime"]           = make_pair(TC_Mysql::DB_STR, TC_Common::now2str("%Y-%m-%d %H:%M:%S"));
		m["lastuser"]           = make_pair(TC_Mysql::DB_STR, "");
		m["protocol"]           = make_pair(TC_Mysql::DB_STR, conf.protocol);

		MYSQL_LOCK

		if(replace)
		{
			MYSQL_INDEX->replaceRecord("t_adapter_conf", m);
		}
		else
		{
			MYSQL_INDEX->insertRecord("t_adapter_conf", m);
		}

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR(" " << application << "." << serverName
								<< " exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::insertConfigFile(const string &sFullServerName, const string &fileName, const string &content, const string &sNodeName, int level, bool replace)
{
	try
	{
		map<string, pair<TC_Mysql::FT, string> > m;
		m["server_name"]    = make_pair(TC_Mysql::DB_STR, sFullServerName);
		m["host"]           = make_pair(TC_Mysql::DB_STR, sNodeName);
		m["filename"]       = make_pair(TC_Mysql::DB_STR, fileName);
		m["config"]         = make_pair(TC_Mysql::DB_STR, content);
		m["posttime"]       = make_pair(TC_Mysql::DB_STR, TC_Common::now2str("%Y-%m-%d %H:%M:%S"));
		m["lastuser"]       = make_pair(TC_Mysql::DB_STR, "");
		m["level"]          = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(level));
		m["config_flag"]    = make_pair(TC_Mysql::DB_INT, "0");

		MYSQL_LOCK

		if(replace)
		{
			MYSQL_INDEX->replaceRecord("t_config_files", m);
		}
		else
		{
			MYSQL_INDEX->insertRecord("t_config_files", m);
		}

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR(" " << sFullServerName << "." << fileName
								<< " exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::getConfigFileId(const string &sFullServerName, const string &fileName,  const string &sNodeName, int level, int &configId)
{
	try
	{
		MYSQL_LOCK
		string sSql =
				"select id from t_config_files where server_name = '" + MYSQL_INDEX->escapeString(sFullServerName) +
				"' and filename = '" + MYSQL_INDEX->escapeString(fileName) + "' and host = '" +
				MYSQL_INDEX->escapeString(sNodeName) + "' and level = " + TC_Common::tostr(level);
		TC_Mysql::MysqlData data = MYSQL_INDEX->queryRecord(sSql);
		if (data.size() != 1)
		{
			string errmsg = string("get id from t_config_files error, serverName = ") + sFullServerName;
			TLOG_ERROR(errmsg << endl);
			return -1;
		}

		configId = TC_Common::strto<int>(data[0]["id"]);

		return 0;
	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR(" " << sFullServerName << "." << fileName
								<< " exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::insertHistoryConfigFile(int configId, const string &reason, const string &content, bool replace)
{
	try
	{
		map<string, pair<TC_Mysql::FT, string> > m;
		m["configid"]       = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(configId));
		m["reason"]         = make_pair(TC_Mysql::DB_STR, reason);
		m["reason_select"]  = make_pair(TC_Mysql::DB_STR, "");
		m["content"]        = make_pair(TC_Mysql::DB_STR, content);
		m["posttime"]       = make_pair(TC_Mysql::DB_STR, TC_Common::now2str("%Y-%m-%d %H:%M:%S"));
		m["lastuser"]       = make_pair(TC_Mysql::DB_STR, "");

		MYSQL_LOCK

		if(replace)
		{
			MYSQL_INDEX->replaceRecord("t_config_history_files", m);
		}
		else
		{
			MYSQL_INDEX->insertRecord("t_config_history_files", m);
		}

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR(" " << configId
								<< " exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::registerPlugin(const PluginConf &conf)
{
	try
	{
		map<string, pair<TC_Mysql::FT, string> > m;
		m["f_name"]       = make_pair(TC_Mysql::DB_STR, conf.name);
		m["f_name_en"]    = make_pair(TC_Mysql::DB_STR, conf.name_en);
		m["f_obj"]  	  = make_pair(TC_Mysql::DB_STR, conf.obj);
		m["f_type"]       = make_pair(TC_Mysql::DB_INT, TC_Common::tostr(conf.type));
		m["f_path"]       = make_pair(TC_Mysql::DB_STR, conf.path);
		m["f_extern"]       = make_pair(TC_Mysql::DB_STR, TC_Common::tostr(conf.fextern));

		MYSQL_LOCK

		MYSQL_INDEX->replaceRecord("db_tars_web.t_plugin", m);

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR(" name:" << conf.name
								<< " exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::getAuth(const string &uid, vector<DbProxy::UserFlag> &flags)
{
	try
	{
		string sql;
		TC_Mysql::MysqlData data;
		{
			MYSQL_LOCK
			sql = "select * from db_user_system.t_auth where uid = '" + MYSQL_INDEX->escapeString(uid) + "'";

			data = MYSQL_INDEX->queryRecord(sql);
		}

		for(size_t i = 0; i < data.size(); i++)
		{
			DbProxy::UserFlag flag;

			flag.flag = data[i]["flag"];
			flag.role = data[i]["role"];

			flags.push_back(flag);
		}

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR("uid:" << uid
								<< " exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::getTicket(const string &ticket, string &uid)
{
	try
	{
		TC_Mysql::MysqlData data;
		{
			MYSQL_LOCK

			string sql;
			sql = "select * from db_user_system.t_login_temp_info where ticket = '" +
				  MYSQL_INDEX->escapeString(ticket) + "'";

			data = MYSQL_INDEX->queryRecord(sql);
		}

		if(data.size() >= 1)
		{
			uid = data[0]["uid"];
		}

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR("uid:" << uid
								<< " exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::getServerTree(vector<ServerTree> &tree)
{
	try
	{
		TC_Mysql::MysqlData data;
		{
			MYSQL_LOCK

			string sql;
			sql = "select * from t_server_conf";

			data = MYSQL_INDEX->queryRecord(sql);
		}

		for(size_t i = 0; i < data.size(); i++)
		{
			ServerTree s;
			s.application = data[i]["application"];
			s.server_name = data[i]["server_name"];
			s.enable_set = data[i]["enable_set"];
			s.set_name = data[i]["set_name"];
			s.set_area = data[i]["set_area"];
			s.set_group = data[i]["set_group"];

			tree.push_back(s);
		}

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR("exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::getPatchPackage(const string &application, const string &serverName, int defaultVersion, PatchPackage &pack)
{
	try
	{
		TC_Mysql::MysqlData data;
		{
			MYSQL_LOCK

			string sql;
			sql = "select id, md5 from t_server_patchs where server='" + MYSQL_INDEX->escapeString(application + "." + serverName) + "' and default_version = " + TC_Common::tostr(defaultVersion) + " limit 0, 1";

			data = MYSQL_INDEX->queryRecord(sql);
		}

		if(data.size() > 0)
		{
			pack.id = TC_Common::strto<int>(data[0]["id"]);
			pack.md5= data[0]["md5"];
			return 0;
		}

		return -1;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR("exception: " << ex.what() << endl);
	}
	return -1;
}

int DbProxy::getServerNameList(const vector<ApplicationServerName> &fullServerName, vector<map<string, string>> &serverList)
{
	try
	{
		string where = " where 1=1 and ( (1=2) ";

		TC_Mysql::MysqlData data;
		{
			MYSQL_LOCK
			for(auto fName : fullServerName)
			{
				where += " or (application='" + MYSQL_INDEX->escapeString(fName.application) + "' and server_name='" + MYSQL_INDEX->escapeString(fName.serverName) + "') ";
			}
			where += ")";

			string sql;
			sql = "select * from t_server_conf" + where;

			data = MYSQL_INDEX->queryRecord(sql);
		}

		serverList.swap(data.data());

		return 0;

	}
	catch (TC_Mysql_Exception& ex)
	{
		TLOG_ERROR("exception: " << ex.what() << endl);
	}
	return -1;
}