#include "DockerPatch.h"
#include "util/tc_file.h"
#include "util/tc_md5.h"
#include "ServerCommand.h"
#include "util.h"
#include "DockerServerToolBox.h"
#include "RegistryProxy.h"
#include "NodeServer.h"

DockerPatch::DockerPatch(const ServerObjectPtr &server, const tars::PatchRequest &request) : _serverObjectPtr(server), _patchRequest(request) {
}

int DockerPatch::updatePatchResult(string &sResult)
{
    try
	{
        NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << endl;

        //服务发布成功，向主控发送UPDDATE命令
        RegistryPrx proxy = AdminProxy::getInstance()->getRegistryProxy();
        if (!proxy)
		{
            sResult = "patch succ but update version and user fault, get registry proxy fail";

            NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << "|" << sResult << endl;
            g_app.reportServer(_patchRequest.appname + "." + _patchRequest.servername, "", _serverObjectPtr->getNodeInfo().nodeName,sResult);

            return -1;
        }

        //向主控发送命令，设置该服务的版本号和发布者
        struct PatchResult patch;
        patch.sApplication = _patchRequest.appname;
        patch.sServerName = _patchRequest.servername;
        patch.sNodeName = _patchRequest.nodename;
        patch.sVersion = _patchRequest.version;
        patch.sUserName = _patchRequest.user;

        int iRet = proxy->updatePatchResult(patch);
        NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << "|Update:" << iRet << endl;
    }
    catch (exception &e)
	{
        NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << "|Exception:" << e.what() << endl;
        sResult = _patchRequest.appname + "." + _patchRequest.servername + "|Exception:" + e.what();
        g_app.reportServer(_patchRequest.appname + "." + _patchRequest.servername, "", _serverObjectPtr->getNodeInfo().nodeName,sResult);
        return -1;
    }
    catch (...)
	{
        NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << "|Unknown Exception" << endl;
        sResult = _patchRequest.appname + "." + _patchRequest.servername + "|Unknown Exception";
        g_app.reportServer(_patchRequest.appname + "." + _patchRequest.servername, "", _serverObjectPtr->getNodeInfo().nodeName, sResult);
        return -1;
    }

    return 0;
}

DockerPatch::ExeStatus DockerPatch::canExecute(std::string &sResult)
{
    ServerObject::InternalServerState eState = _serverObjectPtr->getLastState();

    ServerObject::InternalServerState eCurrentState = _serverObjectPtr->getInternalState();
    NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << "|sResult:" << sResult
                                  << "|current state" << _serverObjectPtr->toStringState(eCurrentState) << "|last state :" << _serverObjectPtr->toStringState(eState) << endl;

	//析构时会👏🏻还原状态, 不用删除这句话!!!
    _pStatExChange = new StatExChange(_serverObjectPtr, ServerObject::BatchPatching, eState);

    _serverObjectPtr->setPatchVersion(_patchRequest.version);
    _serverObjectPtr->setPatchPercent(0);
    _serverObjectPtr->setPatchResult("");

    return EXECUTABLE;
}
//
//int DockerPatch::execute(std::string &sResult)
//{
//    const std::string sPullOutputFile = _serverObjectPtr->getServerDir() + "/bin/pull";
//
//    _serverObjectPtr->getActivator()->setRedirectPath(sPullOutputFile);
//
////    const std::string &sRegistry = _patchRequest.registry;
//    if (_patchRequest.registry.empty())
//	{
//        throw std::runtime_error("DockerPatch::execute docker registry is empty");
//    }
//    const std::string sDockerImageURL =
//			_patchRequest.registry + "/" + TC_Common::lower(_patchRequest.appname) + "." + TC_Common::lower(_patchRequest.servername) + ":" + _patchRequest.version;
//
//    std::vector<std::string> vOptions = {"pull", sDockerImageURL};
//
//    std::vector<std::string> vEnv;
//
//    TC_File::removeFile(sPullOutputFile, false);
//    int64_t iPid = _serverObjectPtr->getActivator()->activate(getDockerExePath(), "", "", vOptions, vEnv);
//    if (iPid == 0)  //child process or error;
//    {
//        return 0;
//    }
//
//    if (iPid < 0) {
//        throw std::runtime_error("create docker pull process error");
//    }
//
//    waitProcessDone(iPid);
//
//    bool bPatchSuccess = checkPull(sPullOutputFile);
//
//    int iRet = -1;
//
//    if (bPatchSuccess)
//	{
//        //设置发布状态到主控
//        iRet = updatePatchResult(sResult);
//        if (iRet != 0)
//		{
//            NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << "|updatePatchResult fail:" << sResult << endl;
//        }
//		else
//		{
//            sResult = "patch " + _patchRequest.appname + "." + _patchRequest.servername + " succ, version " + _patchRequest.version;
//            //发布成功
//            NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
//        }
//        NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << "setPatched,iPercent=100%" << endl;
//    }
//
//    _serverObjectPtr->setPatchResult(sResult, iRet == 0);
//
//    _serverObjectPtr->setPatched(true);
//
//    _serverObjectPtr->resetCoreInfo();
//
//    g_app.reportServer(_patchRequest.appname + "." + _patchRequest.servername, "", _serverObjectPtr->getNodeInfo().nodeName, sResult);
//
//    return iRet;
//}


int DockerPatch::execute(std::string &sResult)
{
	if (_serverObjectPtr->getDockerRegistry().sRegistry.empty())
	{
		throw std::runtime_error("DockerPatch::execute docker registry is empty");
	}
//
//	const std::string sDockerImageURL =
//			_patchRequest.registry + "/" + TC_Common::lower(_patchRequest.appname) + "/" + TC_Common::lower(_patchRequest.servername) + ":" + _patchRequest.version;

	string cmd = g_app.getDockerPull() + " '" + _serverObjectPtr->getDockerImage(_patchRequest.version)  + "' '" + _serverObjectPtr->getDockerRegistry().sUserName + "' '" + _serverObjectPtr->getDockerRegistry().sPassword + "'";

	NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << cmd << endl;

	string output = TC_Port::exec(cmd.c_str());

	NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << output << endl;

	int iRet = -1;

	if (output.find("docker pull succ") != string::npos)
	{
		//设置发布状态到主控
		iRet = updatePatchResult(sResult);
		if (iRet != 0)
		{
			NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << _patchRequest.appname + "." + _patchRequest.servername << "|updatePatchResult fail:" << sResult << endl;
		}
		else
		{
			sResult = "patch " + _patchRequest.appname + "." + _patchRequest.servername + " succ, version " + _patchRequest.version;
			//发布成功
			NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
		}
		NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << "setPatched,iPercent=100%" << endl;
	}

	_serverObjectPtr->setPatchResult(sResult, iRet == 0);

	_serverObjectPtr->setPatched(true);

	_serverObjectPtr->resetCoreInfo();

	g_app.reportServer(_patchRequest.appname + "." + _patchRequest.servername, "", _serverObjectPtr->getNodeInfo().nodeName, sResult);

	return iRet;
}

