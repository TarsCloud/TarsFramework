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

#include "ExecuteTask.h"
#include "servant/Communicator.h"
#include "servant/RemoteLogger.h"
#include "util/tc_timeprovider.h"
#include <thread>
extern TC_Config * g_pconf;

TaskList::TaskList(const TaskReq &taskReq, unsigned int t)
: _taskReq(taskReq)
{
    //_adminPrx = Communicator::getInstance()->stringToProxy<AdminRegPrx>(g_pconf->get("/tars/objname<AdminRegObjName>", ""));
	_adminPrx = ExecuteTask::getInstance()->getAdminImp();

    _taskRsp.taskNo   = _taskReq.taskNo;
    _taskRsp.serial   = _taskReq.serial;
    _taskRsp.userName = _taskReq.userName;
	_taskRsp.createTime = TC_Common::now2str("%Y-%m-%d %H:%M:%S");
	_taskRsp.status = EM_T_NOT_START;

    for (size_t i=0; i < taskReq.taskItemReq.size(); i++)
    {
        TaskItemRsp rsp;
        rsp.req       = taskReq.taskItemReq[i];
        rsp.startTime = "";
        rsp.endTime   = "";
        rsp.status    = EM_I_NOT_START;
        rsp.statusInfo= etos(rsp.status);

        _taskRsp.taskItemRsp.push_back(rsp);
    }

	_preparePool.init(1);
	_preparePool.start();

    _createTime = TC_TimeProvider::getInstance()->getNow();
    _timeout = t;
    _finished = false;
}

bool TaskList::isTimeout()
{
    return (_createTime + _timeout < TNOW);
}

TaskList::~TaskList()
{
    TLOG_DEBUG("TaskList::TaskList stop thread ========================= " << endl);
	_preparePool.stop();
    _pool.stop();
}

TaskRsp TaskList::getTaskRsp()
{
    TC_LockT<TC_ThreadMutex> lock(*this);

    return _taskRsp;
}

void TaskList::cancelTask()
{
    TC_LockT<TC_ThreadMutex> lock(*this);

    int cancelNum = 0;
    for (size_t i = 0; i < _taskRsp.taskItemRsp.size(); ++i)
    {
        if (_taskRsp.taskItemRsp[i].status == EM_I_NOT_START)
        {
            _taskRsp.taskItemRsp[i].status = EM_I_CANCEL;
            cancelNum++;
            TLOG_DEBUG("cancel taskItem:" << _taskRsp.taskItemRsp[i].req.taskNo << endl);
        }
    }

    if (cancelNum > 0)
    {
        _taskRsp.status = EM_T_CANCEL;
        TLOG_DEBUG("cancel task:" << _taskRsp.taskNo << ", cancel item num:" << cancelNum << endl);
    }
}

void TaskList::setRspInfo(size_t index, bool start, EMTaskItemStatus status)
{
    TaskItemRsp rsp;
    map<string, string> info;
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        if (start)
        {
            _taskRsp.taskItemRsp[index].startTime = TC_Common::now2str("%Y-%m-%d %H:%M:%S"); 
            info["start_time"] = _taskRsp.taskItemRsp[index].startTime;
        }
        else
        {
            _taskRsp.taskItemRsp[index].endTime = TC_Common::now2str("%Y-%m-%d %H:%M:%S"); 
            info["end_time"] = _taskRsp.taskItemRsp[index].endTime;
        }

        _taskRsp.taskItemRsp[index].status    = status;
        info["status"] = TC_Common::tostr(_taskRsp.taskItemRsp[index].status);

        _taskRsp.taskItemRsp[index].statusInfo = etos(status); 

        rsp = _taskRsp.taskItemRsp[index];
    }

    try
    {
        _adminPrx->setTaskItemInfo_inner(rsp.req.itemNo, info);
    }
    catch (exception &ex)
    {
        TLOG_ERROR("error:" << ex.what() << endl);
    }
}

void TaskList::setRspPercent(size_t index, int percent)
{
    TC_LockT<TC_ThreadMutex> lock(*this);

    _taskRsp.taskItemRsp[index].percent = percent; 
}

void TaskList::setRspLog(size_t index, const string &log)
{
    TaskItemRsp rsp;
    map<string, string> info;
    {
        TC_LockT<TC_ThreadMutex> lock(*this);

        _taskRsp.taskItemRsp[index].executeLog = log; 

        rsp = _taskRsp.taskItemRsp[index];

        info["log"] = _taskRsp.taskItemRsp[index].executeLog;
    }

    try
    {
        _adminPrx->setTaskItemInfo_inner(rsp.req.itemNo, info);
    }
    catch (exception &ex)
    {
        TLOG_ERROR("TaskList::setRspLog error:" << ex.what() << endl);
    }
}

void TaskList::execute()
{
	_preparePool.exec([&](){

		//先准备文件
		int ret = prepareFile();

		if (ret != 0)
		{
			for (size_t i = 0; i < _taskReq.taskItemReq.size(); i++)
			{
				if(_taskRsp.taskItemRsp[i].executeLog.empty())
				{
					setRspLog(i, _taskRsp.executeLog);
				}
				setRspInfo(i, false, EM_I_FAILED);
			}
		}
		else
		{
			onExecute();
		}

	});
}

int TaskList::prepareFile()
{
	map<string, PrepareInfo> pis;

	for (size_t i=0; i < _taskReq.taskItemReq.size(); i++)
	{
		auto& req = _taskReq.taskItemReq[i];

		if(req.command.find("patch") != string::npos)
		{
			setRspInfo(i, true, EM_I_PREPARE);

			PrepareInfo pi;
			pi.application = req.application;
			pi.serverName = get("group_name", req.parameters);
			if(pi.serverName.empty())
			{
				pi.serverName = req.serverName;
			}
		
			pi.patchId = get("patch_id", req.parameters);

			string result;

			int ret = _adminPrx->prepareInfo_inner(pi, result);
			if (ret != 0)
			{
				TLOG_ERROR("preparePatch_inner error:" << result << endl);
				setRspLog(i, result);
				return ret;
			}

			req.parameters["md5"] = pi.md5;

			pis[pi.key()] = pi;
		}
	}

	for(auto e : pis)
	{
		string result;

		TLOG_DEBUG("preparePatch_inner prepare :" << e.second.application << "." << e.second.serverName << ", patchId:" << e.second.patchId << ", file:" << e.second.patchFile << ", image:" << e.second.baseImage << endl);

		int ret = this->_adminPrx->preparePatch_inner(e.second, result);
		if(ret != 0)
		{
			TLOG_ERROR("preparePatch_inner error:" << result << endl);
			_taskRsp.executeLog = result;
			return ret;
		}
	}

	return 0;
}

EMTaskItemStatus TaskList::start(const TaskItemReq &req, string &log)
{
    int ret = -1;
    try
    {
		ret = _adminPrx->startServer_inner(req.application, req.serverName, req.nodeName, log);
        if (ret == 0)
            return EM_I_SUCCESS;
    }
    catch (exception &ex)
    {
        log = ex.what();
        TLOG_ERROR("TaskList::executeSingleTask startServer error:" << log << endl);
    }

    TLOG_DEBUG("TaskList::startServer error:" << req.application << "," << req.serverName << "," << req.nodeName << "," << req.userName << ", info:" << log << endl);

    return EM_I_FAILED;
}

EMTaskItemStatus TaskList::restart(const TaskItemReq &req, string &log)
{
    int ret = -1;
    try
    {
		ret = _adminPrx->restartServer_inner(req.application, req.serverName, req.nodeName, log);
        if (ret == 0)
		{
	        return EM_I_SUCCESS;
        }
    }
    catch (exception &ex)
    {
        log = ex.what();
        TLOG_ERROR("TaskList::restartServer error:" << log << endl);
    }

    TLOG_ERROR("TaskList::restartServer error:" << req.application << "," << req.serverName << "," << req.nodeName << "," << req.userName << ", info:" << log << endl);
    return EM_I_FAILED;
}

EMTaskItemStatus TaskList::graceRestart(const TaskItemReq &req, string &log)
{
    int ret = -1;
    try
    {
        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(req.application, req.serverName, req.nodeName, true);
        if (server.size() == 0)
        {
            log = "no servers";
            TLOG_ERROR("TaskList::graceRestartServer no servers" << endl);
            return EM_I_FAILED;
        }
        ret = _adminPrx->notifyServer_inner(req.application, req.serverName, req.nodeName, "tars.gracerestart",log);
        if (ret == 0) {
            return EM_I_SUCCESS;
        }
    }
    catch (exception &ex)
    {
        log = ex.what();
        TLOG_ERROR("TaskList::restartServer error:" << log << endl);
    }

    TLOG_ERROR("TaskList::restartServer error:" << req.application << "," << req.serverName << "," << req.nodeName << "," << req.userName << ", info:" << log << endl);
    return EM_I_FAILED;
}

EMTaskItemStatus TaskList::undeploy(const TaskItemReq &req, string &log)
{
    int ret = -1;
    try
    {
		ret = _adminPrx->undeploy_inner(req.application, req.serverName, req.nodeName, req.userName, log);
        if (ret == 0)
            return EM_I_SUCCESS;
    }
    catch (exception &ex)
    {
        log = ex.what();
        TLOG_ERROR("TaskList::undeploy error:" << log << endl);
    }

    TLOG_ERROR("TaskList::undeploy error:" << req.application << "," << req.serverName << "," << req.nodeName << "," << req.userName << ", info:" << log << endl);
    
    return EM_I_FAILED;
}

EMTaskItemStatus TaskList::stop(const TaskItemReq &req, string &log)
{
    int ret = -1;
    try
    {
		ret = _adminPrx->stopServer_inner(req.application, req.serverName, req.nodeName, log);
        if (ret == 0)
            return EM_I_SUCCESS;
    }
    catch (exception &ex)
    {
        log = ex.what();
        TLOG_ERROR("TaskList::stop error:" << log << endl);
    }

    TLOG_ERROR("TaskList::stop error:" << req.application << "," << req.serverName << "," << req.nodeName << "," << req.userName << ", info:" << log << endl);

    return EM_I_FAILED;
}

string TaskList::get(const string &name, const map<string, string> &parameters)
{
    map<string, string>::const_iterator it = parameters.find(name);
    if (it == parameters.end())
    {
        return "";
    }
    return it->second;
}

EMTaskItemStatus TaskList::patch(size_t index, const TaskItemReq &req, string &log)
{
    try
    {
        int ret = EM_TARS_UNKNOWN_ERR;

        TLOG_DEBUG("TaskList::patch:" << req.writeToJsonString() << ", " << TC_Common::tostr(req.parameters.begin(), req.parameters.end()) << endl);

        string patchId   = get("patch_id", req.parameters);
        string patchType = get("patch_type", req.parameters);
	    string groupName = get("group_name", req.parameters);
		string runType = get("run_type",req.parameters);

        tars::PatchRequest patchReq;
        patchReq.appname    = req.application;
        patchReq.servername = req.serverName;
        patchReq.nodename   = req.nodeName;
        patchReq.version    = patchId;
        patchReq.user       = req.userName;
	    patchReq.groupname  = groupName;
		patchReq.md5		= get("md5",req.parameters);

		//外部是串行处理的
        try
        {
//            std::shared_ptr<atomic_int> taskItemSharedState=std::make_shared<atomic_int>(TASK_ITEM_SHARED_STATED_DEFAULT);
//            ret = _adminPrx->preparePatch_inner(patchReq,log, false,taskItemSharedState);
//            if (ret!=0)
//			{
//                log = "tarsAdminRegistry batchPatch err:" + log;
//                TLOG_ERROR("TaskList::patch batchPatch error:" << log << endl);
//                return EM_I_FAILED;
//            }
//            ret = _adminPrx->batchPatch_inner(patchReq, runType=="container", log);
			ret = _adminPrx->batchPatch_inner(patchReq, log);
		}
        catch (exception &ex)
        {
            log = ex.what();
            TLOG_ERROR("TaskList::patch batchPatch error:" << log << endl);
            return EM_I_FAILED;
        }

        if (ret != EM_TARS_SUCCESS)
        {
            log = "tarsAdminRegistry batchPatch err:" + log;

            TLOG_ERROR("TaskList::patch error:" << req.application << "," << req.serverName << "," << req.nodeName << "," << req.userName << ", info:" << log << endl);
            return EM_I_FAILED;
        }

        // 这里做个超时保护， 否则如果tarsnode状态错误， 这里一直循环调用
        time_t tNow = TNOW;
        unsigned int patchTimeout = TC_Common::strto<unsigned int>(g_pconf->get("/tars/patch/<patch_wait_timeout>", "300"));

        EMTaskItemStatus retStatus = EM_I_FAILED;

        while (TNOW < tNow + patchTimeout)
        {
            PatchInfo pi;

            try
            {
				ret = _adminPrx->getPatchPercent_inner(req.application, req.serverName, req.nodeName, pi);
            }
            catch (exception &ex)
            {
                log = ex.what();
                TLOG_ERROR("getPatchPercent error, ret:" << ret << ", error:" << ex.what() << endl);
            }

            //发布100%
            setRspPercent(index, pi.iPercent);

            if (ret != 0)
            {
                log = pi.sResult;
				_adminPrx->updatePatchLog_inner(req.application, req.serverName, req.nodeName, patchId, req.userName, patchType, false);
                TLOG_ERROR("getPatchPercent error, ret:" << ret << ", " << req.application << "." << req.serverName << "_" << req.nodeName << ", percent:" << pi.iPercent << ", log:" << pi.sResult << endl);
                return EM_I_FAILED;
            }

            if(pi.iPercent == 100 && pi.bSucc)
            {
				_adminPrx->updatePatchLog_inner(req.application, req.serverName, req.nodeName, patchId, req.userName, patchType, true);
                TLOG_DEBUG("getPatchPercent ok, percent:" << pi.iPercent << "%" << endl);
                retStatus = EM_I_SUCCESS;
                break;
            }
            
            TLOG_DEBUG("getPatchPercent percent:" << pi.iPercent << "%, succ:" << pi.bSucc << endl);

			std::this_thread::sleep_for(std::chrono::seconds(1));
        }

		TLOG_DEBUG("getPatchPercent retStatus:" << etos(retStatus) << endl);

		return retStatus;

    }
    catch (exception &ex)
    {
        TLOG_ERROR("TaskList::patch error:" << ex.what() << endl);
        return EM_I_FAILED;
    }
    return EM_I_SUCCESS; 
}
//
//EMTaskItemStatus TaskList::patch(const TaskItemReq &req, string &log,std::size_t index)
//{
//    try
//	{
//        int ret = EM_TARS_UNKNOWN_ERR;
//
//        TLOG_DEBUG("TaskList::patch:" << TC_Common::tostr(req.parameters.begin(), req.parameters.end())
//                                     << req.application << "." << req.serverName << ", " << req.nodeName << endl);
//
//        string patchId = get("patch_id", req.parameters);
//        string patchType = get("patch_type", req.parameters);
//		string runType = get("run_type",req.parameters);
//
//        tars::PatchRequest patchReq;
//        patchReq.appname = req.application;
//        patchReq.servername = req.serverName;
//        patchReq.nodename = req.nodeName;
//        patchReq.version = patchId;
//        patchReq.user = req.userName;
//		patchReq.md5 = get("run_type",req.parameters);
//
//		//外部是并行处理的!
//        try {
////            ret = _adminPrx->preparePatch_inner(patchReq, log, index != 0, taskItemSharedState);
////            if (ret != 0) {
////                log = "tarsAdminRegistry batchPatch err:" + log;
////                TLOG_ERROR("TaskList::patch batchPatch error:" << log << endl);
////                return EM_I_FAILED;
////            }
//			ret = _adminPrx->batchPatch_inner(patchReq, runType=="container", log);
//        }
//        catch (exception &ex)
//        {
//            log = ex.what();
//            TLOG_ERROR("TaskList::patch batchPatch error:" << log << endl);
//            return EM_I_FAILED;
//        }
//
//        if (ret != EM_TARS_SUCCESS)
//        {
//            log = "tarsAdminRegistry batchPatch err:" + log;
//            return EM_I_FAILED;
//        }
//
//		// 这里做个超时保护， 否则如果tarsnode状态错误， 这里一直循环调用
//		time_t tNow = TNOW;
//        unsigned int patchTimeout = TC_Common::strto<unsigned int>(g_pconf->get("/tars/patch/<patch_wait_timeout>", "300"));
//
//        EMTaskItemStatus retStatus = EM_I_FAILED;
//        while (TNOW < tNow + patchTimeout)
//        {
//            PatchInfo pi;
//
//            try
//            {
//				ret = _adminPrx->getPatchPercent_inner(req.application, req.serverName, req.nodeName, pi);
//            }
//            catch (exception &ex)
//            {
//                log = ex.what();
//                TLOG_ERROR("TaskList::patch getPatchPercent error, ret:" << ret << endl);
//            }
//
//            if (ret != 0)
//            {
//				_adminPrx->updatePatchLog_inner(req.application, req.serverName, req.nodeName, patchId, req.userName, patchType, false);
//                TLOG_ERROR("TaskList::patch getPatchPercent error, ret:" << ret << endl);
//                return EM_I_FAILED;
//            }
//
//            if(pi.iPercent == 100 && pi.bSucc)
//            {
//				_adminPrx->updatePatchLog_inner(req.application, req.serverName, req.nodeName, patchId, req.userName, patchType, true);
//                TLOG_DEBUG("TaskList::patch getPatchPercent ok, percent:" << pi.iPercent << "%" << endl);
//                retStatus = EM_I_SUCCESS;
//                break;
//            }
//
//            TLOG_DEBUG("TaskList::patch getPatchPercent percent:" << pi.iPercent << "%, succ:" << pi.bSucc << endl);
//
//			std::this_thread::sleep_for(std::chrono::seconds(1));
//        }
//        return retStatus;
//    }
//    catch (exception &ex)
//    {
//        TLOG_ERROR("TaskList::patch error:" << ex.what() << endl);
//        return EM_I_FAILED;
//    }
//    return EM_I_FAILED;
//}

EMTaskItemStatus TaskList::executeSingleTask(size_t index, const TaskItemReq &req)
{
    TLOG_DEBUG("TaskList::executeSingleTask: taskNo=" << req.taskNo 
            << ",application=" << req.application 
            << ",serverName="  << req.serverName 
            << ",nodeName="    << req.nodeName
            << ",setName="     << req.setName 
            << ",command="     << req.command << endl);

    EMTaskItemStatus ret = EM_I_FAILED;
    string log;
    if (req.command == "stop")
    {
        ret = stop(req, log);
    }
    else if (req.command == "start")
    {
        ret = start(req, log);
    }
    else if (req.command == "restart")
    {
        ret = restart(req, log);
    }
    else if (req.command == "patch_tars")
    {
        ret = patch(index, req, log);
        if (ret == EM_I_SUCCESS && get("bak_flag", req.parameters) != "1")
        {
            //不是备机, 需要重启
            ret = restart(req, log); 
        }
    }
    else if (req.command == "grace_patch_tars")
    {
        ret = patch(index, req, log);
        if (ret == EM_I_SUCCESS && get("bak_flag", req.parameters) != "1")
        {
            //不是备机, 需要重启
            ret = graceRestart(req, log);
        }
    }
    else if (req.command == "undeploy_tars")
    {
        ret = undeploy(req, log);
    }
    else 
    {
        ret = EM_I_FAILED;
        log = "command not support!";
        TLOG_DEBUG("TaskList::executeSingleTask command not support!" << endl);
    }

	TLOG_DEBUG("index: " << index << ", result: " << log << endl);

    setRspLog(index, log);

    return ret; 
}
//
//
//EMTaskItemStatus TaskList::executeSingleTaskWithSharedState(size_t index, const TaskItemReq &req,shared_ptr<atomic_int> &taskItemSharedState)
//{
//    TLOG_DEBUG("TaskList::executeSingleTask: taskNo=" << req.taskNo
//                                                     << ",application=" << req.application
//                                                     << ",serverName=" << req.serverName
//                                                     << ",nodeName=" << req.nodeName
//                                                     << ",setName=" << req.setName
//                                                     << ",command=" << req.command << endl);
//
//    EMTaskItemStatus ret = EM_I_FAILED;
//    string log;
//    if (req.command == "stop")
//    {
//        ret = stop(req, log);
//    }
//    else if (req.command == "start")
//    {
//        ret = start(req, log);
//    }
//    else if (req.command == "restart")
//    {
//        ret = restart(req, log);
//    }
//    else if (req.command == "patch_tars")
//    {
//        ret = patch(req, log, index, taskItemSharedState);
//        if (ret == EM_I_SUCCESS && get("bak_flag", req.parameters) != "1")
//        {
//            //不是备机, 需要重启
//            ret = restart(req, log);
//        }
//    }
//    else if (req.command == "undeploy_tars")
//    {
//        ret = undeploy(req, log);
//    }
//    else
//    {
//        ret = EM_I_FAILED;
//        log = "command not support!";
//        TLOG_DEBUG("TaskList::executeSingleTask command not support!" << endl);
//    }
//
//    setRspLog(index, log);
//
//    return ret;
//}

//////////////////////////////////////////////////////////////////////////////
TaskListSerial::TaskListSerial(const TaskReq &taskReq, unsigned int t)
: TaskList(taskReq, t)
{
    _pool.init(1);
    _pool.start();
}

void TaskListSerial::onExecute()
{
    TLOG_DEBUG("TaskListSerial::execute" << endl);
    
    auto cmd = std::bind(&TaskListSerial::doTask, this);
    _pool.exec(cmd);
}

void TaskListSerial::doTask()
{
	EMTaskItemStatus status = EM_I_SUCCESS;

	for (size_t i = 0; i < _taskReq.taskItemReq.size(); i++)
	{
		TaskItemReq req;

		setRspInfo(i, true, EM_I_RUNNING);
		{
			TC_LockT<TC_ThreadMutex> lock(*this);

			req = _taskReq.taskItemReq[i];
		}

		if (EM_I_SUCCESS == status)
		{
			status = executeSingleTask(i, req);
		}
		else
		{
			//上一个任务不成功, 后续的任务都cancel掉
			status = EM_I_CANCEL;
		}

		setRspInfo(i, false, status);
	}


    finish();
}

TaskListElegant::TaskListElegant(const TaskReq &taskReq, unsigned int t)
: TaskList(taskReq, t)
{
    _poolMaster.init(1);
    _poolMaster.start();
}

void TaskListElegant::onExecute()
{
    TLOG_DEBUG("TaskListElegant::execute" << endl);
    
    auto cmd = std::bind(&TaskListElegant::doTask, this);
    _poolMaster.exec(cmd);
}

void TaskListElegant::doTask()
{
    size_t len = 0;
    size_t eachNum = 0;
    bool isSerial = true;
    {
        TC_LockT<TC_ThreadMutex> lock(*this);
        len = _taskReq.taskItemReq.size();
        eachNum = (size_t)_taskReq.eachNum;
        isSerial = _taskReq.serial;
    }

    if (isSerial)
    {
        _pool.init(1);
    }
    else
    {
        size_t num = eachNum > 5 ? 5 : eachNum;
        num = num < 1 ? 1:num;
        TLOG_DEBUG("each num:" << eachNum << ", realnum:" << num << endl);
        _pool.init(num); 
    }
    _pool.start();

    //EMTaskItemStatus status = EM_I_SUCCESS;

    for (size_t i = 0; i < len; i += eachNum)
    {
        if (!isSerial)
        {
//            std::shared_ptr<atomic_int> TaskItemSharedState;
//
//            if (0 == i)
//            {
//                TaskItemSharedState =std::make_shared<atomic_int>(TASK_ITEM_SHARED_STATED_DEFAULT);
//            }
//            else
//            {
//                TaskItemSharedState =std::make_shared<atomic_int>(0);
//            }

            vector<pair<size_t, TaskItemReq> >  tl;
            vector<string>  nodeList;

            {
                TC_LockT<TC_ThreadMutex> lock(*this);
                // 先确认任务是否取消
                if (_taskRsp.status == EM_T_CANCEL)
                {
                    TLOG_DEBUG("task has canceled:" << _taskRsp.taskNo << endl);
                    break;
                }

                for (size_t j=i;  j < len && j-i < eachNum; j++)
                {
                    tl.push_back(make_pair(j, _taskReq.taskItemReq[j]));
                    nodeList.push_back((_taskReq.taskItemReq[j].nodeName));
                    _taskRsp.taskItemRsp[j].status = EM_I_PAUSE_FLOW;
                }
            }

            // 停掉流量
            int affectNum = DBPROXY->updateServerFlowState(tl[0].second.application, tl[0].second.serverName, nodeList, false);

            if (affectNum > 0)
			{
                TLOG_DEBUG("sleep " << ExecuteTask::getInstance()->getElegantWaitSecond() << "s wait for close flow..." << endl);
                //sleep(75);
                TC_Common::sleep(ExecuteTask::getInstance()->getElegantWaitSecond());
            }
            else
            {
                TLOG_DEBUG("donot need sleep wait for close flow." << endl);
            }

            TLOG_DEBUG("bind task func..." << endl);
            // 执行task
            for (size_t x = 0; x < tl.size(); ++x)
            {
                _pool.exec(std::bind(&TaskListElegant::doTaskParallel, this, std::placeholders::_1, std::placeholders::_2), tl[x].second, tl[x].first);
            }

            TLOG_DEBUG("worker pool waitForAllDone, item num:" << tl.size() << endl);
            _pool.waitForAllDone(-1);

            TLOG_DEBUG("_pool.waitForAllDone finish, stop pool." << endl);
        }
        else
        {
            TLOG_ERROR("Elegant not support serial task!" << endl);
            break;
        }

    }

    _pool.waitForAllDone(-1);
    TLOG_DEBUG("Elegant task finish." << endl);
    _pool.stop();

    finish();
}

void TaskListElegant::doTaskParallel(TaskItemReq req, size_t index)
{
    setRspInfo(index, true, EM_I_RUNNING);

    //do work
    TLOG_DEBUG("taskNo=" << req.taskNo 
                << ",application=" << req.application 
                << ",serverName=" << req.serverName 
                << ",setName=" << req.setName 
                << ",command=" << req.command << endl);

//    EMTaskItemStatus status = executeSingleTaskWithSharedState(index, req, taskItemSharedState);
	EMTaskItemStatus status = executeSingleTask(index, req);

	setRspInfo(index, false, status);
}

//////////////////////////////////////////////////////////////////////////////
TaskListParallel::TaskListParallel(const TaskReq &taskReq, unsigned int t)
: TaskList(taskReq, t)
{
    //最大并行线程数
    size_t num = taskReq.taskItemReq.size() > 5 ? 5 : taskReq.taskItemReq.size();
    num = num < 1 ? 1:num;
    TLOG_DEBUG("TaskListParallel::TaskListParallel req thread num:" << taskReq.taskItemReq.size() << ", realnum:" << num << endl);
    _pool.init(num); 
    _pool.start();
}

void TaskListParallel::onExecute()
{
    TLOG_DEBUG("TaskListParallel::execute" << endl);
    
    TC_LockT<TC_ThreadMutex> lock(*this);

//    std::shared_ptr<atomic_int> TaskItemSharedState=std::make_shared<atomic_int>(TASK_ITEM_SHARED_STATED_DEFAULT);
    for (size_t i=0; i < _taskReq.taskItemReq.size(); i++)
    {
        _pool.exec(std::bind(&TaskListParallel::doTask, this, std::placeholders::_1, std::placeholders::_2), _taskReq.taskItemReq[i], i);
    }
}

void TaskListParallel::doTask(TaskItemReq req, size_t index)
{
    setRspInfo(index, true, EM_I_RUNNING);

    //do work
    TLOG_DEBUG("TaskListParallel::executeTask: taskNo=" << req.taskNo 
                << ",application=" << req.application 
                << ",serverName="  << req.serverName 
                << ",setName="     << req.setName 
                << ",command="     << req.command << endl);

//    EMTaskItemStatus status = executeSingleTaskWithSharedState(index, req, taskItemSharedState);
	EMTaskItemStatus status = executeSingleTask(index, req);

	setRspInfo(index, false, status);
}

/////////////////////////////////////////////////////////////////////////////

ExecuteTask::ExecuteTask()
{
	_adminImp = NULL;
    _terminate = false;
    start();
}

ExecuteTask::~ExecuteTask()
{
    terminate();
}

void ExecuteTask::terminate()
{
    _terminate = true;
    TC_LockT<TC_ThreadLock> lock(*this);
    notifyAll();
}

void ExecuteTask::run()
{
    // const time_t diff = 2*60;//2分钟
    while (!_terminate)
    {
        {
            TC_ThreadLock::Lock lock(*this);
            map<string, TaskList* >::iterator it = _task.begin();
            while (it != _task.end())
            {
                //大于diff 时间数据任务就干掉
                if (it->second->isTimeout())
                {
                    TLOG_DEBUG("==============ExecuteTask::run, delete old task, taskNo=" << it->first << endl);
                    TaskList *tmp = it->second;
                    _task.erase(it++);
                    delete tmp;
                }
                else
                {
                    ++it;
                }
            }
        }

        {
            TC_LockT<TC_ThreadLock> lock(*this);
            timedWait(5*1000);
        }
    }
}

int ExecuteTask::addTaskReq(const TaskReq &taskReq)
{
    TLOG_DEBUG("ExecuteTask::addTaskReq" <<
              ", taskNo=" << taskReq.taskNo <<   
              ", size=" << taskReq.taskItemReq.size() <<
              ", serial=" << taskReq.serial <<
              ", isElegant:" << taskReq.isElegant <<
              ", eachNum:" << taskReq.eachNum <<
              ", userName=" << taskReq.userName << endl);

    if (taskReq.taskItemReq.size() == 0)
    {
        TLOG_ERROR("empty task item." << endl);
        return -1;
    }

    TaskList *p = NULL;
    if (taskReq.isElegant)
    {
        p = new TaskListElegant(taskReq);
    }
    else if (taskReq.serial)
    {
        p = new TaskListSerial(taskReq);
    }
    else
    {
        p = new TaskListParallel(taskReq);
    }

    string runningElegantTask;
    {
        TC_ThreadLock::Lock lock(*this);

        _task[taskReq.taskNo] = p;

        string serverId = taskReq.taskItemReq[0].application + "." + taskReq.taskItemReq[0].serverName;
        auto it = _server2ElegantTask.find(serverId);
        if (it != _server2ElegantTask.end())
        {
            auto it2 = _task.find(it->second);

            if (it2 != _task.end() && (!(it2->second->isFinished())))
            {
                runningElegantTask = it->second;
            }
            else
            {
                _server2ElegantTask.erase(serverId);
            }
        }

        if (taskReq.isElegant)
        {
            _server2ElegantTask[serverId] = taskReq.taskNo;
        }
    }

    if (runningElegantTask != "")
    {
        // 取消正在跑的无损task
        cancelTask(runningElegantTask);
    }

    p->execute();

//    TLOG_DEBUG("add task finish!" << endl);

    return 0;
}

int ExecuteTask::cancelTask(const string &taskNo)
{
    TLOG_DEBUG("ExecuteTask::cancelTask, taskNo=" << taskNo << endl);

    TaskList* pTask = NULL;

    {
        TC_ThreadLock::Lock lock(*this);

        map<string, TaskList*>::iterator it = _task.find(taskNo);

        if( it == _task.end())
        {
            TLOG_ERROR("camnot find task:" << taskNo << endl);
            return -1;
        }
        else
        {
            pTask = it->second;
        }
    }

    if (pTask)
    {
        pTask->cancelTask();
    }

    return 0;
}

bool ExecuteTask::getTaskRsp(const string &taskNo, TaskRsp &taskRsp)
{
//    TLOG_DEBUG("ExecuteTask::getTaskRsp, taskNo=" << taskNo << endl);

    TC_ThreadLock::Lock lock(*this);

    map<string, TaskList*>::iterator it = _task.find(taskNo);

    if( it == _task.end())
    {
        return false;
    }

    taskRsp = (it->second)->getTaskRsp();

//    TLOG_DEBUG("ExecuteTask::getTaskRsp, taskNo=" << taskNo << ", rsp:" << taskRsp.writeToJsonString() << endl);
    ExecuteTask::getInstance()->checkTaskRspStatus(taskRsp);

    return true;
}


void ExecuteTask::checkTaskRspStatus(TaskRsp &taskRsp) 
{
    size_t not_start = 0;
	size_t prepare = 0;
	size_t running   = 0;
    size_t success   = 0;
    size_t failed    = 0;
    size_t cancel    = 0;

    for (unsigned i = 0; i < taskRsp.taskItemRsp.size(); i++) 
    {
        TaskItemRsp &rsp = taskRsp.taskItemRsp[i];

        switch (rsp.status)
        {
        case EM_I_NOT_START:
            ++not_start;
            break;
		case EM_I_PREPARE:
			++prepare;
			break;
        case EM_I_RUNNING:
        case EM_I_PAUSE_FLOW:
            ++running;
            break;
        case EM_I_SUCCESS:
            ++success;
            break;
        case EM_I_FAILED:
            ++failed;
            break;
        case EM_I_CANCEL:
            ++cancel;
            break;
        }
    }
    
    if      (not_start == taskRsp.taskItemRsp.size()) taskRsp.status = EM_T_NOT_START;
	else if (prepare == taskRsp.taskItemRsp.size()) taskRsp.status = EM_T_PREPARE;
	else if (running   == taskRsp.taskItemRsp.size()) taskRsp.status = EM_T_RUNNING;
    else if (success   == taskRsp.taskItemRsp.size()) taskRsp.status = EM_T_SUCCESS;
    else if (failed    == taskRsp.taskItemRsp.size()) taskRsp.status = EM_T_FAILED;
    else if (cancel    == taskRsp.taskItemRsp.size()) taskRsp.status = EM_T_CANCEL;
    else taskRsp.status = EM_T_PARIAL;

}

void ExecuteTask::init(TC_Config *conf)
{
    _elegantWaitSecond = TC_Common::strto<unsigned int>(conf->get("/tars/application/server/<elegant_wait_second>", "75"));
    if (_elegantWaitSecond < 2 || _elegantWaitSecond > 180)
    {
        _elegantWaitSecond = 75;
    }
	_adminImp = new AdminRegistryImp();
	_adminImp->initialize();
}


