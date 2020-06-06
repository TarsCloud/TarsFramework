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

#include "LoadDbThread.h"
#include "NotifyServer.h"
#include "NotifyF.h"

using namespace tars;

LoadDbThread::LoadDbThread()
: _interval(30)
, _terminate(false)
{

}

LoadDbThread::~LoadDbThread()
{
    if(isAlive())
    {
        terminate();
        getThreadControl().join();
    }
}

void LoadDbThread::terminate()
{
    _terminate = true;

    TC_ThreadLock::Lock lock(*this);

    notifyAll();
}

void LoadDbThread::init()
{
    try
    {
	    _retainHistory = TC_Common::strto<int>(g_pconf->get("/tars<retainHistory>", "100"));

        map<string, string> mDbConf = g_pconf->getDomainMap("/tars/db");
        assert(!mDbConf.empty());

        TC_DBConf tcDBConf;
        tcDBConf.loadFromMap(mDbConf);

        _mysql.init(tcDBConf);


	    // TLOGDEBUG("eps size:" << eps.size() << endl);

	    TLOGDEBUG("LoadDbThread::init init mysql conf succ." << endl);
    }
    catch(exception &ex)
    {
        TLOGERROR("LoadDbThread::init ex:" << ex.what() << endl);
        FDLOG("EX") << "LoadDbThread::init ex:" << ex.what() << endl;
        exit(0);
    }
}

bool LoadDbThread::isFirst()
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

void LoadDbThread::run()
{
    size_t iLastTime = 0;

    while (!_terminate)
    {
        size_t iNow = TNOW;
        if(iNow - iLastTime >= _interval)
        {
            iLastTime = iNow;

            loadData();
        }

        string now = TC_Common::now2str("%H");
        if(now == "03") {

//	    deleteNotifys();
        }

        {
            TC_ThreadLock::Lock lock(*this);
            timedWait(1000);
        }
    }
}

void LoadDbThread::deleteNotifys()
{
	try {
		string sql = "select application,server_name,node_name from t_server_conf";

		auto mysqlData = _mysql.queryRecord(sql);

		vector<pair<string, string>> conditions;

		for (size_t i = 0; i < mysqlData.size(); i++) {

			string serverId = mysqlData[i]["application"] + "." + mysqlData[i]["server_name"] + "." + mysqlData[i]["node_name"];

			sql = "select notifytime from t_server_notifys where server_id = '"
				+ _mysql.escapeString(serverId) + "' order by notifytime desc limit "
				+ TC_Common::tostr(_retainHistory) + ", 1";

			auto data = _mysql.queryRecord(sql);

			if (data.size() != 0) {
				conditions.push_back(std::make_pair(mysqlData[i]["server_id"], data[0]["notifytime"]));
			}

			if (i == (mysqlData.size() - 1) || conditions.size() >= 500) {
				TLOGDEBUG("LoadDbThread::deleteNotifys conditions size:" << conditions.size() << endl);

				sql = "delete from t_server_notifys where ";

				for (size_t i = 0; i < conditions.size(); i++) {
					sql += " ( server_id = '" + _mysql.escapeString(conditions[i].first) + "' and notifytime < '"
						+ conditions[i].second + "') ";
					if (i < conditions.size() - 1) {
						sql += " or ";
					}
				}

				_mysql.execute(sql);

				conditions.clear();
			}
		}
	}
	catch(exception &ex)
	{
		TLOGERROR("LoadDbThread::deleteNotifys ex:" << ex.what() << endl);
	}
}

void LoadDbThread::loadData()
{
    try
    {
        TC_Mysql::MysqlData mysqlData;
        map<string, string> &mTemp = _data.getWriter();
        mTemp.clear();
        size_t iOffset(0);

        do
        {
            string sSql("select application, set_name, set_area, set_group, server_name, node_name from t_server_conf limit 1000 offset ");
            sSql = sSql + TC_Common::tostr<size_t>(iOffset) +";";

            mysqlData = _mysql.queryRecord(sSql);

            for (size_t i = 0; i < mysqlData.size(); i++)
            {
                string sValue = mysqlData[i]["set_name"] +"."+ mysqlData[i]["set_area"]+ "." + mysqlData[i]["set_group"];
                if (mysqlData[i]["set_name"].empty())
                {
                    continue;
                }

                string sKey = mysqlData[i]["application"] + "." +  mysqlData[i]["server_name"] + mysqlData[i]["node_name"];
                mTemp.insert(map<string, string>::value_type(sKey, sValue));
            }

            iOffset += mysqlData.size();

        } while (iOffset % 1000 == 0 || mysqlData.data().empty());

        _data.swap();

        TLOGDEBUG("LoadDbThread::loadData load data finish, _mSetApp size:" << mTemp.size() << endl);

    }
    catch (exception &ex)
    {
        TLOGERROR("LoadDbThread::loadData exception:" << ex.what() << endl);
        FDLOG("EX") << "LoadDbThread::loadData exception:" << ex.what() << endl;
    }
}
