#include "DockerStop.h"

#include "ServerCommand.h"

using namespace std;

DockerStop::DockerStop(const ServerObjectPtr &pServerObjectPtr, bool bUseAdmin, bool bByNode, bool bGenerateCore)
        : _pServerObjectPtr(pServerObjectPtr), _bUseAdmin(bUseAdmin), _bByNode(bByNode), _bGenerateCore(bGenerateCore) {
    _tDesc = _pServerObjectPtr->getServerDescriptor();
}

//////////////////////////////////////////////////////////////
//
ServerCommand::ExeStatus DockerStop::canExecute(string &sResult) {
    TC_ThreadRecLock::Lock lock(*_pServerObjectPtr);

    NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << " beging deactivate------|byRegistry|" << _bByNode << endl;

    ServerObject::InternalServerState eState = _pServerObjectPtr->getInternalState();

    if (_bByNode)
	{
        _pServerObjectPtr->setEnabled(false);
    }

    if (eState == ServerObject::Inactive) {
        _pServerObjectPtr->synState();
        sResult = "server state is Inactive. ";
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return NO_NEED_EXECUTE;
    }
    if (eState == ServerObject::Destroying) {
        sResult = "server state is Destroying. ";
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return DIS_EXECUTABLE;
    }

    if (eState == ServerObject::Patching || eState == ServerObject::BatchPatching) {
        PatchInfo tPatchInfo;
        _pServerObjectPtr->getPatchPercent(tPatchInfo);
        //状态为Pathing时 10秒无更新才允许stop
        std::string sPatchType = eState == ServerObject::BatchPatching ? "BatchPatching" : "Patching";
        time_t iTimes = eState == ServerObject::BatchPatching ? 10 * 60 : 10;
        if (TNOW - tPatchInfo.iModifyTime < iTimes) {
            sResult = "server is " + sPatchType + " " + TC_Common::tostr(tPatchInfo.iPercent) + "%, please try again later.....";
            NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << "DockerStop::canExecute|" << sResult << endl;
            return DIS_EXECUTABLE;
        } else {
            NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << "server is patching " << tPatchInfo.iPercent << "% ,and no modify info for " << TNOW - tPatchInfo.iModifyTime << "s"
                                            << endl;
        }
    }

    _pServerObjectPtr->setState(ServerObject::Deactivating);

    return EXECUTABLE;

}


//////////////////////////////////////////////////////////////
//
int DockerStop::execute(string &sResult)
{
    bool needWait = false;
    try
	{
        int64_t pid = _pServerObjectPtr->getPid();
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << "pid:" << pid << endl;
#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_IOS
        if (pid != 0) {
            string f = "/proc/" + TC_Common::tostr(pid) + "/status";
            NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << "print the server status :" << f << "|" << TC_File::load2str(f) << endl;
        }
#endif
        string sStopScript = _pServerObjectPtr->getStopScript();
        //配置了脚本或者非taf服务
        if (!sStopScript.empty() || _pServerObjectPtr->isTarsServer() == false) {
            map<string, string> mResult;
            string sServerId = _pServerObjectPtr->getServerId();
            _pServerObjectPtr->getActivator()->doScript(sStopScript, sResult, mResult);
            needWait = true;
        } else {
            if (_bUseAdmin) {
                AdminFPrx pAdminPrx;    //服务管理代理
                string sAdminPrx = "AdminObj@" + _pServerObjectPtr->getLocalEndpoint().toString();
                NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName
                                                << " call " << sAdminPrx << endl;
                pAdminPrx = Application::getCommunicator()->stringToProxy<AdminFPrx>(sAdminPrx);
                pAdminPrx->async_shutdown(NULL);
                needWait = true;
            }
        }

    }
    catch (exception &e) {
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << " shut down result:|" << e.what() << "|use kill -9 for check" << endl;
        int64_t pid = _pServerObjectPtr->getPid();
        _pServerObjectPtr->getActivator()->deactivate(pid);
    }
    catch (...) {
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << " shut down fail:|" << "|use kill -9 for check" << endl;
        int64_t pid = _pServerObjectPtr->getPid();
        _pServerObjectPtr->getActivator()->deactivate(pid);
    }

    //等待STOP_WAIT_INTERVAL秒
    time_t tNow = TNOW;
    int iStopWaitInterval = STOP_WAIT_INTERVAL;
    try {
        //服务停止,超时时间自己定义的情况
        TC_Config conf;
        conf.parseFile(_pServerObjectPtr->getConfigFile());
        iStopWaitInterval = TC_Common::strto<int>(conf["/taf/application/server<deactivating-timeout>"]) / 1000;
        if (iStopWaitInterval < STOP_WAIT_INTERVAL) {
            iStopWaitInterval = STOP_WAIT_INTERVAL;
        }
        if (iStopWaitInterval > 60) {
            iStopWaitInterval = 60;
        }

    }
    catch (...) {
    }

    if (needWait) {
        while (TNOW - iStopWaitInterval < tNow) {
            if (_pServerObjectPtr->checkPid() != 0)  //如果pid已经不存在
            {
                _pServerObjectPtr->setPid(0);
                _pServerObjectPtr->setState(ServerObject::Inactive);
                NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << "server already stop, stop_cost:" << TNOW - tNow << endl;
                return 0;
            }
            //NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << " deactivating usleep " << int(STOP_SLEEP_INTERVAL) << endl;
            std::this_thread::sleep_for(std::chrono::microseconds(STOP_SLEEP_INTERVAL / 1000));
        }
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << " shut down timeout ( used " << iStopWaitInterval << "'s), use kill -9"
                                        << endl;
    }

    //仍然失败。用kill -9，再等待STOP_WAIT_INTERVAL秒。
    int64_t pid = _pServerObjectPtr->getPid();
    if (_bGenerateCore == true) {
        _pServerObjectPtr->getActivator()->deactivateAndGenerateCore(pid);
    } else {
        _pServerObjectPtr->getActivator()->deactivate(pid);
    }

    tNow = TNOW;
    while (TNOW - STOP_WAIT_INTERVAL < tNow) {
        if (_pServerObjectPtr->checkPid() != 0)  //如果pid已经不存在
        {
            _pServerObjectPtr->setPid(0);
            _pServerObjectPtr->setState(ServerObject::Inactive);
            NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << ", stop server by kill-9 succ. kill_cost:" << TNOW - tNow << endl;
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(STOP_SLEEP_INTERVAL / 1000));
    }
    sResult = "server pid " + TC_Common::tostr(pid) + " is still exist";

    NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << ", " << sResult << ", kill_cost:" << TNOW - tNow << endl;

    return -1;
}

