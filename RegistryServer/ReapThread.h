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

#ifndef __REAP_THREAD_H__
#define __REAP_THREAD_H__

#include <iostream>
#include "util/tc_thread.h"
#include "DbHandle.h"

using namespace tars;

//////////////////////////////////////////////////////
/**
 * 用于执行定时操作的线程类
 */
class ReapThread : public TC_Thread, public TC_ThreadLock
{
public:
    /**
     * 构造函数
     */
    ReapThread();
    
    /**
     * 析构函数
     */
    ~ReapThread();

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

protected:

	/**
	 * 启动时加载基础镜像数据
	 */
	void loadDockerRegistry(bool pull);

	/**
	 * 检查
	 */
	void checkDockerRegistries(const map<string, DockerRegistry> &dockerRegistries, bool pull);

protected:
    /*
     * 线程结束标志
     */
    bool       _terminate;

    /*
     * 数据库操作
     */
    CDbHandle  _db;

    /*
     * 加载对象列表的时间间隔,单位是秒
     * 第一阶段加载时间 consider
     */
    int        _loadObjectsInterval1;
    int        _leastChangedTime1;

    /*
     * 全量加载时间,单位是秒
     */
    int        _loadObjectsInterval2;
    int        _leastChangedTime2;
 
    /*
     * registry心跳超时时间
     */
    int        _registryTimeout;

    /*
     * 是否启用DB恢复保护功能，默认为打开
     */
    bool       _recoverProtect;

    /*
     * 启用DB恢复保护功能状态下极限值
     */
    int        _recoverProtectRate;

    /*
     * 主控心跳时间更新开关
     */
    bool       _heartBeatOff;

	/*
	 * 加载docker仓库镜像定时(秒)
	 */
	int        _checkDockerRegistry = 10;

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
