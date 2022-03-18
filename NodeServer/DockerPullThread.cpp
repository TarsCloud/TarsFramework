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
	g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, "check docker pull " + server->getServerDescriptor().baseImage);

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

void DockerPullThread::loadDockerRegistries()
{
	try
	{
		vector<DockerRegistry> dockerRegistries;

		RegistryPrx registryPrx = AdminProxy::getInstance()->getRegistryProxy();

		registryPrx->getDockerRegistry(dockerRegistries);

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

			TLOG_DEBUG("docker registries size:" << _dockerRegistries.size() << ", base image size:" << _baseImages.size() << endl);
		}
	}
	catch(exception &ex)
	{
		TLOG_ERROR(ex.what() << endl);
	}
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
	try
	{
		bool isNeedPull = true;
		bool isSucc = true;
		string sha;
		string result;

		do
		{

			if(server->getServerDescriptor().baseImage.empty())
			{
				isNeedPull = false;
				isSucc = false;
				result = "base image is empty, baseImageId:" + server->getServerDescriptor().baseImageId;
				NODE_LOG(server->getServerId())->error() << FILE_FUN << result << endl;
				g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, result);
				break;
			}

			//check sha
			if (server->getServerDescriptor().sha.empty())
			{
				isNeedPull = true;
				isSucc = true;
				NODE_LOG(server->getServerId())->debug() << FILE_FUN << "sha is empty, need pull base image:" << server->getServerDescriptor().baseImage << endl;
				break;
			}

			TC_Docker docker;
			docker.setDockerUnixLocal(g_app.getDocketSocket());

			isSucc = docker.inspectImage(server->getServerDescriptor().baseImage);

			if(!isSucc)
			{
				isNeedPull = true;
				result = "inspect image:" + server->getServerDescriptor().baseImage + "  error" + docker.getErrMessage();
				NODE_LOG(server->getServerId())->debug() << FILE_FUN << result << endl;
				g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, result);
				break;
			}

			JsonValueObjPtr oPtr = JsonValueObjPtr::dynamicCast(TC_Json::getValue(docker.getResponseMessage()));

			sha = JsonValueStringPtr::dynamicCast(oPtr->value["Id"])->value;

			isNeedPull = (sha != server->getServerDescriptor().sha);

			if (isNeedPull)
			{
				NODE_LOG(server->getServerId())->debug() << FILE_FUN
														 << "check " + server->getServerDescriptor().baseImage +
															": " + result + " != " +
															server->getServerDescriptor().sha
														 << endl;
				result = server->getServerDescriptor().baseImage + " sha changed, need pull: " + server->getServerDescriptor().sha;

			}
			else
			{
				NODE_LOG(server->getServerId())->debug() << FILE_FUN
														 << "check " + server->getServerDescriptor().baseImage +
															" " + result + " not change" << endl;
				result = server->getServerDescriptor().baseImage + " sha not changed, no need pull: " + server->getServerDescriptor().sha;

			}

			g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, result);
			break;
		}
		while(true);

		if (isSucc && isNeedPull)
		{
			TC_Docker docker;
			docker.setDockerUnixLocal(g_app.getDocketSocket());
			docker.setRequestTimeout(g_app.getDockerPullTimeout() * 1000);

			DockerRegistry dr = getDockerRegistry(server->getServerDescriptor().baseImageId);
			if (dr.sId.empty())
			{
				isSucc = false;
				result = "docker pull error: registry is empty! baseImageId:" +
						 server->getServerDescriptor().baseImageId;

				NODE_LOG(server->getServerId())->error() << FILE_FUN << result << endl;
				g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, result);
			}

			if (!dr.sUserName.empty())
			{
				docker.setAuthentication(dr.sUserName, dr.sPassword, dr.sRegistry);
			}

			isSucc = docker.pull(server->getServerDescriptor().baseImage);

			NODE_LOG(server->getServerId())->debug() << FILE_FUN << "registry:" << dr.sRegistry
													 << ", user:" << dr.sUserName << ", baseImage:"
													 << server->getServerDescriptor().baseImage << ", out:"
													 << (isSucc ? docker.getResponseMessage()
																: docker.getErrMessage())
													 << endl;

			if (!isSucc)
			{
				result = "docker pull " + server->getServerDescriptor().baseImage + "error: " + docker.getErrMessage();
				g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, result);
			}
		}

		if (isSucc)
		{
			callback(server, true, result);
		}
		else
		{
			callback(server, false, result);
		}
	}
	catch(exception &ex)
	{
		NODE_LOG(server->getServerId())->error() << FILE_FUN << "error:" << ex.what() << endl;
	}

	{
		std::lock_guard<std::mutex> lock(_mutex);

		_pullData.erase(server->getServerId());
	}

}
