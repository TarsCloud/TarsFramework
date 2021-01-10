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

#include "NodeServer.h"
#include "NodeImp.h"
#include "ServerImp.h"
#include "RegistryProxy.h"
#include "NodeRollLogger.h"
#include "util/tc_md5.h"

string NodeServer::g_sNodeIp;
string NodeServer::NODE_ID = "";
string NodeServer::CONFIG  = "";

BatchPatch *g_BatchPatchThread;
RemoveLogManager *g_RemoveLogThread;

void NodeServer::initialize()
{
    //滚动日志也打印毫秒
    LocalRollLogger::getInstance()->logger()->modFlag(TC_DayLogger::HAS_MTIME);

    //node使用的循环日志初始化
    RollLoggerManager::getInstance()->setLogInfo(ServerConfig::Application,
            ServerConfig::ServerName, ServerConfig::LogPath,
            ServerConfig::LogSize, ServerConfig::LogNum, _communicator,
            ServerConfig::Log);

    initRegistryObj();

 //   addConfig("tarsnode.conf");

    //增加对象
    string sNodeObj     = ServerConfig::Application + "." + ServerConfig::ServerName + ".NodeObj";
    string sServerObj   = ServerConfig::Application + "." + ServerConfig::ServerName + ".ServerObj";

    addServant<NodeImp>(sNodeObj);
    addServant<ServerImp>(sServerObj);

    TLOGDEBUG("NodeServer::initialize ServerAdapter " << (getAdapterEndpoint("ServerAdapter")).toString() << endl);
    TLOGDEBUG("NodeServer::initialize NodeAdapter "   << (getAdapterEndpoint("NodeAdapter")).toString() << endl);

    g_sNodeIp = getAdapterEndpoint("NodeAdapter").getHost();

    if (!ServerFactory::getInstance()->loadConfig())
    {
        TLOGERROR("NodeServer::initialize ServerFactory loadConfig failure" << endl);
    }

    //查看服务desc
    TARS_ADD_ADMIN_CMD_PREFIX("tars.serverdesc", NodeServer::cmdViewServerDesc);
    TARS_ADD_ADMIN_CMD_PREFIX("reloadconfig", NodeServer::cmdReLoadConfig);

    initHashMap();

    //启动KeepAliveThread
    _keepAliveThread   = new KeepAliveThread();
    _keepAliveThread->start();

    TLOGDEBUG("NodeServer::initialize |KeepAliveThread start" << endl);

    _reportMemThread = new ReportMemThread();
    _reportMemThread->start();

    TLOGDEBUG("NodeServer::initialize |_reportMemThread start" << endl);

    //启动批量发布线程
    PlatformInfo plat;
    int iThreads        = TC_Common::strto<int>(g_pconf->get("/tars/node<bpthreads>", "10"));

    _batchPatchThread  = new BatchPatch();
    _batchPatchThread->setPath(plat.getDownLoadDir());
    _batchPatchThread->start(iThreads);

    g_BatchPatchThread  = _batchPatchThread;

    TLOGDEBUG("NodeServer::initialize |BatchPatchThread start(" << iThreads << ")" << endl);

    _removeLogThread = new RemoveLogManager();
    _removeLogThread->start(iThreads);

    g_RemoveLogThread = _removeLogThread;

    TLOGDEBUG("NodeServer::initialize |RemoveLogThread start(" << iThreads << ")" << endl);
}

void NodeServer::initRegistryObj()
{
    string sLocator =    Application::getCommunicator()->getProperty("locator");
    vector<string> vtLocator = TC_Common::sepstr<string>(sLocator, "@");
    TLOGDEBUG("locator:" << sLocator << endl);
    if (vtLocator.size() == 0)
    {
        TLOGERROR("NodeServer::initRegistryObj failed to parse locator" << endl);
        exit(1);
    }

    vector<string> vObj = TC_Common::sepstr<string>(vtLocator[0], ".");
    if (vObj.size() != 3)
    {
        TLOGERROR("NodeServer::initRegistryObj failed to parse locator" << endl);
        exit(1);
    }

    //获取主控名字的前缀
    string sObjPrefix("");
    string::size_type pos = vObj[2].find("QueryObj");
    if (pos != string::npos)
    {
        sObjPrefix = vObj[2].substr(0, pos);
    }

    AdminProxy::getInstance()->setRegistryObjName("tars.tarsregistry." + sObjPrefix + "RegistryObj",
                                                  "tars.tarsregistry." + sObjPrefix + "QueryObj");

    TLOGDEBUG("NodeServer::initRegistryObj RegistryObj:" << ("tars.tarsregistry." + sObjPrefix + "RegistryObj") << endl);
    TLOGDEBUG("NodeServer::initRegistryObj QueryObj:" << ("tars.tarsregistry." + sObjPrefix + "QueryObj") << endl);
}

void NodeServer::initHashMap()
{
    TLOGDEBUG("NodeServer::initHashMap " << endl);

    string sFile        = ServerConfig::DataPath + FILE_SEP + g_pconf->get("/tars/node/hashmap<file>", "__tarsnode_servers");
    string sPath        = TC_File::extractFilePath(sFile);
    int iMinBlock       = TC_Common::strto<int>(g_pconf->get("/tars/node/hashmap<minBlock>", "500"));
    int iMaxBlock       = TC_Common::strto<int>(g_pconf->get("/tars/node/hashmap<maxBlock>", "500"));
    float iFactor       = TC_Common::strto<float>(g_pconf->get("/tars/node/hashmap<factor>", "1"));
    int iSize           = TC_Common::toSize(g_pconf->get("/tars/node/hashmap<size>"), 1024 * 1024 * 10);

    if (!TC_File::makeDirRecursive(sPath))
    {
        TLOGDEBUG("NodeServer::initHashMap cannot create hashmap file " << sPath << endl);
        exit(0);
    }

    try
    {
        g_serverInfoHashmap.initDataBlockSize(iMinBlock, iMaxBlock, iFactor);
        g_serverInfoHashmap.initStore(sFile.c_str(), iSize);

        TLOGDEBUG("NodeServer::initHashMap init hash map succ" << endl);
    }
    catch (TC_HashMap_Exception& e)
    {
        TC_File::removeFile(sFile, false);
        runtime_error(e.what());
    }
}

TC_Endpoint NodeServer::getAdapterEndpoint(const string& name) const
{
    TLOGINFO("NodeServer::getAdapterEndpoint:" << name << endl);

    TC_EpollServerPtr pEpollServerPtr = getEpollServer();
    assert(pEpollServerPtr);

    TC_EpollServer::BindAdapterPtr pBindAdapterPtr = pEpollServerPtr->getBindAdapter(name);
    assert(pBindAdapterPtr);

	TC_Endpoint ep = pBindAdapterPtr->getEndpoint();

	TLOGINFO("NodeServer::getAdapterEndpoint:" << ep.toString() << endl);

	return ep;
//    return pBindAdapterPtr->getEndpoint();
}

bool NodeServer::cmdViewServerDesc(const string& command, const string& params, string& result)
{
    TLOGINFO("NodeServer::cmdViewServerDesc" << command << " " << params << endl);

    vector<string> v = TC_Common::sepstr<string>(params, ".");
    if (v.size() != 2)
    {
        result = "invalid params:" + params;
        return false;
    }

    string application  = v[0];
    string serverName   = v[1];

    ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer(application, serverName);
    if (pServerObjectPtr)
    {
        ostringstream os;
        pServerObjectPtr->getServerDescriptor().display(os);
        result = os.str();
        return false;
    }

    result = "server " + params + " not exist";

    return true;
}

bool NodeServer::cmdReLoadConfig(const string& command, const string& params, string& result)
{
    TLOGDEBUG("NodeServer::cmdReLoadConfig " << endl);

//    bool bRet = false;
/*
    if (addConfig("tafnode.conf"))
    {

        bRet = ServerFactory::getInstance()->loadConfig();

    }

    string s = bRet ? "OK" : "failure";
*/
    result = "cmdReLoadConfig not support!";
    return false;
}

void NodeServer::destroyApp()
{
    if (_keepAliveThread)
    {
        delete _keepAliveThread;
        _keepAliveThread = NULL;
    }

    if (_reportMemThread)
    {
        delete _reportMemThread;
        _reportMemThread = NULL;
    }

    if (_batchPatchThread)
    {
        delete _batchPatchThread;
        _batchPatchThread = NULL;
    }

    if (_removeLogThread)
    {
        delete _removeLogThread;
        _removeLogThread = NULL;
    }

  //  TLOGDEBUG("NodeServer::destroyApp "<< pthread_self() << endl);
}

string tostr(const set<string>& setStr)
{
    string str = "{";
    set<string>::iterator it = setStr.begin();
    int i = 0;
    for (; it != setStr.end(); it++, i++)
    {
        if (i > 0)
        {
            str += "," + *it;
        }
        else
        {
            str += *it;
        }
    }
    str += "}";

    return str;
}

string NodeServer::host2Ip(const string& host)
{
    struct in_addr stSinAddr;
    TC_Socket::parseAddr(host, stSinAddr);

    char ip[INET_ADDRSTRLEN] = "\0";
    inet_ntop(AF_INET, &stSinAddr, ip, INET_ADDRSTRLEN);

    return ip;
}

bool NodeServer::isValid(const string& ip)
{
    static time_t g_tTime = 0;
    static set<string> g_ipSet;

    time_t tNow = TNOW;

    // TLOGDEBUG("NodeServer::isValid ip:" << ip << " -> dst:" << dst << endl);

    static  TC_ThreadLock g_tMutex;

    TC_ThreadLock::Lock  lock(g_tMutex);
    if (tNow - g_tTime > 60)
    {
        string objs = g_pconf->get("/tars/node<cmd_white_list>", "tars.tarsregistry.AdminRegObj:tars.tarsAdminRegistry.AdminRegObj");
        string ips  = g_pconf->get("/tars/node<cmd_white_list_ip>", "");

        if(!ips.empty())
        {
            ips += ":";
        }
        ips += string("127.0.0.1:") + host2Ip(ServerConfig::LocalIp);

        TLOGDEBUG("NodeServer::isValid objs:" << objs << "|ips:" << ips << endl);

        vector<string> vObj = TC_Common::sepstr<string>(objs, ":");

        vector<string> vIp = TC_Common::sepstr<string>(ips, ":");
        for (size_t i = 0; i < vIp.size(); i++)
        {
            g_ipSet.insert(vIp[i]);
            LOG->debug() << ips << ", g_ipSet insert ip:" << vIp[i] << endl;
        }

        map<string, string> context;
        //获取实际ip, 穿透代理, 给TarsCloud云使用
        context["TARS_REAL"] = "true";

        QueryFPrx queryPrx = AdminProxy::getInstance()->getQueryProxy();

        for (size_t i = 0; i < vObj.size(); i++)
        {
            set<string> tempSet;
            string obj = vObj[i];
            try
            {

                vector<EndpointF> vActiveEp, vInactiveEp;
                queryPrx->findObjectById4All(obj, vActiveEp, vInactiveEp, context);
                // queryPrx->tars_endpointsAll(vActiveEp, vInactiveEp);

                for (unsigned i = 0; i < vActiveEp.size(); i++)
                {
                    tempSet.insert(host2Ip(vActiveEp[i].host));
                }

                for (unsigned i = 0; i < vInactiveEp.size(); i++)
                {
                    tempSet.insert(host2Ip(vInactiveEp[i].host));
                }

                TLOGDEBUG("NodeServer::isValid "<< obj << "|tempSet.size():" << tempSet.size() << "|" << tostr(tempSet) << endl);
            }
            catch (exception& e)
            {
                TLOGERROR("NodeServer::isValid catch error: " << e.what() << endl);
            }
            catch (...)
            {
                TLOGERROR("NodeServer::isValid catch error: " << endl);
            }

            if (tempSet.size() > 0)
            {
                g_ipSet.insert(tempSet.begin(), tempSet.end());
            }
        }

        TLOGDEBUG("NodeServer::isValid g_ipSet.size():" << g_ipSet.size() << "|" << tostr(g_ipSet) << endl);
        g_tTime = tNow;
    }

    if (g_ipSet.count(ip) > 0)
    {
        return true;
    }

    if (g_sNodeIp == ip || ServerConfig::LocalIp == ip)
    {
        return true;
    }

    return false;
}

void NodeServer::reportServer(const string& sServerId, const string &sSet, const string &sNodeName, const string& sResult)
{
    try
    {
        //上报到notify
        NotifyPrx pNotifyPrx = Application::getCommunicator()->stringToProxy<NotifyPrx>(ServerConfig::Notify);

        if (pNotifyPrx && sResult != "")
        {
            ReportInfo ri;
            ri.eType = REPORT;
            ri.sApp = sServerId;
            ri.sServer = sServerId;

            vector<string> vModule = TC_Common::sepstr<string>(sServerId, ".");
            if (vModule.size() >= 2)
            {
                ri.sApp = vModule[0];
                ri.sServer = vModule[1];
            }
            ri.sSet = sSet;

            ri.sMessage = sResult;
            ri.eLevel   = NOTIFYERROR;
            ri.sNodeName = sNodeName;

            pNotifyPrx->async_reportNotifyInfo(NULL, ri);
        }
    }
    catch (exception& ex)
    {
        TLOGERROR("NodeServer::reportServer error:" << ex.what() << endl);
    }
}

string NodeServer::getQueryEndpoint()
{
    QueryFPrx prx = CommunicatorFactory::getInstance()->getCommunicator()->stringToProxy<QueryFPrx>("tars.tarsregistry.QueryObj");

    string str;

    vector<EndpointF> activeEp;
    vector<EndpointF> inactiveEp;

    prx->findObjectById4All("tars.tarsregistry.QueryObj", activeEp, inactiveEp);

    if(activeEp.empty())
    {
        return str;
    }
    
    for(size_t i = 0; i < inactiveEp.size(); i++)
    {
        activeEp.push_back(inactiveEp[i]);
    }

    for(size_t i = 0; i < activeEp.size(); i++)
    {
        str += "tcp -h " + activeEp[i].host + " -p " + TC_Common::tostr(activeEp[i].port); 

        if(i != activeEp.size() -1) 
        {
            str += ":";
        }
    }

    if(!str.empty())
    {
        str = "tars.tarsregistry.QueryObj@" + str;
    }

    return str;
}


int NodeServer::onUpdateConfig(const string &nodeId, const string &sConfigFile, bool first)
{
	try
	{
		NODE_ID = nodeId;
		CONFIG  = sConfigFile;

		TC_Config config;
		config.parseFile(CONFIG);

		string sLocator = config.get("/tars/application/client<locator>");
		CommunicatorFactory::getInstance()->getCommunicator()->setProperty("locator", sLocator);
		RegistryPrx pRegistryPrx = CommunicatorFactory::getInstance()->getCommunicator()->stringToProxy<RegistryPrx>(config.get("/tars/node<registryObj>"));

		//if registry is dead, do not block too match time, avoid monitor check tarsnode is dead(first start)
		if(first) {
			pRegistryPrx->tars_set_timeout(500)->tars_ping();
		}

		string sLocalIp;

		if(NODE_ID != "" )
		{
			sLocalIp = NODE_ID;
		}
		else
		{
			try
			{
				int ret = pRegistryPrx->getClientIp(sLocalIp);
				if(ret != 0)
				{
					TLOGERROR("NodeServer::onUpdateConfig cannot get localip from registry"<< endl);
					return -1;
				}
			}
			catch(exception &ex)
			{
				TLOGERROR("NodeServer::onUpdateConfig cannot get localip from registry:" << ex.what() << endl);
				return -1;
			}

			NODE_ID = sLocalIp;
		}

		TLOGDEBUG("NodeServer::onUpdateConfig NODE_ID:" << NODE_ID << endl);

		string sTemplate;
		pRegistryPrx->getNodeTemplate(NODE_ID, sTemplate);
		if(TC_Common::trim(sTemplate) == "" )
		{
			TLOGERROR("NodeServer::onUpdateConfig cannot get node Template from registry, nodeId:" << NODE_ID << endl);
			return -1;
		}

        string nNewLocator = getQueryEndpoint();
        if(!nNewLocator.empty())
        {
            sLocator = nNewLocator;
        }

		sTemplate = TC_Common::replace(sTemplate, "${enableset}", config.get("/tars/application/server<enableset>", "n"));
		sTemplate = TC_Common::replace(sTemplate, "${setdivision}", config.get("/tars/application/server<setdivision>", ""));
		sTemplate = TC_Common::replace(sTemplate, "${locator}", sLocator);

		sTemplate = TC_Common::replace(sTemplate, "${app}", config.get("/tars/application/server<app>", "tars"));
	    sTemplate = TC_Common::replace(sTemplate, "${server}", config.get("/tars/application/server<server>", "tarsnode"));
	    sTemplate = TC_Common::replace(sTemplate, "${basepath}", config.get("/tars/application/server<basepath>", ""));
	    sTemplate = TC_Common::replace(sTemplate, "${datapath}", config.get("/tars/application/server<datapath>", "tars"));
	    sTemplate = TC_Common::replace(sTemplate, "${logpath}", config.get("/tars/application/server<logpath>", "tars"));
        sTemplate = TC_Common::replace(sTemplate, "${localip}",sLocalIp);
		sTemplate = TC_Common::replace(sTemplate, "${local}", config.get("/tars/application/server<local>", ""));
		sTemplate = TC_Common::replace(sTemplate, "${modulename}", "tars.tarsnode");


//		cout << TC_Common::outfill("", '-') << endl;

		TC_Config newConf;

		newConf.parseString(sTemplate);

		config.joinConfig(newConf, true);

		string sConfigPath    = TC_File::extractFilePath(CONFIG);
		if(!TC_File::makeDirRecursive( sConfigPath ))
		{
			TLOGERROR("NodeServer::onUpdateConfig cannot create dir:" << sConfigPath << endl);
			return -1;
		}
		string sFileTemp    = sConfigPath + FILE_SEP + "config.conf.tmp";
		ofstream configfile(sFileTemp.c_str());
		if(!configfile.good())
		{
			TLOGERROR("NodeServer::onUpdateConfig can not create config:" << sFileTemp << endl);
			return -1;
		}

		configfile << TC_Common::replace(config.tostr(),"\\s"," ") << endl;

		configfile.close();

        // 如果新template config 和 现在的template config一样， 那么就不写文件更新
        if ((TC_MD5::md5file(sFileTemp) == TC_MD5::md5file(CONFIG)))
        {
            TLOGTARS("NodeServer::onUpdateConfig new-template-file = now-template-file, do not update it." << endl);
            return 0;
        }

		string sFileBak = CONFIG + ".bak";

		if(TC_File::isFileExist(CONFIG))
		{
			TC_File::copyFile(CONFIG, sFileBak,true);
		}

        // 备份失败， 也不更新
        if ((TC_MD5::md5file(sFileBak) != TC_MD5::md5file(CONFIG)))
        {
            TLOGERROR("NodeServer::onUpdateConfig bak tempalte file error." << endl);
            return -1;
        }

		if(TC_File::isFileExist(sFileTemp) && TC_File::getFileSize(sFileTemp) > 10)
        {
            TC_Config checkConfig;

            //如果解析失败, 直接跑异常, 就不会执行后续的copy file, 避免错误文件覆盖正常文件
            checkConfig.parseFile(sFileTemp);

    		TC_File::renameFile(sFileTemp, CONFIG);
        }
        else if (TC_File::isFileExist(sFileTemp))
        {
            TLOGERROR("NodeServer::onUpdateConfig update template config fail!" << endl);
            TC_File::removeFile(sFileTemp,false);
        }

        TLOGDEBUG("NodeServer::onUpdateConfig update tempalte config succ." << endl);
	}
	catch(exception &e)
	{
		TLOGERROR("NodeServer::onUpdateConfig error:" << e.what() << endl);
		return -1;
	}

	return 0;
}