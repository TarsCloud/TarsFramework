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

#include "KeepAliveThread.h"
#include "RegistryProxy.h"
#include "util/tc_timeprovider.h"
#include "util.h"

KeepAliveThread::KeepAliveThread()
: _terminate(false)
, _registryPrx(NULL)
{
    _runTime                = time(0);
    _nodeInfo               = _platformInfo.getNodeInfo();
    _heartTimeout           = TC_Common::strto<int>(g_pconf->get("/tars/node/keepalive<heartTimeout>", "10"));
    _monitorInterval        = TC_Common::strto<int>(g_pconf->get("/tars/node/keepalive<monitorInterval>", "2"));    
    _reportAliveInterval    = TC_Common::strto<int>(g_pconf->get("/tars/node/keepalive<reportAliveInterval>", "10"));

    _monitorIntervalMs      = TC_Common::strto<int>(g_pconf->get("/tars/node/keepalive<monitorIntervalMs>", "10"));

    _synInterval            = TC_Common::strto<int>(g_pconf->get("/tars/node/keepalive<synStatInterval>", "60"));
    _synStatBatch           = g_pconf->get("/tars/node/keepalive<synStatBatch>", "Y");
    _monitorInterval        = _monitorInterval > 10 ? 10 : (_monitorInterval < 1 ? 1 : _monitorInterval);

    _latestKeepAliveTime    = TNOW;
}

KeepAliveThread::~KeepAliveThread()
{
    terminate();
}

void KeepAliveThread::terminate()
{
    _terminate = true;

    if (isAlive())
    {
	    TC_ThreadLock::Lock lock(_lock);

	    _lock.notifyAll();
    }

    getThreadControl().join();
}

bool KeepAliveThread::timedWait(int millsecond)
{
    TC_ThreadLock::Lock lock(_lock);

    if (_terminate)
    {
        return true;
    }

    return _lock.timedWait(millsecond);
}

FrameworkKey KeepAliveThread::getFrameworkKey()
{
	TC_ThreadLock::Lock lock(_lock);

	return _fKey;
}

void KeepAliveThread::loadFrameworkKey()
{
	try {

		FrameworkKey fKey;
		_registryPrx->getFrameworkKey(fKey);

		if(!fKey.priKey.empty())
		{
			TC_ThreadLock::Lock lock(_lock);

			_fKey = fKey;
		}
	}
	catch (exception &ex)
	{
		NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "catch exception:" << ex.what() << endl;
	}
}
void KeepAliveThread::run()
{
    bool bLoaded        = false;
    bool bRegistered    = false;

    int64_t updateConfigTime = TNOW;

    _registryPrx = AdminProxy::getInstance()->getRegistryProxy();
    try
    {
        _registryPrx->tars_set_timeout(1000)->tars_ping();
    }
    catch (exception &ex)
    {
        NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "catch exception:" << ex.what() << endl;
        TC_Common::sleep(1);
    }

    while (!_terminate)
    {
        try
        {
			if(TNOW - _dockerLastUpdateTime >= 10)
			{
				_dockerLastUpdateTime = TNOW;

				g_app.getDockerPullThread()->loadDockerRegistries();

				loadFrameworkKey();
			}

        	if(TNOW - updateConfigTime > 60 )
	        {
		        updateConfigTime = TNOW;
        		NodeServer::onUpdateConfig(NodeServer::NODE_ID, NodeServer::CONFIG);
	        }

            //注册node信息
            if (_registryPrx && bRegistered == false && !_nodeInfo.nodeName.empty())
            {
                bRegistered = registerNode();
            }
            else
            {
                _nodeInfo = _platformInfo.getNodeInfo();
            }

            //加载服务
            if (bLoaded == false)
            {
                bLoaded = loadAllServers();
            }

            //检查服务的limit配置是否需要更新
            if (bLoaded)
            {
                ServerFactory::getInstance()->setAllServerResourceLimit();
            }

            //检查服务
            checkAlive();

            // 上报node状态
            if (reportAlive() != 0)
            {
                bRegistered = false; //registry服务重启  需要重新注册
            }
        }
        catch (exception& e)
        {
            NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "catch exception|" << e.what() << endl;
        }
        catch (...)
        {
            NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "catch unkown exception|" << endl;
        }

        _latestKeepAliveTime = TNOW;

        timedWait(_monitorInterval * 1000);
    }
}

bool KeepAliveThread::registerNode()
{
    NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "registerNode begin===============|node name|" << _nodeInfo.nodeName << endl;
    try
    {
        if(!_nodeInfo.nodeName.empty())
        {
            int iRet = _registryPrx->tars_set_timeout(1000)->registerNode(_nodeInfo.nodeName, _nodeInfo, _platformInfo.getLoadInfo());

            if (iRet == 0)
            {
                NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "register node succ" << endl;
                return true;
            }
        }
    }
    catch (exception& e)
    {
        NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "KeepAliveThread::registerNode catch exception|" << e.what() << endl;
    }

    NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "register node fail" << endl;

    return false;
}

bool KeepAliveThread::loadAllServers()
{
    NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "load server begin, node name: " << _nodeInfo.nodeName << endl;

    /**
     * 由于加载失败或者node下没有部署服务，这里就会一直去访问主控
     * 增加这个限制，如果超过5次失败，则不去加载，10分钟重试10次
     * 如果之后有新服务部署，会自动添加到node缓存中，不影响监控流程
     */
    static int iFailNum = 0;
    if (iFailNum >= 5)
    {
        static time_t tLastLoadTime = 0;
        if ((TNOW - tLastLoadTime) < 600)
        {
            NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "load server fail" << endl;
            return false;
        }
        tLastLoadTime = TNOW;
        iFailNum = 0;
    }

    try
    {
        if (ServerFactory::getInstance()->loadServer())
        {
            NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "load server succ" << endl;
            return true;
        }
    }
    catch (exception& e)
    {
        NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "catch exception|" << e.what() << endl;
    }

    iFailNum++;

    NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "load server fail" << endl;

    return false;
}

int KeepAliveThread::reportAlive()
{
    try
    {
        static time_t tReport;
        time_t tNow =  TNOW;
        if (tNow - tReport > _reportAliveInterval)
        {
            tReport = tNow;

            NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "node keep alive time:" << TC_Common::tm2str(TNOW, "%Y-%m-%dT%H:%M:%S") << ", thread id:" << TC_Thread::CURRENT_THREADID() << endl;
            
            int iRet = _registryPrx->keepAlive(_nodeInfo.nodeName, _platformInfo.getLoadInfo());

            return iRet;
        }
    }
    catch (exception& e)
    {
        NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "KeepAliveThread::reportAlive catch exception|" << e.what() << endl;
    }

    return 0;
}

int KeepAliveThread::synStat()
{
    try
    {
        int iRet = -1;

        vector<ServerStateInfo> v;
        _stat.swap(v);

        NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "node syn stat  size|" << v.size() << "|" << _synStatBatch << "|" << TC_Thread::CURRENT_THREADID() << endl;

        if (v.size() > 0)
        {
            iRet = _registryPrx->updateServerBatch(v);
        }

        return iRet;
    }
    catch (exception& e)
    {
        string s = e.what();
        if (s.find("server function mismatch exception") != string::npos)
        {
            _synStatBatch = "N";
        }
        NODE_LOG("KeepAliveThread")->error() << FILE_FUN << "catch exception|" << s << endl;
    }

    NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "fail,set SynStatBatch = " << _synStatBatch << endl;

    return -1;
}

void KeepAliveThread::checkAlive()
{
//    int64_t startMs = TC_TimeProvider::getInstance()->getNowMs();

    static time_t tSyn = 0;
    bool bNeedSynServerState = false;

    time_t tNow =  TNOW;
    if (tNow - tSyn > _synInterval)
    {
        tSyn = tNow;
        bNeedSynServerState = true;
    }

    string sServerId;

    _stat.clear();

    map<string, ServerGroup> mmServerList = ServerFactory::getInstance()->getAllServers();

	// NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << "server list size:" << mmServerList.size() << ", synInterval:" << _synInterval << endl;

	map<string, ServerGroup>::const_iterator it = mmServerList.begin();
    for (; it != mmServerList.end(); it++)
    {
        map<string, ServerObjectPtr>::const_iterator  p = it->second.begin();
        for (; p != it->second.end(); p++)
        {
            try
            {
                sServerId   = it->first + "." + p->first;
                ServerObjectPtr pServerObjectPtr = p->second;
                if (!pServerObjectPtr)
                {
                    NODE_LOG("KeepAliveThread")->debug() << FILE_FUN << sServerId << "|=NULL|" << endl;
                    continue;
                }

                pServerObjectPtr->doMonScript();

                if ((TNOW - _runTime) < (ServantHandle::HEART_BEAT_INTERVAL + 1))
                {
                    //等待心跳包
                    continue;
                }

                //corelimit checking
                pServerObjectPtr->checkCoredumpLimit();

                //(checkServer时，上报Server所占用的内存给主控)
                pServerObjectPtr->checkServer(_heartTimeout);

                //cache 同步server 状态
                if (bNeedSynServerState == true)
                {
                    ServerInfo   tServerInfo;
                    tServerInfo.application = it->first;
                    tServerInfo.serverName  = p->first;
                    ServerDescriptor tServerDescriptor = pServerObjectPtr->getServerDescriptor();
                    tServerDescriptor.settingState     = pServerObjectPtr->isEnabled() == true ? "active" : "inactive";
                    g_serverInfoHashmap.set(tServerInfo, tServerDescriptor);

                    ServerStateInfo tServerStateInfo;
                    tServerStateInfo.serverState    = (pServerObjectPtr->isEnSynState() ? pServerObjectPtr->getState() : Inactive);
                    tServerStateInfo.processId      = pServerObjectPtr->getPid();
                    tServerStateInfo.nodeName       = _nodeInfo.nodeName;
                    tServerStateInfo.application    = it->first;
                    tServerStateInfo.serverName     = p->first;

                    if (TC_Common::lower(_synStatBatch) == "y")
                    {
                        _stat.push_back(tServerStateInfo);
                    }
                    else
                    {
                        pServerObjectPtr->asyncSynState();
                    }
                }
            }
            catch (exception& e)
            {
                NODE_LOG("KeepAliveThread")->error() << FILE_FUN << sServerId << " catch exception|" << e.what() << endl;
            }
        }
    }

    if (bNeedSynServerState)
    {
        synStat();
    }
}
