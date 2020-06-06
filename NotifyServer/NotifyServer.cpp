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

#include "NotifyServer.h"
#include "NotifyImp.h"
#include "jmem/jmem_hashmap.h"

TarsHashMap<NotifyKey, NotifyInfo, ThreadLockPolicy, MemStorePolicy> * g_notifyHash;

void NotifyServer::initialize()
{
    //增加对象
    addServant<NotifyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".NotifyObj");

    //初始化hash
    g_notifyHash = new TarsHashMap<NotifyKey, NotifyInfo, ThreadLockPolicy, MemStorePolicy>();

    g_notifyHash->initDataBlockSize(TC_Common::strto<size_t>((*g_pconf)["/tars/hash<min_block>"]),
            TC_Common::strto<size_t>((*g_pconf)["/tars/hash<max_block>"]),
            TC_Common::strto<float>((*g_pconf)["/tars/hash<factor>"]));

    size_t iSize = TC_Common::toSize((*g_pconf)["/tars/hash<file_size>"], 1024 * 1024 * 10);
    if (iSize > 1024 * 1024 * 100)
        iSize = 1024 * 1024 * 100;
    char* data = new char[iSize];
    g_notifyHash->create(data, iSize);

	int retainHistory = TC_Common::strto<int>(g_pconf->get("/tars<retainHistory>", "100"));
	string cron = g_pconf->get("/tars<cron>", "0 0 3 * * *");

	_timer.startTimer(1);

	_timer.postCron(cron, std::bind(&NotifyServer::deleteNotifys, this, retainHistory));

	_loadDbThread = new LoadDbThread();
    _loadDbThread->init();
    _loadDbThread->start();
}

void NotifyServer::destroyApp()
{
    if(_loadDbThread != NULL)
    {
        delete _loadDbThread;
        _loadDbThread = NULL;
    }

    TLOGDEBUG("NotifyServer::destroyApp ok" << endl);
}

void NotifyServer::deleteNotifys(int retainHistory)
{
	if(!isFirst())
	{
		return;
	}

	try {

		map<string, string> mDbConf = g_pconf->getDomainMap("/tars/db");
		assert(!mDbConf.empty());

		TC_DBConf tcDBConf;
		tcDBConf.loadFromMap(mDbConf);

		TC_Mysql mysql;

		mysql.init(tcDBConf);

		string sql = "select application,server_name,node_name from t_server_conf";

		auto mysqlData = mysql.queryRecord(sql);

		vector<pair<string, string>> conditions;

		for (size_t i = 0; i < mysqlData.size(); i++) {

			string serverId = mysqlData[i]["application"] + "." + mysqlData[i]["server_name"] + "." + mysqlData[i]["node_name"];

			sql = "select notifytime from t_server_notifys where server_id = '"
				+ mysql.escapeString(serverId) + "' order by notifytime desc limit "
				+ TC_Common::tostr(retainHistory) + ", 1";

			auto data = mysql.queryRecord(sql);

			if (data.size() != 0) {
				conditions.push_back(std::make_pair(mysqlData[i]["server_id"], data[0]["notifytime"]));
			}

			if (i == (mysqlData.size() - 1) || conditions.size() >= 500) {
				TLOGDEBUG("LoadDbThread::deleteNotifys conditions size:" << conditions.size() << endl);

				sql = "delete from t_server_notifys where ";

				for (size_t i = 0; i < conditions.size(); i++) {
					sql += " ( server_id = '" + mysql.escapeString(conditions[i].first) + "' and notifytime < '"
						+ conditions[i].second + "') ";
					if (i < conditions.size() - 1) {
						sql += " or ";
					}
				}

				mysql.execute(sql);

				conditions.clear();
			}
		}
	}
	catch(exception &ex)
	{
		TLOGERROR("LoadDbThread::deleteNotifys ex:" << ex.what() << endl);
	}
}

bool NotifyServer::isFirst()
{
	NotifyPrx prx = Application::getCommunicator()->stringToProxy<NotifyPrx>(ServerConfig::Application + "." + ServerConfig::ServerName + ".NotifyObj");

	vector<EndpointInfo> eps1;
	vector<EndpointInfo> eps2;
	prx->tars_endpoints(eps1, eps2);

	vector<TC_Endpoint> eps;
	for_each(eps1.begin(), eps1.end(), [&](const EndpointInfo &e) {
		eps.push_back(e.getEndpoint());
	});
	for_each(eps2.begin(), eps2.end(), [&](const EndpointInfo &e) {
		eps.push_back(e.getEndpoint());
	});

	if(eps.empty())
	{
		return false;
	}

	auto adapter = Application::getEpollServer()->getBindAdapter(ServerConfig::Application + "." + ServerConfig::ServerName + ".NotifyObjAdapter");

	return adapter->getEndpoint().getHost() == eps[0].getHost();
}
