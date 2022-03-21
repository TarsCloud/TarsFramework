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

#include "ReapThread.h"
#include "servant/Application.h"
#include "util/tc_docker.h"

extern TC_Config * g_pconf;

ReapThread::ReapThread()
: _terminate(false)
, _loadObjectsInterval1(10)
, _leastChangedTime1(60)
, _loadObjectsInterval2(3600)
, _leastChangedTime2(30*60)
, _registryTimeout(150)
, _recoverProtect(true)
, _recoverProtectRate(30)
, _heartBeatOff(false)
{
}

ReapThread::~ReapThread()
{
    if (isAlive())
    {
        terminate();
        notifyAll();
        getThreadControl().join();
    }
}

int ReapThread::init()
{
    TLOG_DEBUG("begin ReapThread init"<<endl);

    //初始化配置db连接
    _db.init(g_pconf);

    //加载对象列表的时间间隔
    _loadObjectsInterval1 = TC_Common::strto<int>((*g_pconf).get("/tars/reap<loadObjectsInterval1>", "10"));
    //第一阶段加载最近时间更新的记录,默认是60秒
    _leastChangedTime1    = TC_Common::strto<int>((*g_pconf).get("/tars/reap<LeastChangedTime1>", "150"));

    _loadObjectsInterval2 = TC_Common::strto<int>((*g_pconf).get("/tars/reap<loadObjectsInterval2>", "300"));

	//检查docker仓库超时
	_checkDockerRegistry  = TC_Common::strto<int>((*g_pconf).get("/tars/reap<checkDockerRegistry>", "60"));
	
    //主控心跳超时时间
    _registryTimeout      = TC_Common::strto<int>((*g_pconf)["/tars/reap<registryTimeout>"]);

    //是否启用DB恢复保护功能
    _recoverProtect       = (*g_pconf).get("/tars/reap<recoverProtect>", "N") == "N"?false:true;

    //启用DB恢复保护功能状态下极限值
    _recoverProtectRate   = TC_Common::strto<int>((*g_pconf).get("/tars/reap<recoverProtectRate>", "30"));

    //是否关闭更新主控心跳时间,一般需要迁移主控服务是，设置此项为Y
    _heartBeatOff = (*g_pconf).get("/tars/reap<heartbeatoff>", "N") == "Y"?true:false;

    //最小值保护
    _loadObjectsInterval1  = _loadObjectsInterval1 < 5 ? 5 : _loadObjectsInterval1;

    _registryTimeout       = _registryTimeout      < 5 ? 5 : _registryTimeout;

    _recoverProtectRate    = _recoverProtectRate   < 1 ? 30: _recoverProtectRate;

	_dockerSocket		=	(*g_pconf).get("/tars/container<socket>", "/var/run/docker.sock");
	_dockerTimeout		=	TC_Common::strto<int>((*g_pconf).get("/tars/container<timeout>", "300"));

    //加载对象列表
    _db.loadObjectIdCache(_recoverProtect, _recoverProtectRate,0,true, true);

	//加载仓库信息以及基础镜像sha
	loadDockerRegistry(false);

    TLOG_DEBUG("ReapThread init ok."<<endl);

    return 0;
}

void ReapThread::terminate()
{
    TLOG_DEBUG("[ReapThread terminate.]" << endl);
    _terminate = true;
}

map<string, DockerRegistry> ReapThread::getDockerRegistry()
{
	TC_ThreadLock::Lock lock(*this);
	return _dockerRegistries;
}

pair<string, string> ReapThread::getBaseImageById(const string &baseImageId)
{
	{
		TC_ThreadLock::Lock lock(*this);
		auto it = _baseImages.find(baseImageId);
		if(it != _baseImages.end())
		{
			return it->second;
		}
	}
	//不存在, 则加载一次
	loadDockerRegistry(false);
	{
		TC_ThreadLock::Lock lock(*this);
		auto it = _baseImages.find(baseImageId);
		if(it != _baseImages.end())
		{
			return it->second;
		}
	}

	TLOGEX_DEBUG("ReapThread", "getBaseImageById baseImageId:" << baseImageId << " image is empty" << endl);

	return make_pair("", "");
}

void ReapThread::loadDockerRegistry(bool pull)
{
	map<string, DockerRegistry> rii;

	int ret = _db.loadDockerInfo(rii);

	if(ret == 0)
	{
		checkDockerRegistries(rii, pull);
	}
}

void ReapThread::checkDockerRegistries(const map<string, DockerRegistry> &dockerRegistries, bool pull)
{
	try
	{
		map<string, DockerRegistry> mDockerRegistry;

		{
			//先历史的保存起来
			TC_ThreadLock::Lock lock(*this);

			mDockerRegistry = _dockerRegistries;
		}

		TLOGEX_DEBUG("ReapThread", "old docker registry size:" << mDockerRegistry.size() << ", new docker registry:" << dockerRegistries.size() << endl);

		//检查新的仓库是否有修改过
		for (auto dr: dockerRegistries)
		{
			for(auto e : dr.second.baseImages)
			{
				TC_ThreadLock::Lock lock(*this);
				_baseImages[e.id] = make_pair(e.image, "");
			}

			auto it = mDockerRegistry.find((dr.second.sId));

			if (it != mDockerRegistry.end())
			{
				if (it->second.sRegistry != dr.second.sRegistry || it->second.sUserName != dr.second.sUserName ||
					it->second.sPassword != dr.second.sPassword)
				{
					dr.second.bSucc = false;

					mDockerRegistry[dr.second.sId] = dr.second;
				}
			}
			else
			{
				dr.second.bSucc = false;
				mDockerRegistry[dr.second.sId] = dr.second;
			}
		}

		{
			TC_ThreadLock::Lock lock(*this);
			TLOGEX_DEBUG("ReapThread", "base image size: " << _baseImages.size() << endl);
		}

		TC_Docker docker;

		//修改过的需要重新登录
		for (auto& e: mDockerRegistry)
		{
			if (!e.second.bSucc)
			{
				if (!e.second.sUserName.empty())
				{
					bool succ = docker.login(e.second.sUserName, e.second.sPassword, e.second.sRegistry);

					if(succ)
					{
						TLOGEX_DEBUG("ReapThread",
								"login registry:" << e.second.sRegistry << ", user:" << e.second.sUserName << ", result:" << docker.getResponseMessage() << endl);
					}else
					{
						TLOGEX_ERROR("ReapThread",
								"login registry:" << e.second.sRegistry << ", user:" << e.second.sUserName << ", error:"
												  << docker.getErrMessage() << endl);
					}

					if(succ)
					{
						mDockerRegistry[e.first].bSucc = true;
					}
					else
					{
						TARS_NOTIFY_ERROR(docker.getErrMessage());
					}
				}
				else
				{
					e.second.bSucc = true;
				}
			}

			{
				TC_ThreadLock::Lock lock(*this);

				_dockerRegistries = mDockerRegistry;
			}

			TC_Docker docker;
			docker.setDockerUnixLocal(_dockerSocket);
			docker.setRequestTimeout(_dockerTimeout*1000);

			//拉取基础镜像, 获取基础镜像的sha
			for(auto e : mDockerRegistry)
			{
				//先加载本地sha
				for(auto &b : e.second.baseImages)
				{
					if(docker.inspectImage(b.image))
					{
						JsonValueObjPtr  oPtr = JsonValueObjPtr::dynamicCast(TC_Json::getValue( docker.getResponseMessage()));

						JsonValueStringPtr sPtr = JsonValueStringPtr::dynamicCast(oPtr->value["Id"]);

						TLOGEX_DEBUG("ReapThread", "inspect image:" << b.image << ", sha:" << sPtr->value << endl);

						{
							TC_ThreadLock::Lock lock(*this);
							_baseImages[b.id] = make_pair(b.image, sPtr->value);
						}
					}
					else
					{
						TLOGEX_ERROR("ReapThread", "inspect error:" << docker.getErrMessage() << endl);

						TARS_NOTIFY_ERROR(docker.getErrMessage());
					}
				}

				if(!pull)
				{
					continue;
				}

				//如果如果仓库没有登录成功, 直接跳过, 否则pull镜像, 更新sha
				if(!e.second.bSucc)
				{
					TLOGEX_DEBUG("ReapThread", "docker registry:" << e.second.sRegistry << ", " << e.second.sUserName << " not login." << endl);

					continue;
				}

				for(auto &b : e.second.baseImages)
				{
					if(!e.second.sUserName.empty())
					{
						docker.setAuthentication(e.second.sUserName, e.second.sPassword, e.second.sRegistry);
					}

					if(!docker.pull(b.image))
					{
						TLOGEX_ERROR("ReapThread", "docker pull " << b.image << ", registry :" << e.second.sRegistry << ", " << e.second.sUserName << ", error:" << docker.getErrMessage() << endl);

						TARS_NOTIFY_ERROR(docker.getErrMessage());
					}
				}
			}
		}
	}
	catch(exception &ex)
	{
		TLOGEX_ERROR("ReapThread", ex.what() << endl);
	}
}


void ReapThread::run()
{
    //增量加载服务分两个阶段
    //第一阶段加载时间
    time_t tLastLoadObjectsStep1 = TC_TimeProvider::getInstance()->getNow();

    //全量加载时间
    time_t tLastLoadObjectsStep2 = TC_TimeProvider::getInstance()->getNow();

    time_t tLastQueryServer      = 0;
	time_t tLoadRegistryImageTimeout      = 0;
	time_t tNow;
    while(!_terminate)
    {
        try
		{
			tNow = TC_TimeProvider::getInstance()->getNow();

			//加载对象列表
			if (tNow - tLastLoadObjectsStep1 >= _loadObjectsInterval1)
			{
				tLastLoadObjectsStep1 = tNow;

				_db.updateRegistryInfo2Db(_heartBeatOff);

				if (tNow - tLastLoadObjectsStep2 >= _loadObjectsInterval2)
				{
					tLastLoadObjectsStep2 = tNow;
					//全量加载,_leastChangedTime2参数没有意义
					_db.loadObjectIdCache(_recoverProtect, _recoverProtectRate, _leastChangedTime2, true, false);
				}
				else
				{
					_db.loadObjectIdCache(_recoverProtect, _recoverProtectRate, _leastChangedTime1, false, false);
				}
			}

			//轮询心跳超时的主控
			if (tNow - tLastQueryServer >= _registryTimeout)
			{
				tLastQueryServer = tNow;
				_db.checkRegistryTimeout(_registryTimeout);
			}

			if (tNow - tLoadRegistryImageTimeout >= _checkDockerRegistry)
			{
				tLoadRegistryImageTimeout = tNow;

				loadDockerRegistry(true);
			}

            TC_ThreadLock::Lock lock(*this);
            timedWait(100); //ms
        }
        catch(exception & ex)
        {
			TLOGEX_ERROR("ReapThread", "exception:" << ex.what() << endl);
        }
        catch(...)
        {
			TLOGEX_ERROR("ReapThread", "unknown exception:" << endl);
        }
    }
}



