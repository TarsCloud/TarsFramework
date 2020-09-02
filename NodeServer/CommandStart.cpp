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

    if (TC_File::isAbsolute(_exeFile) &&
        !TC_File::isFileExistEx(_exeFile) &&
        _serverObjectPtr->getStartScript().empty() &&
        _serverObjectPtr->getServerType() != "tars_nodejs" && _serverObjectPtr->getServerType() != "tars_php")
    {
        _serverObjectPtr->setPatched(false);
        sResult      = "The server exe patch " + _exeFile + " is not exist.";
        NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << sResult << endl;
        return DIS_EXECUTABLE;
    }

    _serverObjectPtr->setPatched(true);

    _serverObjectPtr->setState(ServerObject::Activating, false); //此时不通知regisrty。checkpid成功后再通知

    return EXECUTABLE;
}

bool CommandStart::startByScript(string& sResult)
{
    // int64_t iPid = -1;
    bool bSucc = false;
    string sStartScript     = _serverObjectPtr->getStartScript();
    string sMonitorScript   = _serverObjectPtr->getMonitorScript();

    string sServerId    = _serverObjectPtr->getServerId();
    _serverObjectPtr->getActivator()->activate(sStartScript, sMonitorScript, sResult);

    // vector<string> vtServerName =  TC_Common::sepstr<string>(sServerId, ".");
    // if (vtServerName.size() != 2)
    // {
    //     sResult = sResult + "|failed to get pid for  " + sServerId + ",server id error";
    //     NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << sResult  << endl;
    //     throw runtime_error(sResult);
    // }

    // //默认使用启动脚本路径
    // string sFullExeFileName = TC_File::extractFilePath(sStartScript) + vtServerName[1];
    // if (!TC_File::isFileExist(sFullExeFileName))
    // {
    //     NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << "file name " << sFullExeFileName << " error, use exe file" << endl;

    //     //使用exe路径
    //     sFullExeFileName = _serverObjectPtr->getExeFile();
    //     if (!TC_File::isFileExist(sFullExeFileName))
    //     {
    //         sResult = sResult + "|failed to get full exe file name|" + sFullExeFileName;
    //         NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << sResult << endl;
    //         throw runtime_error(sResult);
    //     }
    // }
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

    // string sPidFile = ServerConfig::TarsPath + FILE_SEP + "tarsnode" + FILE_SEP + "data" + FILE_SEP + sServerId + FILE_SEP + sServerId + ".pid";

    // string sGetServerPidScript = "ps -ef | grep -v 'grep' |grep -iE '" + sFullExeFileName + "'| awk '{print $2}' > " + sPidFile;

    while ((TNOW - iStartWaitInterval) < tNow)
    {
        if(_serverObjectPtr->savePid())
        {
            bSucc = true;
            break;
        }
        // //注意:由于是守护进程,不要对sytem返回值进行判断,始终wait不到子进程
        // system(sGetServerPidScript.c_str());
        // string sPid = TC_Common::trim(TC_File::load2str(sPidFile));
        // if (TC_Common::isdigit(sPid))
        // {
        //     iPid = TC_Common::strto<int>(sPid);
        //     _serverObjectPtr->setPid(iPid);
        //     if (_serverObjectPtr->checkPid() == 0)
        //     {
        //         bSucc = true;
        //         break;
        //     }
        // }

        // NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << _desc.application << "." << _desc.serverName << " activating usleep " << int(iStartWaitInterval) << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(START_SLEEP_INTERVAL/1000));
    }

    if (_serverObjectPtr->checkPid() != 0)
    {
        sResult = sResult + "|get pid for server[" + sServerId + "],pid is not digit";
        NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << sResult << endl;
        // if (TC_File::isFileExist(sPidFile))
        // {
        //     TC_File::removeFile(sPidFile, false);
        // }
        throw runtime_error(sResult);
    }

    // if (TC_File::isFileExist(sPidFile))
    // {
    //     TC_File::removeFile(sPidFile, false);
    // }
    return bSucc;
}


bool CommandStart::startNormal(string& sResult)
{
	int64_t iPid = -1;
    bool bSucc  = false;
    string sRollLogFile;
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
	osStartStcript << "source /etc/profile" << std::endl;
    for (vector<string>::size_type i = 0; i < vEnvs.size(); i++)
    {
        osStartStcript << "export " << vEnvs[i] << std::endl;
    }
#endif

    osStartStcript << std::endl;
    if (_serverObjectPtr->getServerType() == "tars_java")
    {
        //java服务
        TC_Config conf;
        conf.parseFile(_serverObjectPtr->getConfigFile());
        string packageFormat= conf.get("/tars/application/server<packageFormat>","war");
        string sClassPath       = _serverObjectPtr->getClassPath();
        vOptions.push_back("-Dconfig=" + sConfigFile);
        string s ;
        if("jar" ==packageFormat)
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
        vOptions.push_back(sExePath + FILE_SEP + string("tars_nodejs") + FILE_SEP + string("node-agent") + FILE_SEP + string("bin") + FILE_SEP + string("node-agent"));
        string s = sExePath + FILE_SEP + "src" + FILE_SEP +" -c " + sConfigFile;
        vector<string> v = TC_Common::sepstr<string>(s," \t");
        vOptions.insert(vOptions.end(), v.begin(), v.end());

        //对于tars_nodejs类型需要修改下_exeFile
        _exeFile = sExePath + FILE_SEP + string("tars_nodejs") + FILE_SEP + string("node");

        osStartStcript << TARS_START << _exeFile <<" "<<TC_Common::tostr(vOptions);
    }
    else if (_serverObjectPtr->getServerType() == "tars_php")
    {
        TC_Config conf;
        conf.parseFile(sConfigFile);
        string entrance    = sServerDir + FILE_SEP + "bin" + FILE_SEP + "src" + FILE_SEP + "index.php";
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

#if !TARGET_PLATFORM_WINDOWS
    osStartStcript << " &" << endl;
#endif

    string scriptFile = sExePath + FILE_SEP + TARS_SCRIPT;

    //保存启动方式到bin目录供手工启动
    TC_File::save2file(scriptFile, osStartStcript.str());
    TC_File::setExecutable(scriptFile, true);

    if (sLogPath != "")
    {
        sRollLogFile = sLogPath + FILE_SEP + _desc.application + FILE_SEP + _desc.serverName + FILE_SEP + _desc.application + "." + _desc.serverName + ".log";
    }

    const string sPwdPath = sLogPath != "" ? sLogPath : sServerDir; //pwd patch 统一设置至log目录 以便core文件发现 删除
    iPid = _serverObjectPtr->getActivator()->activate(_exeFile, sPwdPath, sRollLogFile, vOptions, vEnvs);
    // iPid = _serverObjectPtr->getActivator()->activate(sExePath, sPwdPath, sRollLogFile, vOptions, vEnvs);
    if (iPid == 0)  //child process
    {
        return false;
    }

    _serverObjectPtr->setPid(iPid);

    bSucc = (_serverObjectPtr->checkPid() == 0) ? true : false;

    return bSucc;
}

// inline bool CommandStart::startByScriptPHP(string& sResult)
// {
//     //生成启动shell脚本 
//     string sConfigFile      = _serverObjectPtr->getConfigFile();
//     string sLogPath         = _serverObjectPtr->getLogPath();
//     string sServerDir       = _serverObjectPtr->getServerDir();
//     string sLibPath         = _serverObjectPtr->getLibPath();
//     string sExePath         = _serverObjectPtr->getExePath();
//     string sLogRealPath     = sLogPath + _desc.application + FILE_SEP + _desc.serverName + FILE_SEP ;
//     string sLogRealPathFile = sLogRealPath +_serverObjectPtr->getServerId() +".log";

//     TC_Config conf;
//     conf.parseFile(sConfigFile);
//     string phpexecPath = "";
//     string entrance = "";
//     try{
//         phpexecPath = TC_Common::strto<string>(conf["/tars/application/server<php>"]);
//         entrance =  TC_Common::strto<string>(conf["/tars/application/server<entrance>"]);
//     } catch (...){}
//     entrance = entrance=="" ? sServerDir+"/bin/src/index.php" : entrance;

//     std::ostringstream osStartStcript;
//     osStartStcript << "#!/bin/sh" << std::endl;
//     osStartStcript << "if [ ! -d \"" << sLogRealPath << "\" ];then mkdir -p \"" << sLogRealPath << "\"; fi" << std::endl;
//     osStartStcript << phpexecPath << " " << entrance <<" --config=" << sConfigFile << " start  >> " << sLogRealPathFile << " 2>&1 " << std::endl;
//     osStartStcript << "echo \"end-tars_start.sh\"" << std::endl;

//     TC_File::save2file(sExePath + FILE_SEP + TARS_SCRIPT, osStartStcript.str());
//     TC_File::setExecutable(sExePath + FILE_SEP + TARS_SCRIPT, true);

//     int64_t iPid = -1;
//     bool bSucc = false;

//     string sStartScript     = _serverObjectPtr->getStartScript();
//     sStartScript = sStartScript=="" ? _serverObjectPtr->getExePath() + FILE_SEP + TARS_SCRIPT : sStartScript;
//     string sMonitorScript   = _serverObjectPtr->getMonitorScript();
//     string sServerId    = _serverObjectPtr->getServerId();
//     iPid = _serverObjectPtr->getActivator()->activate(sStartScript, sMonitorScript, sResult);


//     vector<string> vtServerName =  TC_Common::sepstr<string>(sServerId, ".");
//     if (vtServerName.size() != 2)
//     {
//         sResult = sResult + "|failed to get pid for  " + sServerId + ",server id error";
//         NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << sResult  << endl;
//         throw runtime_error(sResult);
//     }

//     //默认使用启动脚本路径
//     string sFullExeFileName = TC_File::extractFilePath(sStartScript) + vtServerName[1];
//     time_t tNow = TNOW;
//     int iStartWaitInterval = START_WAIT_INTERVAL;
//     try
//     {
//         //服务启动,超时时间自己定义的情况
//         TC_Config conf;
//         conf.parseFile(_serverObjectPtr->getConfigFile());
//         iStartWaitInterval = TC_Common::strto<int>(conf.get("/tars/application/server<activating-timeout>", "3000")) / 1000;
//         if (iStartWaitInterval < START_WAIT_INTERVAL)
//         {
//             iStartWaitInterval = START_WAIT_INTERVAL;
//         }
//         if (iStartWaitInterval > 60)
//         {
//             iStartWaitInterval = 60;
//         }

//     }
//     catch (...)
//     {
//     }                

//     string sPidFile = "/usr/local/app/tars/tarsnode/util/" + sServerId + ".pid";
//     string sGetServerPidScript = "ps -ef | grep -v 'grep' |grep -iE ' " + _desc.application+"."+_desc.serverName + "'|grep master| awk '{print $2}' > " + sPidFile;

//     while ((TNOW - iStartWaitInterval) < tNow)
//     {
//         //注意:由于是守护进程,不要对sytem返回值进行判断,始终wait不到子进程
//         system(sGetServerPidScript.c_str());
//         string sPid = TC_Common::trim(TC_File::load2str(sPidFile));
//         if (TC_Common::isdigit(sPid))
//         {
//             iPid = TC_Common::strto<int>(sPid);
//             _serverObjectPtr->setPid(iPid);
//             if (_serverObjectPtr->checkPid() == 0)
//             {
//                 bSucc = true;
//                 break;
//             }
//         }

//         NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << _desc.application << "." << _desc.serverName << " activating usleep " << int(iStartWaitInterval) << endl;
//         TC_Common::msleep(START_SLEEP_INTERVAL/1000);
//     }

//     if (_serverObjectPtr->checkPid() != 0)
//     {
//         sResult = sResult + "|get pid for server[" + sServerId + "],pid is not digit";
//         NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << sResult << endl;
//         if (TC_File::isFileExist(sPidFile))
//         {
//             TC_File::removeFile(sPidFile, false);
//         }
//         throw runtime_error(sResult);
//     }

//     if (TC_File::isFileExist(sPidFile))
//     {
//         TC_File::removeFile(sPidFile, false);
//     }
//     return bSucc;
// }

//////////////////////////////////////////////////////////////
//
int CommandStart::execute(string& sResult)
{
    int64_t startMs = TC_TimeProvider::getInstance()->getNowMs();
    try
    {
        bool bSucc  = false;
        //set stdout & stderr
        _serverObjectPtr->getActivator()->setRedirectPath(_serverObjectPtr->getRedirectPath());

        // if( TC_Common::lower(TC_Common::trim(_desc.serverType)) == "tars_php" ){

        //     bSucc = startByScriptPHP(sResult);

        // }else 

        if (!_serverObjectPtr->getStartScript().empty() || _serverObjectPtr->isTarsServer() == false){

            bSucc = startByScript(sResult);
        }
        else
        {
            bSucc = startNormal(sResult);
        }

        if (bSucc == true)
        {
            _serverObjectPtr->synState();
            _serverObjectPtr->setLastKeepAliveTime(TNOW);
            _serverObjectPtr->setLimitInfoUpdate(true);
            _serverObjectPtr->setStarted(true);
			_serverObjectPtr->setStartTime(TNOWMS);
            NODE_LOG(_serverObjectPtr->getServerId())->debug() << FILE_FUN << _desc.application << "." << _desc.serverName << "_" << _desc.nodeName << " start succ " << sResult << ", use:" << (TC_TimeProvider::getInstance()->getNowMs() - startMs) << " ms" << endl;

            return 0;
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
        sResult = "catch unkwon exception";
    }

    NODE_LOG(_serverObjectPtr->getServerId())->error() << FILE_FUN << _desc.application << "." << _desc.serverName << " start  failed :" << sResult << ", use:" << (TC_TimeProvider::getInstance()->getNowMs() - startMs) << " ms" << endl;

    _serverObjectPtr->setPid(0);
    _serverObjectPtr->setState(ServerObject::Inactive);
	_serverObjectPtr->setStarted(false);

    return -1;
}

