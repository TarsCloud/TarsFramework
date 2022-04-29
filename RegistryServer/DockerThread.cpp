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

#include "servant/Application.h"

#include "DockerThread.h"
#include "util/tc_docker.h"

extern TC_Config * g_pconf;

DockerThread::DockerThread()
: _terminate(false)
{
}

DockerThread::~DockerThread()
{
    if (isAlive())
    {
        terminate();
        notifyAll();
        getThreadControl().join();
    }
}

int DockerThread::init()
{
    TLOG_DEBUG("DockerThread init"<<endl);

	//检查docker仓库超时
	_checkDockerRegistry  = TC_Common::strto<int>((*g_pconf).get("/tars/reap<checkDockerRegistry>", "120"));

	_dockerSocket		=	(*g_pconf).get("/tars/container<socket>", "/var/run/docker.sock");
	_dockerTimeout		=	TC_Common::strto<int>((*g_pconf).get("/tars/container<timeout>", "300"));

	//加载仓库信息以及基础镜像sha
	loadDockerRegistry();

	TLOG_DEBUG("DockerThread init ok."<<endl);
    return 0;
}

void DockerThread::terminate()
{
    TLOG_DEBUG("DockerThread terminate" << endl);
    _terminate = true;
}

map<string, DockerRegistry> DockerThread::getDockerRegistry()
{
	TC_ThreadLock::Lock lock(*this);
	return _dockerRegistries;
}

pair<string, string> DockerThread::getBaseImageById(const string &baseImageId)
{
	{
		TC_ThreadLock::Lock lock(*this);
		auto it = _baseImages.find(baseImageId);
		if(it != _baseImages.end())
		{
			return it->second;
		}
	}

	loadDockerRegistry();

	{
		TC_ThreadLock::Lock lock(*this);
		auto it = _baseImages.find(baseImageId);
		if(it != _baseImages.end())
		{
			return it->second;
		}
	}

	TLOGEX_DEBUG("DockerThread", "getBaseImageById baseImageId:" << baseImageId << " image is empty" << endl);

	return make_pair("", "");
}

void DockerThread::loadDockerRegistry()
{
	map<string, DockerRegistry> rii;

	CDbHandle db;
	//初始化配置db连接
	db.init(g_pconf);
	int ret = db.loadDockerInfo(rii);

	if(ret == 0)
	{
		TC_ThreadLock::Lock lock(*this);
		_dockerRegistries = rii;
		//检查新的仓库是否有修改过
		for (auto dr: _dockerRegistries)
		{
			for (auto e: dr.second.baseImages)
			{
				_baseImages[e.id] = make_pair(e.image, e.sha);
			}
		}
	}
}

string DockerThread::doPull(const DockerRegistry &dockerRegistry, const BaseImageInfo &baseImageInfo)
{
	TC_Docker docker;
	docker.setDockerUnixLocal(_dockerSocket);
	docker.setRequestTimeout(_dockerTimeout*1000);

	if (!dockerRegistry.sUserName.empty())
	{
		docker.setAuthentication(dockerRegistry.sUserName, dockerRegistry.sPassword, dockerRegistry.sRegistry);
	}

	string result = TC_Common::now2str("%Y-%m-%d %H:%M:%S") + " - ";

	CDbHandle db;
	//初始化配置db连接
	db.init(g_pconf);

	TLOGEX_DEBUG("DockerThread",
			"docker pull " << baseImageInfo.image << ", registry :" << dockerRegistry.sRegistry << ", "
						   << dockerRegistry.sUserName << endl);

	if (!docker.pull(baseImageInfo.image))
	{
		result = docker.getErrMessage();
		TLOGEX_ERROR("DockerThread",
				"docker pull " << baseImageInfo.image << ", registry :" << dockerRegistry.sRegistry << ", "
							   << dockerRegistry.sUserName << ", error:" << result
							   << endl);

		TARS_NOTIFY_ERROR(result);
	}
	else
	{
		if (docker.inspectImage(baseImageInfo.image))
		{
			JsonValueObjPtr oPtr = JsonValueObjPtr::dynamicCast(
					TC_Json::getValue(docker.getResponseMessage()));

			JsonValueStringPtr sPtr = JsonValueStringPtr::dynamicCast(oPtr->value["Id"]);

			TLOGEX_DEBUG("DockerThread",
					"inspect image:" << baseImageInfo.image << ", sha:" << sPtr->value << endl);

			{
				TC_ThreadLock::Lock lock(*this);
				_baseImages[baseImageInfo.id] = make_pair(baseImageInfo.image, sPtr->value);
			}

			db.updateBaseImageSha(baseImageInfo.id, sPtr->value);

			result += "succ";
		}
		else
		{
			result = docker.getErrMessage();
		}
	}

	db.updateBaseImageResult(baseImageInfo.id, result);

	return result;
}

void DockerThread::checkPullDocker()
{
	try
	{
		map<string, DockerRegistry> mDockerRegistry;

		{
			TC_ThreadLock::Lock lock(*this);

			mDockerRegistry = _dockerRegistries;
		}

		TC_Docker docker;
		docker.setDockerUnixLocal(_dockerSocket);
		docker.setRequestTimeout(_dockerTimeout*1000);

		//拉取基础镜像, 获取基础镜像的sha
		for(auto &e : mDockerRegistry)
		{
			for (auto& b: e.second.baseImages)
			{
				bool same = false;

				if (docker.inspectImage(b.image))
				{
					//本地有镜像
					JsonValueObjPtr oPtr = JsonValueObjPtr::dynamicCast(TC_Json::getValue(docker.getResponseMessage()));

					JsonValueStringPtr sPtr = JsonValueStringPtr::dynamicCast(oPtr->value["Id"]);

					same = sPtr->value == b.sha;
				}

				//本地镜像和数据库记录的不一致, 则更新
				//如果数据库没有记录镜像, 则更新
				if (!same || b.sha.empty())
				{
					doPull(e.second, b);
				}
			}
		}
	}
	catch(exception &ex)
	{
		TLOGEX_ERROR("DockerThread", ex.what() << endl);
	}
}

bool DockerThread::findBaseImage(const string & baseImageId, DockerRegistry &dockerRegistry, BaseImageInfo &baseImageInfo)
{
	TC_ThreadLock::Lock lock(*this);

	//拉取基础镜像, 获取基础镜像的sha
	for (auto& e: _dockerRegistries)
	{
		for (auto& b: e.second.baseImages)
		{
			if (b.id == baseImageId)
			{
				dockerRegistry = e.second;
				baseImageInfo = b;

				return true;
			}
		}
	}

	return false;
}

int DockerThread::dockerPull(const string & baseImageId)
{
	DockerRegistry dockerRegistry;
	BaseImageInfo baseImageInfo;

	bool find = findBaseImage(baseImageId, dockerRegistry, baseImageInfo);

	loadDockerRegistry();

	if(!find)
	{
		find = findBaseImage(baseImageId, dockerRegistry, baseImageInfo);
	}

	if(find)
	{
		std::thread th(&DockerThread::doPull,this, dockerRegistry, baseImageInfo);
		th.detach();
		return 0;
	}

	TLOGEX_ERROR("DockerThread", "docker pull base image id:" << baseImageId << " not exists" << endl);

	return -1;
}

void DockerThread::run()
{
	time_t tLoadRegistryImageTimeout      = 0;

    time_t tNow;
    while(!_terminate)
    {
        try
        {
            tNow = TC_TimeProvider::getInstance()->getNow();

			if (tNow - tLoadRegistryImageTimeout >= _checkDockerRegistry)
			{
				tLoadRegistryImageTimeout = tNow;

				loadDockerRegistry();

				checkPullDocker();
			}

        }
        catch(exception & ex)
        {
            TLOGEX_ERROR("DockerThread", "DockerThread exception:" << ex.what() << endl);
        }
        catch(...)
        {
			TLOGEX_ERROR("DockerThread", "DockerThread unknown exception" << endl);
        }

		TC_ThreadLock::Lock lock(*this);
		timedWait(1000); //ms
    }
}



