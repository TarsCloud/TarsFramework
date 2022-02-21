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
#include "util.h"
#include "NodeServer.h"

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

void DockerPullThread::doPull(ServerObjectPtr server, std::function<void(ServerObjectPtr server, bool, string&)> callback)
{
	bool succ = false;
	string out;
	if(!server->getServerDescriptor().sha.empty())
	{
		//检查sha是否更新
		string command;

		command = "docker images --format '{{.Digest}}' | grep " + server->getServerDescriptor().sha;

		out = TC_Common::trim(TC_Port::exec(command.c_str()));

		succ = out == server->getServerDescriptor().sha;

		if(succ)
		{
			NODE_LOG(server->getServerId())->debug() << FILE_FUN << "check " + server->getServerDescriptor().baseImage + " " + out + " not change" << endl;
			g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, server->getServerDescriptor().baseImage  + " sha:" + out + " not change.");
		}
		else
		{
			NODE_LOG(server->getServerId())->debug() << FILE_FUN << "check " + server->getServerDescriptor().baseImage + " " + out + " != " + server->getServerDescriptor().sha << endl;
		}
	}

	if(!succ)
	{
		//镜像没有拉取下来
		string command = "docker pull " + server->getServerDescriptor().baseImage + " 2>&1";

		NODE_LOG(server->getServerId())->debug() << command << endl;

		out = TC_Common::trim(TC_Port::exec(command.c_str()));

		NODE_LOG(server->getServerId())->debug() << FILE_FUN << "out:" << out << endl;

		g_app.reportServer(server->getServerId(), "", server->getNodeInfo().nodeName, out);

		succ = out.find("Status: ") != string::npos;
	}

	if(succ)
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
