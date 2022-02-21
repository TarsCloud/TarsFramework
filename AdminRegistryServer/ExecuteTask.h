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

#ifndef __EXECUTE_TASK_H__
#define __EXECUTE_TASK_H__

#include <iostream>
#include "util/tc_thread_pool.h"
#include "util/tc_singleton.h"
#include "AdminReg.h"
#include "AdminRegistryImp.h"

using namespace tars;
constexpr int TASK_ITEM_SHARED_STATED_DEFAULT = 23232323;
class TaskList : public TC_ThreadMutex
{
public:
    /**
     * 构造函数
     */
    TaskList(const TaskReq &taskReq, unsigned int t = 300);

	/**
	 *
	 */
    ~TaskList();

	/**
	 * 任务状态
	 * @return
	 */
    TaskRsp getTaskRsp();

	/**
	 * 取消任务
	 */
    void cancelTask();

	/**
	 * 执行
	 */
	void execute();

	/**
     * 创建时间
     */
    time_t getCreateTime() { return _createTime; }

	/**
	 * 是否执行超时了
	 * @return
	 */
    bool isTimeout();

	/**
	 * 是否完成了
	 * @return
	 */
    bool isFinished() const { return _finished; }

protected:
	virtual void onExecute() = 0;

	//准备镜像或者文件
	int prepareFile();

    //设置应答信息
    void setRspInfo(size_t index, bool start, EMTaskItemStatus status);

    //设置应答的log
    void setRspLog(size_t index, const string &log);
    void setRspPercent(size_t index, int percent);

    EMTaskItemStatus executeSingleTask(size_t index, const TaskItemReq &req);

//    EMTaskItemStatus executeSingleTaskWithSharedState(size_t index, const TaskItemReq &req);
    EMTaskItemStatus start        (const TaskItemReq &req, string &log);
    EMTaskItemStatus restart      (const TaskItemReq &req, string &log);
    EMTaskItemStatus graceRestart (const TaskItemReq &req, string &log);
    EMTaskItemStatus stop         (const TaskItemReq &req, string &log);
    EMTaskItemStatus patch        (size_t index, const TaskItemReq &req, string &log);
//    EMTaskItemStatus patch		  (const TaskItemReq &req, string &log, size_t index);
    EMTaskItemStatus undeploy     (const TaskItemReq &req, string &log);
//    EMTaskItemStatus gridPatchServer(const TaskItemReq &req, string &log);
    string get(const string &name, const map<string, string> &parameters);

    void finish() { _finished = true; }

protected:
    
    //线程池
    TC_ThreadPool   _pool;
	//准备
	TC_ThreadPool   _preparePool;
	//请求任务
    TaskReq         _taskReq;
    //返回任务
    TaskRsp         _taskRsp;

	AdminRegistryImp*	_adminPrx;
    time_t          _createTime;
    unsigned int    _timeout;
    bool            _finished;
};

class TaskListSerial : public TaskList
{
public:
    TaskListSerial(const TaskReq &taskReq, unsigned int t = 300);

    virtual void onExecute();

protected:
    void doTask();
};

class TaskListParallel : public TaskList
{
public:
    TaskListParallel(const TaskReq &taskReq, unsigned int t = 300);

    virtual void onExecute();

protected:
    void doTask(TaskItemReq req, size_t index);
};

class TaskListElegant : public TaskList
{
public:
    TaskListElegant(const TaskReq &taskReq, unsigned int t = 600);

    virtual void onExecute();

protected:
    void doTask();
    void doTaskParallel(TaskItemReq req, size_t index);

protected:
    TC_ThreadPool   _poolMaster;
};

class ExecuteTask : public TC_Singleton<ExecuteTask>, public TC_ThreadLock, public TC_Thread
{
public:
    /**
     * 添加任务请求
     * 
     * @param taskList 
     * @param serial 
     * @param current 
     * 
     * @return string
     */
    int addTaskReq(const TaskReq &taskReq);

	/**
	 * 取消任务
	 * @param taskNo
	 * @return
	 */
    int cancelTask(const string& taskNo);

    /**
     * 获取任务状态
     *
     * @param taskIdList : 任务列表id
     *
     * @return 任务状态
     */
    bool getTaskRsp(const string &taskNo, TaskRsp &taskRsp);

    /**
     * 检查task的状态
     * 
     * @param taskRsp 
     */
    void checkTaskRspStatus(TaskRsp &taskRsp);

    unsigned int getElegantWaitSecond() const
    {
        return _elegantWaitSecond;
    }

    void terminate();
public:
     ExecuteTask();
	 void init(TC_Config *conf);
	 AdminRegistryImp* getAdminImp() { return _adminImp; }
    ~ExecuteTask();
    virtual void run();
protected:

	TC_ThreadMutex	_initLock;
    map<string, TaskList*> _task;
    map<string, string> _server2ElegantTask;

	AdminRegistryImp*	_adminImp;

	unsigned int _elegantWaitSecond;

    //线程结束标志
    bool _terminate;
};

#endif
