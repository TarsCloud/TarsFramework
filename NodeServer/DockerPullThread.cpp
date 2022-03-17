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

#include "DockerPullThread.h"
#include "util/tc_port.h"
#include "util/tc_docker.h"
#include "util.h"
#include "NodeServer.h"
#include "RegistryProxy.h"

using namespace tars;

DockerPullThread::DockerPullThread()
{
}

DockerPullThread::~DockerPullThread()
{
}

void DockerPullThread::start(int num)
{
	_tpool.init(num);
	_tpool.start();
}

void DockerPullThread::terminate()
{
	_tpool.stop();
}

bool DockerPullThread::isPulling(const ServerObjectPtr &server)
{
	std::lock_guard<std::mutex> lock(_mutex);

	return _pullData.find(server->getServerId()) != _pullData.end();
}

void DockerPullThread::pull(ServerObjectPtr server, std::function<void(ServerObjectPtr server, bool, string&)> callback)
{
	g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, "docker pull " + server->getServerDescriptor().baseImage);

	{
		std::lock_guard<std::mutex> lock(_mutex);

		_pullData[server->getServerId()] = server;
	}

	_tpool.exec(server->getServerId(), std::bind(&DockerPullThread::doPull, this, server, callback));
}

DockerRegistry DockerPullThread::getDockerRegistry(const string &baseImageId)
{
	std::lock_guard<std::mutex> lock(_mutex);

	auto it = _baseImages.find(baseImageId);
	if(it != _baseImages.end())
	{
		auto rit = _dockerRegistries.find(it->second);
		if(rit != _dockerRegistries.end())
		{
			return rit->second;
		}
	}

	return DockerRegistry();
}

vector<string> DockerPullThread::checkDockerRegistries()
{
	vector<string> result;

	try
	{
		vector<DockerRegistry> dockerRegistries;

		RegistryPrx registryPrx = AdminProxy::getInstance()->getRegistryProxy();

		registryPrx->getDockerRegistry(dockerRegistries);

		map<string, DockerRegistry> mDockerRegistries;
		{
			std::lock_guard<std::mutex> lock(_mutex);

			for (auto& e: dockerRegistries)
			{
				_dockerRegistries[e.sId] = e;
			}

			for(auto dr : _dockerRegistries)
			{
				for(auto bi : dr.second.baseImages)
				{
					_baseImages[bi.id] = dr.second.sId;
				}
			}

			mDockerRegistries = _dockerRegistries;
		}

		TLOG_DEBUG("docker registries size:" << mDockerRegistries.size() << endl);

		TC_Docker docker;
		docker.setDockerUnixLocal(g_app.getDocketSocket());

		for (auto& e: mDockerRegistries)
		{
			if(!e.second.sUserName.empty())
			{

				bool succ = docker.login(e.second.sUserName, e.second.sPassword, e.second.sRegistry);

				TLOG_DEBUG("registry:" << e.second.sRegistry << ", user:"
									   << e.second.sUserName << ", out:" << (succ ? docker.getResponseMessage() : docker.getErrMessage()) << endl);

				result.emplace_back(e.second.sRegistry + "(" + e.second.sUserName + ") " + (succ ? docker.getResponseMessage() : docker.getErrMessage()));

			}
			else
			{
				result.push_back("(" + e.second.sRegistry + ") no username, no need login");
			}
		}
	}
	catch(exception &ex)
	{
		TLOG_ERROR(ex.what() << endl);
		result.emplace_back(string(ex.what()));
	}

	return result;
}

void DockerPullThread::doPull(ServerObjectPtr server, std::function<void(ServerObjectPtr server, bool, string&)> callback)
{
	bool notChange = false;
	string out;
	if(!server->getServerDescriptor().sha.empty())
	{
		TC_Docker docker;
		docker.setDockerUnixLocal(g_app.getDocketSocket());

		bool succ = docker.inspectImage(server->getServerDescriptor().baseImage);

		if(succ)
		{
			JsonValueObjPtr oPtr = JsonValueObjPtr::dynamicCast(TC_Json::getValue(docker.getResponseMessage()));

			string sha = JsonValueStringPtr::dynamicCast(oPtr->value["Id"])->value;

			notChange = (out == server->getServerDescriptor().sha);

			if(notChange)
			{
				NODE_LOG(server->getServerId())->debug() << FILE_FUN << "check " + server->getServerDescriptor().baseImage + " " + out + " not change" << endl;
				g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, server->getServerDescriptor().baseImage  + " sha:" + out + " not change.");
			}
			else
			{
				NODE_LOG(server->getServerId())->debug() << FILE_FUN << "check " + server->getServerDescriptor().baseImage + " " + out + " != " + server->getServerDescriptor().sha << endl;
			}
		}
		else
		{
			NODE_LOG(server->getServerId())->debug() << "inspect image: " << server->getServerDescriptor().baseImage << ", error:" << docker.getErrMessage() << endl;
			TARS_NOTIFY_ERROR(docker.getErrMessage());
		}
	}

	if(!notChange)
	{
		TC_Docker docker;
		docker.setDockerUnixLocal(g_app.getDocketSocket());

		DockerRegistry dr = getDockerRegistry(server->getServerDescriptor().baseImageId);

		if(!dr.sUserName.empty())
		{
			docker.setAuthentication(dr.sUserName, dr.sPassword, dr.sRegistry);
		}

		bool succ = docker.pull(server->getServerDescriptor().baseImage);

		NODE_LOG(server->getServerId())->debug() << "registry:" << dr.sRegistry
									<< ", user:" << dr.sUserName << ", baseImage:" << server->getServerDescriptor().baseImage << ", out:" << (succ ? docker.getResponseMessage() : docker.getErrMessage()) << endl;

		g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, (succ ? docker.getResponseMessage() : docker.getErrMessage()));
	}

	if(notChange)
	{
		callback(server, true, out);
	}
	else
	{
		callback(server, false, out);
	}

	{
		std::lock_guard<std::mutex> lock(_mutex);

		_pullData.erase(server->getServerId());
	}

}
