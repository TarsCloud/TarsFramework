#include "DockerStart.h"
#include "ServerObject.h"
#include "util/tc_thread_mutex.h"

using namespace tars;

DockerStart::DockerStart(const ServerObjectPtr &pServerObjectPtr, bool bByNode)
        : _bByNode(bByNode), _pServerObjectPtr(pServerObjectPtr) {
    _pServerObjectPtr->getLogPath();
    _tDesc = _pServerObjectPtr->getServerDescriptor();
}

ServerCommand::ExeStatus DockerStart::canExecute(string &sResult) {

    TC_ThreadRecLock::Lock lock(*_pServerObjectPtr);

    NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << " begging activate------|byRegistry|" << _bByNode << endl;

    ServerObject::InternalServerState eState = _pServerObjectPtr->getInternalState();

    if (_bByNode)
	{
        _pServerObjectPtr->setEnabled(true);
    }
	else if (!_pServerObjectPtr->isEnabled())
	{
        sResult = "server is disabled";
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return DIS_EXECUTABLE;
    }

    if (eState == ServerObject::Active || eState == ServerObject::Activating)
	{
        _pServerObjectPtr->synState();
        sResult = "server is already " + _pServerObjectPtr->toStringState(eState);
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return NO_NEED_EXECUTE;
    }

    if (eState != ServerObject::Inactive) {
        sResult = "server state is not Inactive. the current state is " + _pServerObjectPtr->toStringState(eState);
        NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return DIS_EXECUTABLE;
    }

    _pServerObjectPtr->setPatched(true);

    _pServerObjectPtr->setState(ServerObject::Activating, false); //此时不通知 registry, check pid 成功后再通知

    return EXECUTABLE;
}

inline int DockerStart::execute(string &sResult)
{
    int64_t startMs = TC_TimeProvider::getInstance()->getNowMs();
    try
	{
        _pServerObjectPtr->getActivator()->setRedirectPath(_pServerObjectPtr->getRedirectPath());

        bool bSuccess = startNormal(sResult);

        if (bSuccess)
		{
            _pServerObjectPtr->synState();
            _pServerObjectPtr->setLastKeepAliveTime(TNOW);
            _pServerObjectPtr->setLimitInfoUpdate(true);
            NODE_LOG(_pServerObjectPtr->getServerId())->debug() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << "_" << _tDesc.nodeName << " docker start success " << sResult
                                             << ", use:"
                                             << (TC_TimeProvider::getInstance()->getNowMs() - startMs) << " ms" << endl;
            return 0;
        }
    }
    catch (exception &e) {
        sResult = e.what();
    }
    catch (const std::string &e) {
        sResult = e;
    }
    catch (...) {
        sResult = "catch unknown exception";
    }
    NODE_LOG(_pServerObjectPtr->getServerId())->error() << FILE_FUN << _tDesc.application << "." << _tDesc.serverName << " start  failed :" << sResult << ", use:"
                                     << (TC_TimeProvider::getInstance()->getNowMs() - startMs) << " ms" << endl;
    _pServerObjectPtr->setPid(0);
    _pServerObjectPtr->setState(ServerObject::Inactive);
    return -1;
}

inline bool DockerStart::startNormal(string &sResult) {

    const string sServerDir = _pServerObjectPtr->getServerDir();
    const string sLibPath = _pServerObjectPtr->getLibPath();
    const string sExePath = _pServerObjectPtr->getExePath();
    const string sConfigFile = _pServerObjectPtr->getConfigFile();
    const std::string sServerType = _pServerObjectPtr->getServerType();
    const std::string sLogPath = _pServerObjectPtr->getLogPath();

    const std::string sBaseDir = TC_File::simplifyDirectory(sServerDir + FILE_SEP + "bin" + FILE_SEP);
    const std::string sConfDir = TC_File::simplifyDirectory(sServerDir + FILE_SEP + "conf"  + FILE_SEP);
    const std::string sDataDir = TC_File::simplifyDirectory(sServerDir + FILE_SEP + "data" + FILE_SEP);

    bool mkdir_success = TC_File::makeDirRecursive(sBaseDir) && TC_File::makeDirRecursive(sConfDir) && TC_File::makeDirRecursive(sDataDir);
    if (!mkdir_success) {
        NODE_LOG(_pServerObjectPtr->getServerId())->error() << FILE_FUN << "create dir error: " << strerror(errno) << endl;
        return false;
    }

    //生成 docker-compose.yml
    DockerComposeYamlBuilder oBuilder(_tDesc.application, _tDesc.serverName, _pServerObjectPtr->getDockerImage(_tDesc.patchVersion));

    std::string sEntrypoint;
    std::vector<std::string> vEntrypointOptions{};

    vector<string> vecEnvs = TC_Common::sepstr<string>(_pServerObjectPtr->getEnv(), ";|");
    for (const auto &env:vecEnvs) {
        oBuilder.addEnv(env);
    }

    vector<string> vecVolumes = TC_Common::sepstr<string>(_pServerObjectPtr->getVolumes(), ";|");
    for (const auto &volume:vecVolumes) {
        oBuilder.addVolume(volume, volume);
    }

    if (sServerType == "tars_java")
	{
    }
	else if (sServerType == "tars_nodejs")
	{
        constexpr char FIXED_NODE_AGENT_PATH[] = "/usr/local/app/tars/tars-node-agent/bin/tars-node-agent";
        sEntrypoint = _pServerObjectPtr->getExeFile();
        vEntrypointOptions = {
                FIXED_NODE_AGENT_PATH,
                sBaseDir,
                "-c",
                sConfigFile
        };
    } else if (sServerType == "tars_cpp" || sServerType == "tars_go" ) {
        oBuilder.addEnv("LD_LIBRARY_PATH=" + sExePath + ":" + sLibPath);
        sEntrypoint = TC_File::simplifyDirectory(sBaseDir + FILE_SEP + _tDesc.serverName);
        vEntrypointOptions = {
                "--config=" + sConfigFile
        };
    } else {
        NODE_LOG(_pServerObjectPtr->getServerId())->error() << FILE_FUN << "bad server type" << endl;
        return false;
    }

    assert(!sEntrypoint.empty());

    const std::string sDockerComposeYamlPath = TC_File::simplifyDirectory(sBaseDir + FILE_SEP + "docker-compose.yml");
    const std::string sDockerComposeYamlContent = oBuilder.build(sEntrypoint, vEntrypointOptions, sBaseDir, sConfDir, sDataDir, sLogPath);
    TC_File::save2file(sDockerComposeYamlPath, sDockerComposeYamlContent);

    const std::string sDockerComposeExeFile = getDockerComposeExePath();
    const std::string sDockerComposeProjectName = _tDesc.application + "." + _tDesc.serverName;

    const vector<string> vOptions = {"-f", sDockerComposeYamlPath, "-p", sDockerComposeProjectName, "up", "-d"};

    //生成手动启动脚本
    std::ostringstream osStartScript;
    osStartScript << "#!/bin/sh" << std::endl;
    osStartScript << sDockerComposeExeFile;
    for (const auto &option:vOptions) {
        osStartScript << " " << option;
    }
    osStartScript << std::endl;
    TC_File::save2file(sBaseDir + "tars_start.sh", osStartScript.str());
    TC_File::setExecutable(sBaseDir + "tars_start.sh", true);

    //准备执行环境以及启动
    std::string sRollLogFile;
    if (!sLogPath.empty()) {
        sRollLogFile = sLogPath + _tDesc.application + FILE_SEP + _tDesc.serverName + FILE_SEP + _tDesc.application + "." + _tDesc.serverName + ".log";
    }

    vector<string> vEnvs;

    int64_t iPid = _pServerObjectPtr->getActivator()->activate(sDockerComposeExeFile, "", sRollLogFile, vOptions, vEnvs);

    if (iPid <= 0)  //child process or error;
    {
        return false;
    }

	_pServerObjectPtr->setPid(iPid);

	return (_pServerObjectPtr->checkPid() == 0) ? true : false;

//    waitProcessDone(iPid);    //此处无法也没必要得知是否真正启动成功, 由KeepAlive ,和 CheckServer 机制去检测.
//    return true;
}
