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

#include "CommandStart.h"
#include "util/tc_docker.h"
#if TARGET_PLATFORM_LINUX
#include <sys/types.h>
#include <sys/wait.h>
#endif


#if TARGET_PLATFORM_WINDOWS
#define TARS_START "start /b "
#define TARS_SCRIPT "tars_start.bat"

#else
#define TARS_START ""
#define TARS_SCRIPT "tars_start.sh"
#endif

//////////////////////////////////////////////////////////////
//
ServerCommand::ExeStatus CommandStart::canExecute(string& sResult)
{
    TC_ThreadRecLock::Lock lock(*_serverObjectPtr);

    NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << _desc.application << "." << _desc.serverName << " beging activate------|byRegistry|" << _byNode << endl;

    ServerObject::InternalServerState eState = _serverObjectPtr->getInternalState();

    if (_byNode)
    {
        _serverObjectPtr->setEnabled(true);
    }
    else if (!_serverObjectPtr->isEnabled())
    {
        sResult = "server is disabled";
        NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return DIS_EXECUTABLE;
    }

    if (eState == ServerObject::Active || eState == ServerObject::Activating)
    {
        _serverObjectPtr->synState();
        sResult = "server is already " + _serverObjectPtr->toStringState(eState);
        NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return NO_NEED_EXECUTE;
    }

    if (eState != ServerObject::Inactive)
    {
        sResult = "server state is not Inactive. the curent state is " + _serverObjectPtr->toStringState(eState);
        NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return DIS_EXECUTABLE;
    }

    if ( _serverObjectPtr->getStartScript().empty())
	{
		bool findExe = true;

        if(_serverObjectPtr->getServerType() == "tars_cpp" || _serverObjectPtr->getServerType() == "tars_go")
		{
			string server;

    		//为了兼容服务exe文件改名, 如果exeFile不存在, 则遍历目录下, 第一个文件名带Server的可执行程序

			findExe = searchServer(_serverObjectPtr->getExePath(), server);
		}
		else if(_serverObjectPtr->getServerType() == "tars_nodejs")
		{
			//对于tars_nodejs类型需要修改下_exeFile
			_exeFile = _serverObjectPtr->getExePath() + FILE_SEP + string("tars_nodejs") + FILE_SEP + string("node");

			if(!TC_File::isFileExist(_exeFile))
			{
				_exeFile = "node";
			}
		}
		else if(_serverObjectPtr->getServerType() == "tars_java")
		{
			if(TC_File::isAbsolute(_exeFile) && !TC_File::isFileExist(_exeFile))
			{
				_exeFile = "java";
			}
		}
		else if(_serverObjectPtr->getServerType() == "tars_php")
		{
			if(TC_File::isAbsolute(_exeFile) && !TC_File::isFileExist(_exeFile))
			{
				_exeFile = "php";
			}
		}

    	if(!findExe)
    	{
    		_serverObjectPtr->setPatched(false);
    		sResult      = "server start error: " + _exeFile + " is not exist.";
    		NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
			g_app.reportServer(_serverObjectPtr->getServerId(), "", _serverObjectPtr->getNodeInfo().nodeName,
					sResult);

    		return DIS_EXECUTABLE;
    	}
    	else
    	{
    		sResult      = "find server exe: " + _exeFile;
    		NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
    	}
    }

    _serverObjectPtr->setPatched(true);

    _serverObjectPtr->setState(ServerObject::Activating, false); //此时不通知regisrty。checkpid成功后再通知

    return EXECUTABLE;
}

bool CommandStart::searchServer(const string &searchDir, string &server)
{
	vector<string> files;

	TC_File::listDirectory(searchDir, files, false);

	vector<string> dirs;

	for(auto &f : files)
	{
		if(TC_File::isFileExist(f, S_IFDIR))
		{
			dirs.push_back(f);
			continue;
		}

		//找到了可执行程序
		if(f == _exeFile)
		{
			server = f;
			TC_File::setExecutable(f, true);
			return true;
		}

		if(!TC_File::canExecutable(f))
		{
			continue;
		}

		string fileName = TC_File::extractFileName(f);
		if(fileName.find("Server") != string::npos || fileName.find("server") != string::npos)
		{
			server = f;
			return true;
		}
	}

	for(auto &dir : dirs)
	{
		if (searchServer(dir, server))
			return true;
	}

	return false;
}

string CommandStart::getStartScript(const ServerObjectPtr &serverObjectPtr)
{
	return TC_File::simplifyDirectory(serverObjectPtr->getExePath() + FILE_SEP + TARS_SCRIPT);
}

#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_IOS

//这段代码如果调用会导致子进程启动时, 总是提示端口被占用, 要启动好几次, 原因未知!!!
int CommandStart::waitProcessDone(int64_t iPid)
{
	assert(iPid > 0);

	if (iPid <= 0)
	{
		throw std::runtime_error("iPid must bigger than zero");
	}

	int process_status;

	while (true)
	{
		int v = ::waitpid(iPid, &process_status, WNOHANG);
		if (v != 0)
		{
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return process_status;
}

#endif

bool CommandStart::startByScript(string& sResult)
{
    bool bSucc = false;
    string sStartScript     = _serverObjectPtr->getStartScript();
    string sMonitorScript   = _serverObjectPtr->getMonitorScript();
    string sServerId    = _serverObjectPtr->getServerId();

    _serverObjectPtr->getActivator()->activate(sStartScript, sMonitorScript, sResult);

    time_t tNow = TNOW;
    int iStartWaitInterval = START_WAIT_INTERVAL;

    //服务启动,超时时间自己定义的情况
    TC_Config conf;
    conf.parseFile(_serverObjectPtr->getConfigFile());
    iStartWaitInterval = TC_Common::strto<int>(conf.get("/tars/application/server<activating-timeout>", "3000")) / 1000;
    if (iStartWaitInterval < START_WAIT_INTERVAL)
    {
        iStartWaitInterval = START_WAIT_INTERVAL;
    }
    if (iStartWaitInterval > 60)
    {
        iStartWaitInterval = 60;
    }

    while ((TNOW - iStartWaitInterval) < tNow)
    {
        if(_serverObjectPtr->savePid())
        {
            bSucc = true;
            break;
        }

		std::this_thread::sleep_for(std::chrono::milliseconds(START_SLEEP_INTERVAL/1000));
    }

    if (_serverObjectPtr->checkPid() != 0)
    {
        sResult = sResult + ", get pid for server[" + sServerId + "], pid is not digit";
        NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << sResult << endl;

        throw runtime_error(sResult);
    }

    return bSucc;
}

bool CommandStart::searchPackage(const string &searchDir, string &package)
{
	vector<string> files;

	TC_File::listDirectory(searchDir, files, false);

	vector<string> dirs;

	for(auto &f : files)
	{
		string fileName = TC_File::extractFileName(f);

		if(fileName == "node_modules" || fileName == "tars_nodejs")
		{
			continue;
		}

		if(TC_File::isFileExist(f, S_IFDIR))
		{
			dirs.push_back(f);
			continue;
		}

		if(fileName == "package.json")
		{
			package = f;
			return true;
		}
	}

	for(auto &dir : dirs)
	{
		if (searchPackage(dir, package))
			return true;
	}

	return false;
}

void CommandStart::prepareScript()
{
	vector<string> vEnvs;
	vector<string> vOptions;
	string sConfigFile      = _serverObjectPtr->getConfigFile();
	string sLogPath         = _serverObjectPtr->getLogPath();
	string sServerDir       = _serverObjectPtr->getServerDir();
	string sLibPath         = _serverObjectPtr->getLibPath();
	string sExePath         = _serverObjectPtr->getExePath();

	//环境变量设置
	vector<string> vecEnvs = TC_Common::sepstr<string>(_serverObjectPtr->getEnv(), ";|");
	for (vector<string>::size_type i = 0; i < vecEnvs.size(); i++)
	{
		vEnvs.push_back(vecEnvs[i]);
	}

	//生成启动脚本
	std::ostringstream osStartStcript;
#if TARGET_PLATFORM_WINDOWS
	vEnvs.push_back("PATH=%PATH%;" + sExePath + ";" + sLibPath);

	// osStartStcript << "set PATH = %PATH%; /etc/profile" << std::endl;
    for (vector<string>::size_type i = 0; i < vEnvs.size(); i++)
    {
        if(i == 0)
        {
            osStartStcript << "set ";
        }
        osStartStcript << vEnvs[i] << ";";
    }
    osStartStcript << endl;

#else
	vEnvs.push_back("LD_LIBRARY_PATH=$LD_LIBRARY_PATH:" + sExePath + ":" + sLibPath);

	osStartStcript << "#!/bin/sh" << std::endl;

	if(_serverObjectPtr->getRunType() != ServerObject::Container)
	{
		osStartStcript << ". /etc/profile" << std::endl;
	}

	for (vector<string>::size_type i = 0; i < vEnvs.size(); i++)
	{
		osStartStcript << "export " << vEnvs[i] << std::endl;
	}
#endif

	osStartStcript << std::endl;
	if (_serverObjectPtr->getServerType() == "tars_java")
	{
		//java服务
		string packageFormat= _serverObjectPtr->getPackageFormat();
		string sClassPath   = _serverObjectPtr->getClassPath();
		vOptions.push_back("-Dconfig=" + sConfigFile);
		string s ;
		if("jar" == packageFormat)
		{
			s = _serverObjectPtr->getJvmParams() +  " " + _serverObjectPtr->getMainClass();
		}else {
			s = _serverObjectPtr->getJvmParams() + " -cp " + sClassPath +"/*" + " " + _serverObjectPtr->getMainClass();
		}
		vector<string> v = TC_Common::sepstr<string>(s, " \t");
		vOptions.insert(vOptions.end(), v.begin(), v.end());

		osStartStcript << TARS_START << _exeFile << " " << TC_Common::tostr(vOptions);
	}
	else if (_serverObjectPtr->getServerType() == "tars_nodejs")
	{
		vOptions.emplace_back(TC_File::simplifyDirectory(
				sExePath + FILE_SEP + string("tars_nodejs") + FILE_SEP + string("node-agent") + FILE_SEP +
				string("bin") + FILE_SEP + string("node-agent")));

		string package;
		string src;
		if(searchPackage(sExePath, package))
		{
			NODE_LOG(_serverObjectPtr->getServerId())->debug() << "search nodejs package.json: " << package << endl;

			src = TC_File::simplifyDirectory(TC_File::extractFilePath(package)) + " -c " + sConfigFile;
		}
		else
		{
			src = TC_File::simplifyDirectory(sExePath) + " -c " + sConfigFile;
		}

		vector<string> v = TC_Common::sepstr<string>(src," \t");
		vOptions.insert(vOptions.end(), v.begin(), v.end());

		osStartStcript << TARS_START << _exeFile <<" "<<TC_Common::tostr(vOptions);
	}
	else if (_serverObjectPtr->getServerType() == "tars_php")
	{
		TC_Config conf;
		conf.parseFile(sConfigFile);
		string entrance = sServerDir + FILE_SEP + "bin" + FILE_SEP + "src" + FILE_SEP + "index.php";
		entrance = conf.get("/tars/application/server<entrance>", entrance);

		vOptions.push_back(entrance);
		vOptions.push_back("--config=" + sConfigFile);

		osStartStcript << TARS_START << _exeFile << " " << TC_Common::tostr(vOptions) ;
	}
	else
	{
		//c++ or go
		vOptions.push_back("--config=" + sConfigFile);
		osStartStcript  << TARS_START << _exeFile << " " << TC_Common::tostr(vOptions) ;
	}

	if(_serverObjectPtr->getRunType() != ServerObject::Container)
	{
#if !TARGET_PLATFORM_WINDOWS
		osStartStcript << " &" << endl;
#endif
	}

	//保存启动方式到bin目录供手工启动
	TC_File::save2file(getStartScript(_serverObjectPtr), osStartStcript.str());
	TC_File::setExecutable(getStartScript(_serverObjectPtr), true);
}


bool CommandStart::startNormal(string& sResult)
{
	int64_t iPid = -1;
	bool bSucc  = false;
	string sRollLogFile;

	string sLogPath = _serverObjectPtr->getLogPath();

	if (sLogPath != "")
	{
		sRollLogFile = sLogPath + FILE_SEP + _desc.application + FILE_SEP + _desc.serverName + FILE_SEP + _desc.application + "." + _desc.serverName + ".log";
	}

	const string sPwdPath = sLogPath != "" ? sLogPath : _serverObjectPtr->getServerDir(); //pwd patch 统一设置至log目录 以便core文件发现 删除

	iPid = _serverObjectPtr->getActivator()->activate(getStartScript(_serverObjectPtr), sPwdPath, sRollLogFile, vector<string>());
	if (iPid == 0)  //child process
	{
		return false;
	}

	_serverObjectPtr->setPid(iPid);

	bSucc = (_serverObjectPtr->checkPid() == 0) ? true : false;

	return bSucc;
}

bool CommandStart::startContainer(const ServerObjectPtr &serverObjectPtr, string &sResult)
{
	TC_Docker docker;
	docker.setDockerUnixLocal(g_app.getDocketSocket());

	vector<string> entrypoint;
	entrypoint.emplace_back(CommandStart::getStartScript(serverObjectPtr));
	map<string, string> mounts;
	mounts[serverObjectPtr->getExePath()] = serverObjectPtr->getExePath();
	mounts[serverObjectPtr->getDataDir()] = serverObjectPtr->getDataDir();
	mounts[serverObjectPtr->getConfigPath()] = serverObjectPtr->getConfigPath();
	mounts[serverObjectPtr->getLogPath()] = serverObjectPtr->getLogPath();
	mounts["/etc/localtime"] = "/etc/localtime";

	for(auto &volume: serverObjectPtr->getVolumes())
	{
		mounts[volume.first] = volume.second;
	}

	NODE_LOG(serverObjectPtr->getServerId())->debug() << "mounts:" << TC_Common::tostr(mounts.begin(), mounts.end(), " ") << endl;

	map<string, pair<string, int>> ports;

	string networkMode = "host";
#if !TARGET_PLATFORM_LINUX
	ports = serverObjectPtr->getPorts();
	networkMode = "bridge";
#endif

	for(auto port : ports)
	{
		NODE_LOG(serverObjectPtr->getServerId())->debug() << port.first << " " << port.second.first<<":"<< port.second.second << endl;
	}
	bool succ = false;

	docker.remove(serverObjectPtr->getServerId(), true);

	succ = docker.create(serverObjectPtr->getServerId(), serverObjectPtr->getServerDescriptor().baseImage, entrypoint, {}, {}, mounts, ports, "", 0, networkMode, "host", true, false);
	if(!succ)
	{
		string result = "create container error: " + docker.getErrMessage();
		NODE_LOG(serverObjectPtr->getServerId())->debug() << result << endl;
		g_app.reportServer(serverObjectPtr->getServerId(), "", serverObjectPtr->getNodeInfo().nodeName, result);
	}
	else
	{
		succ = docker.start(serverObjectPtr->getServerId());
		if(!succ)
		{
			string result = "start container error: " + docker.getErrMessage();
			NODE_LOG(serverObjectPtr->getServerId())->debug() << result << endl;
			g_app.reportServer(serverObjectPtr->getServerId(), "", serverObjectPtr->getNodeInfo().nodeName, result);
		}
		else
		{
			succ = docker.inspectContainer(serverObjectPtr->getServerId());

			if(succ)
			{
				JsonValueObjPtr oPtr = JsonValueObjPtr::dynamicCast(TC_Json::getValue(docker.getResponseMessage()));

				JsonValueObjPtr sPtr = JsonValueObjPtr::dynamicCast(oPtr->value["State"]);

				int64_t value = JsonValueNumPtr::dynamicCast(sPtr->value["Pid"])->value;

				serverObjectPtr->setPid(value);

				return true;
			}
		}
	}
//
//	ostringstream os;
//	os << serverObjectPtr->getExePath() << ":" << serverObjectPtr->getExePath() << " ";
// 	os << serverObjectPtr->getDataDir() << ":" << serverObjectPtr->getDataDir() << " ";
//	os << serverObjectPtr->getConfigPath() << ":" << serverObjectPtr->getConfigPath() << " ";
//	os << serverObjectPtr->getLogPath() << ":" << serverObjectPtr->getLogPath() << " ";
//	os << "/etc/localtime" << ":" << "/etc/locatime" << " ";
//
//	vector<string> vOptions = {"run", "--rm", "--name", serverObjectPtr->getServerId()};
//	vOptions.emplace_back("-v");
//	vOptions.emplace_back(serverObjectPtr->getExePath() + ":" + serverObjectPtr->getExePath());
//	vOptions.emplace_back("-v");
//	vOptions.emplace_back(serverObjectPtr->getDataDir() + ":" + serverObjectPtr->getDataDir());
//	vOptions.emplace_back("-v");
//	vOptions.emplace_back(serverObjectPtr->getConfigPath() + ":" + serverObjectPtr->getConfigPath());
//	vOptions.emplace_back("-v");
//	vOptions.emplace_back(serverObjectPtr->getLogPath() + ":" + serverObjectPtr->getLogPath());
//	vOptions.emplace_back("-v");
//	vOptions.emplace_back("/etc/localtime:/etc/localtime");
//
//
//	vOptions.emplace_back(serverObjectPtr->getServerDescriptor().baseImage);
//	vOptions.emplace_back(CommandStart::getStartScript(serverObjectPtr));
//
////	int64_t iPid = serverObjectPtr->getActivator()->activate("docker", "", sRollLogFile, vOptions);//, vEnvs);
//	int64_t iPid = serverObjectPtr->getActivator()->activate("docker", "", "", vOptions);//, vEnvs);
//
//	if (iPid <= 0)  //child process or error;
//	{
//		return false;
//	}
//
//	NODE_LOG(serverObjectPtr->getServerId())->debug() << FILE_FUN << "activate succ, pid:" << iPid << endl;


	return false;
}

//////////////////////////////////////////////////////////////
//
int CommandStart::execute(string& sResult)
{
	bool bSucc  = false;

    int64_t startMs = TC_TimeProvider::getInstance()->getNowMs();
    try
    {
        //set stdout & stderr
        _serverObjectPtr->getActivator()->setRedirectPath(_serverObjectPtr->getRedirectPath());

		if (!_serverObjectPtr->getStartScript().empty() || _serverObjectPtr->isTarsServer() == false)
		{
			bSucc = startByScript(sResult);
		}
		else
		{
			prepareScript();

			if(_serverObjectPtr->getRunType() == ServerObject::Container)
			{
				g_app.getDockerPullThread()->pull(_serverObjectPtr, [startMs](ServerObjectPtr server, bool succ, string &result){

					NODE_LOG(server->getServerId())->debug() << "pull container finish " << (succ?"succ":"failed") << endl;

					if(!succ)
					{
						startFinish(server, false, startMs, result);
					}
					else
					{
						succ = startContainer(server, result);

						startFinish(server, succ, startMs, result);
					}

				});

				return 0;

			}
			else
			{
				bSucc = startNormal(sResult);
			}
		}
    }
    catch (exception& e)
    {
        sResult = e.what();
    }
    catch (const std::string& e)
    {
        sResult = e;
    }
    catch (...)
    {
        sResult = "catch unknown exception";
    }

	startFinish(_serverObjectPtr, bSucc, startMs, sResult);

    return bSucc?0:-1;
}

void CommandStart::startFinish(const ServerObjectPtr &serverObjectPtr, bool bSucc, int64_t startMs, const string &sResult)
{
	NODE_LOG(serverObjectPtr->getServerId())->debug() << FILE_FUN << "start finish: " << (bSucc?"succ":"failed") << ", use:"
													   << (TNOWMS - startMs)
													   << " ms" << endl;
	if(bSucc)
	{
		serverObjectPtr->synState();
		serverObjectPtr->setLastKeepAliveTime(TNOW);
		serverObjectPtr->setLimitInfoUpdate(true);
		serverObjectPtr->setStarted(true);
		serverObjectPtr->setStartTime(TNOWMS);
	}
	else
	{
		NODE_LOG(serverObjectPtr->getServerId())->debug() << FILE_FUN << "start failed, result:" << sResult << endl;

		serverObjectPtr->setPid(0);
		serverObjectPtr->setState(ServerObject::Inactive);
		serverObjectPtr->setStarted(false);
	}

}
