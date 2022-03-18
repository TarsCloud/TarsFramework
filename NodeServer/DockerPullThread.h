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

#ifndef __DOCKER_PULL_THREAD_H_
#define __DOCKER_PULL_THREAD_H_

#include "servant/Application.h"
#include "ServerObject.h"
#include "util/tc_thread_queue.h"

class DockerPullThread
{
public:
    DockerPullThread();

    ~DockerPullThread();

	/**
	 * 启动
	 * @param num
	 */
	void start(int num);

	/**
	 *
	 */
    void terminate();

    /**
     * 执行发布单个请求
     */
    void pull(ServerObjectPtr server, std::function<void(ServerObjectPtr server, bool, string&)> callback);

	/**
	 * 正在拉取中
	 * @return
	 */
	bool isPulling(const ServerObjectPtr &server);

	/**
	 * 检查docker仓库是改变以及登录
	 */
	vector<string> checkDockerRegistries();

	/**
	 * 加载仓库信息
	 */
	void loadDockerRegistries();

	/**
	 * 获取仓库信息
	 * @param baseImageId
	 * @return
	 */
	DockerRegistry getDockerRegistry(const string &baseImageId);

protected:

	void doPull(ServerObjectPtr server, std::function<void(ServerObjectPtr server, bool, string&)> callback);

protected:

	TC_ThreadPoolHash	_tpool;

	std::mutex		_mutex;

	/**
	 * 仓库地址
	 */
	map<string, DockerRegistry> _dockerRegistries;

	/**
	 * 基础镜像(id, registryId)
	 */
	map<string, string>			_baseImages;

	/**
	 * 正在拉取的镜像
	 */
	unordered_map<string, ServerObjectPtr> _pullData;

};

#endif
