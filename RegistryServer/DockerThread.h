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

#ifndef __DOCKER_THREAD_H__
#define __DOCKER_THREAD_H__

#include <iostream>
#include "util/tc_thread.h"
#include "DbHandle.h"

using namespace tars;

//////////////////////////////////////////////////////
/**
 * 监控tarsnode超时的线程类
 */
class DockerThread : public TC_Thread, public TC_ThreadLock
{
public:

    /**
     * 构造函数
     */
    DockerThread();

    /**
     * 析构函数
     */
    ~DockerThread();

    /**
     * 结束线程
     */
    void terminate();

    /**
     * 初始化
     */
    int init();

    /**
     * 轮询函数
     */
    virtual void run();

	/**
	 * 获取docker仓库镜像地址
	 * @return
	 */
	map<string, DockerRegistry> getDockerRegistry();

	/**
	 * 根据基础镜像ID获取基础镜像
	 * @param baseImageId
	 * @return <image, sha>
	 */
	pair<string, string> getBaseImageById(const string &baseImageId);

	/**
	 * 拉取镜像
	 * @param baseImageId
	 * @return
	 */
	int dockerPull(const string & baseImageId);

protected:

	/**
	 * 启动时加载基础镜像数据
	 */
	void loadDockerRegistry();

	/**
	 * 检查是否需要强制pull一次镜像
	 */
	void checkPullDocker();

	/**
	 * 找到对应的基础镜像
	 * @param baseImageId
	 * @param dockerRegistry
	 * @param baseImageInfo
	 * @return
	 */
	bool findBaseImage(const string & baseImageId, DockerRegistry dockerRegistry, BaseImageInfo baseImageInfo);

	/**
	 * 拉取镜像, 并更新sha5
	 * @param dockerRegistry
	 * @param baseImageInfo
	 * @return
	 */
	string doPull(DockerRegistry dockerRegistry, BaseImageInfo baseImageInfo);

protected:
    /*
     * 线程结束标志
     */
    bool         _terminate;

	/*
	 * 加载docker仓库镜像定时(秒)
	 */
	int        _checkDockerRegistry = 120;

	/**
	 * docker socket
	 */
	string 		_dockerSocket;

	/**
	 * pull timeout
	 */
	int 		_dockerTimeout = 300;
	/**
	 * docker的镜像仓库
	 */
	map<string, DockerRegistry> _dockerRegistries;

	/**
	 * imageId-><image, sha>
	 */
	unordered_map<string, pair<string, string>> _baseImages;

};

#endif
