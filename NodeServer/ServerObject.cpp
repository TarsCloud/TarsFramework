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

#include "ServerObject.h"
#include "Activator.h"
#include "RegistryProxy.h"
#include "util/tc_port.h"
#include "util/tc_clientsocket.h"
#include "servant/Communicator.h"
#include "NodeServer.h"
#include "CommandStart.h"
#include "CommandNotify.h"
#include "CommandStop.h"
#include "CommandDestroy.h"
#include "CommandAddFile.h"
#include "NodeRollLogger.h"

ServerObject::ServerObject( const ServerDescriptor& tDesc)
: _tarsServer(true)
, _loaded(false)
, _patched(true)
, _noticed(false)
, _noticeFailTimes(0)
, _enSynState(true)
, _pid(0)
, _state(ServerObject::Inactive)
, _limitStateUpdated(false)
, _started(false)
{
    //60秒内最多启动10次，达到10次启动仍失败后,每隔600秒再重试一次
    _activatorPtr  = new Activator(this, 60,10,600);

    //服务相关修改集中放到setServerDescriptor函数中
     setServerDescriptor(tDesc);

    _serviceLimitResource = new ServerLimitResource(5,10,60,_application,_serverName);
}

int64_t ServerObject::savePid()
{
    // vector<string> vtServerName =  TC_Common::sepstr<string>(_serverId, ".");
    // if (vtServerName.size() != 2)
    // {
    //     sResult = sResult + "|failed to get pid for  " + _serverId + ",server id error";
    //     NODE_LOG(_serverId)->error() << FILE_FUN << sResult  << endl;
    //     throw runtime_error(sResult);
    // }

    // time_t tNow = TNOW;
    // int iStartWaitInterval = START_WAIT_INTERVAL;

    // //服务启动,超时时间自己定义的情况
    // TC_Config conf;
    // conf.parseFile(_confFile);
    // iStartWaitInterval = TC_Common::strto<int>(conf.get("/tars/application/server<activating-timeout>", "3000")) / 1000;
    // if (iStartWaitInterval < START_WAIT_INTERVAL)
    // {
    //     iStartWaitInterval = START_WAIT_INTERVAL;
    // }
    // if (iStartWaitInterval > 60)
    // {
    //     iStartWaitInterval = 60;
    // }

    string sPidFile = ServerConfig::TarsPath + FILE_SEP + "tarsnode" + FILE_SEP + "data" + FILE_SEP + _serverId + FILE_SEP + _serverId + ".pid";

#if TARGET_PLATFORM_WINDOWS
    // string sGetServerPidScript = "ps -ef | grep -v 'grep' |grep -iE '" + _startScript + "'| awk '{print $2}' > " + sPidFile;

    string sGetServerPidScript = "wmic process get executablepath,processid | " + ServerConfig::TarsPath + FILE_SEP + "tarsnode\\util\\busybox.exe grep \"" + _startScript + "\" >" + sPidFile ;
#else
    // string sGetServerPidScript = "ps -ef | grep -v 'grep' |grep -iE '" + _startScript + "'| awk '{print $2}' > " + sPidFile;

    string sGetServerPidScript = "ps -ef | grep -v grep | grep -iE '" + _startScript + "' | awk '{print $8 \" \"$2}' >" + sPidFile;
#endif

    // while ((TNOW - iStartWaitInterval) < tNow)
    // {
        //注意:由于是守护进程,不要对sytem返回值进行判断,始终wait不到子进程
    system(sGetServerPidScript.c_str());

    string data = TC_File::load2str(sPidFile);

    int64_t iPid = 0;
    vector<string> v = TC_Common::sepstr<string>(data, " \t");
    if(v.size() > 2)
    {
        string sPid = TC_Common::trim(v[1]);
        if (TC_Common::isdigit(sPid))
        {
            iPid = TC_Common::strto<int64_t>(sPid);
            setPid(iPid);
            if (checkPid() != 0)
            {
                iPid = -1;
            }
        }
    }

    if (TC_File::isFileExist(sPidFile))
    {
        TC_File::removeFile(sPidFile, false);
    }
    
    // NODE_LOG(_serverId)->debug() << FILE_FUN << < " activating usleep " << int(iStartWaitInterval) << endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(START_SLEEP_INTERVAL/1000));
    // }

    return iPid;
}

void ServerObject::setServerDescriptor( const ServerDescriptor& tDesc )
{
    _desc          = tDesc;
    _application   = tDesc.application;
    _serverName    = tDesc.serverName;
    _serverId      = _application+"."+_serverName;
    _desc.settingState == "active"?_enabled = true:_enabled = false;

	NODE_LOG(_serverId)->debug() << "ServerObject::setServerDescriptor "<< _serverId <<endl;

	time_t now = TNOW;
    _adapterKeepAliveTime.clear();

     map<string, AdapterDescriptor>::const_iterator itAdapters;
     for( itAdapters = _desc.adapters.begin(); itAdapters != _desc.adapters.end(); itAdapters++)
     {
         _adapterKeepAliveTime[itAdapters->first] = now;
     }
     _adapterKeepAliveTime["AdminAdapter"] = now;
     setLastKeepAliveTime(now);
}

bool ServerObject::isAutoStart()
{
    Lock lock(*this);

    // NODE_LOG(_serverId)->debug()<<FILE_FUN<< _serverId <<"|"<<_enabled<<"|"<<_loaded<<"|"<<_patched<<"|"<<toStringState(_state)<<endl;
    if(toStringState(_state).find( "ing" ) != string::npos)  //正处于中间态时不允许重启动
    {
	    NODE_LOG(_serverId)->debug() << "ServerObject::isAutoStart " << _serverId <<" not allow to restart in (ing) state"<<endl;
        return false;
    }
    
    if(!_enabled||!_loaded || !_patched || _activatorPtr->isActivatingLimited())
    {
        if(_activatorPtr->isActivatingLimited())
        {
	        NODE_LOG(_serverId)->debug() << "ServerObject::isAutoStart " << _serverId <<" not allow to restart in limited state"<<endl;
        }
        return false;
    }
    return true;
}

void ServerObject::setExeFile(const string &sExeFile)
{
    string ext = TC_Common::trim(TC_File::extractFileExt(sExeFile));

#if TARGET_PLATFORM_WINDOWS
    if(ext.empty())
    {
        _exeFile = sExeFile + ".exe";
    }
    else
    {
        _exeFile = sExeFile;
    }
#else
    _exeFile = sExeFile;
#endif    
    NODE_LOG(_serverId)->debug() << "ServerObject::setExeFile: " << _exeFile <<endl;
}

ServerState ServerObject::getState()
{
    Lock lock(*this);
	NODE_LOG(_serverId)->debug() << "ServerObject::getState " << _serverId <<"'s state is "<<toStringState(_state)<<endl;
    return toServerState( _state );
}

void ServerObject::setState(InternalServerState eState, bool bSynState)
{
    Lock lock(*this);

    if (_state == eState  && _noticed == true)
    {
        return;
    }

    if ( _state != eState )
    {
	    NODE_LOG(_serverId)->debug() << FILE_FUN << _serverId << " State changed! old State:" << toStringState( _state ) << " new State:" << toStringState( eState ) << endl;
        _state = eState;
    }

    if(bSynState == true)
    {
        synState();
    }
}

void ServerObject::setLastState(InternalServerState eState)
{
    Lock lock(*this);

    _lastState = eState;
}

bool ServerObject::isScriptFile(const string &sFileName)
{
    if(TC_Common::lower(TC_File::extractFileExt(sFileName)) == "sh" ) //.sh文件为脚本
    {
        return true;
    }

    if(TC_File::extractFileName(_startScript) == sFileName )
    {
        return true;
    }

    if(TC_File::extractFileName(_stopScript) == sFileName )
    {
        return true;
    }

    if(TC_File::extractFileName(_monitorScript) == sFileName )
    {
        return true;
    }

    return false;
}

//同步更新 重启服务 需要准确通知主控状态时调用
void ServerObject::synState()
{
    Lock lock(*this);
    try
    {
        ServerStateInfo tServerStateInfo;
        tServerStateInfo.serverState    = (IsEnSynState()?toServerState(_state):tars::Inactive);
        tServerStateInfo.processId      = _pid;
        //根据uNoticeFailTimes判断同步还是异步更新服务状态
        //防止主控超时导致阻塞服务上报心跳
        if(_noticeFailTimes < 3)
        {
            int ret = AdminProxy::getInstance()->getRegistryProxy()->tars_set_timeout(1000)->updateServer( _nodeInfo.nodeName,  _application, _serverName, tServerStateInfo);
            onUpdateServerResult(ret);
        }
        else
        {
            AdminProxy::getInstance()->getRegistryProxy()->async_updateServer(this, _nodeInfo.nodeName,  _application, _serverName, tServerStateInfo);
        }

        //日志
        stringstream ss;
        tServerStateInfo.displaySimple(ss);
        NODE_LOG(_serverId)->debug()<<FILE_FUN << "synState" << "|"<< _nodeInfo.nodeName << "|" <<  _serverId << "|" << std::boolalpha << _enSynState <<"|" << ss.str() << endl;

        //_noticed = true;
        //_noticeFailTimes = 0;
    }
    catch (exception &e)
    {
        _noticed = false;
        _noticeFailTimes ++;
	    NODE_LOG(_serverId)->error() << "ServerObject::synState "<<_serverId<<" error times:"<<_noticeFailTimes<<" reason:"<< e.what() << endl;
    }
}

//异步同步状态
void ServerObject::asyncSynState()
{
    Lock lock(*this);
    try
    {
        ServerStateInfo tServerStateInfo;
        tServerStateInfo.serverState    = (IsEnSynState()?toServerState(_state):tars::Inactive);
        tServerStateInfo.processId      = _pid;
        AdminProxy::getInstance()->getRegistryProxy()->async_updateServer( this, _nodeInfo.nodeName,  _application, _serverName, tServerStateInfo);

        //日志
        stringstream ss;
        tServerStateInfo.displaySimple(ss);
        NODE_LOG(_serverId)->debug()<<FILE_FUN<< _nodeInfo.nodeName << "|" << _serverId
                << "|" << std::boolalpha << _enSynState <<"|" << ss.str() << endl;

    }
    catch (exception &e)
    {
	    NODE_LOG(_serverId)->error() << "ServerObject::asyncSynState "<<_serverId<<" error:" << e.what() << endl;
    }
}

ServerState ServerObject::toServerState(InternalServerState eState) const
{
    switch (eState)
    {
        case Inactive:
        {
            return tars::Inactive;
        }
        case Activating:
        {
            return tars::Activating;
        }
        case Active:
        {
            return tars::Active;
        }
        case Deactivating:
        {
            return tars::Deactivating;
        }
        case Destroying:
        {
            return tars::Destroying;
        }
        case Destroyed:
        {
            return tars::Destroyed;
        }
        default:
        {
            return tars::Inactive;
        }
    }
}

string ServerObject::toStringState(InternalServerState eState) const
{
    switch (eState)
    {
        case Inactive:
        {
            return "Inactive";
        }
        case Activating:
        {
            return "Activating";
        }
        case Active:
        {
            return "Active";
        }
        case Deactivating:
        {
            return "Deactivating";
        }
        case Destroying:
        {
            return "Destroying";
        }
        case Destroyed:
        {
            return "Destroyed";
        }
        case Loading:
        {
            return "Loading";
        }
        case Patching:
        {
            return "Patching";
        }
        case BatchPatching:
        {
            return "BatchPatching";
        }
        case AddFilesing:
        {
            return "AddFilesing";
        }
        default:
        {
            ostringstream os;
            os << "state " << eState;
            return os.str();
        }
    };
}

int ServerObject::checkPid()
{
    // Lock lock(*this);
	int iRet = 0;

	if(_pid != 0)
    {
#if TARGET_PLATFORM_WINDOWS
        HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, _pid);
        if (hProcess == NULL)
        {
            iRet = -1;
        }
		CloseHandle(hProcess);
        if (iRet == 0)
        {
            // NODE_LOG(_serverId)->info() <<FILE_FUN<< _serverId << "|" << _pid << " exists, ret:" << iRet << ", " <<  TC_Exception::getSystemCode() << endl;
            return 0;
        }

        NODE_LOG(_serverId)->debug() <<FILE_FUN<< _serverId << "|" << _pid << "| pid exists, ret:" << iRet << ", " << TC_Exception::getSystemCode() << endl;

#else        
        iRet = ::kill(static_cast<pid_t>(_pid), 0);
		if (iRet != 0)// && (errno == ESRCH || errno == ENOENT ))
		{
			NODE_LOG(_serverId)->error() << "kill " << signal << " to pid, pid not exsits, pid:" << _pid << ", catch exception|" << errno << endl;
			return -1;
		}

		// NODE_LOG(_serverId)->debug() <<FILE_FUN<< _serverId << "|" << _pid << "| pid exists, ret:" << iRet << ", " << TC_Exception::getSystemCode() << endl;

		return 0;
#endif
	}

    return iRet;
}

void ServerObject::keepAlive(int64_t pid,const string &adapter)
{
    Lock lock(*this);
    if (pid <= 0)
    {
	    NODE_LOG(_serverId)->error() << "ServerObject::keepAlive "<< _serverId << " pid "<<pid<<" error, pid <= 0"<<endl;
        return;
    }
    // else
    // {
	//     NODE_LOG(_serverId)->debug() << "ServerObject::keepAlive " <<_serverType<< ", pid:" << pid <<", adapter:" << adapter<< endl;
    // }
    time_t now  = TNOW;
    setLastKeepAliveTime(now,adapter);
    
    //Java KeepAliive服务有时会传来一个不存在的PID，会导致不断的启动新Java进程
    if (0 != checkPid() || _state != ServerObject::Active)
    {    
        setPid(pid);
    }

    //心跳不改变正在转换期状态(Activating除外)
    if(toStringState(_state).find("ing") == string::npos || _state == ServerObject::Activating)
    {
        setState(ServerObject::Active);
    }
    // else
    // {
	//     NODE_LOG(_serverId)->debug() << "ServerObject::keepAlive " << _serverId << " State no need changed to active!  State:" << toStringState( _state ) << endl;
    // }

    if(!_loaded)
    {
        _loaded = true;
    }

    if(!_patched)
    {
        _patched = true;
    }
}

void ServerObject::keepActiving(int64_t pid)
{
    Lock lock(*this);
    if (pid <= 0)
    {
	    NODE_LOG(_serverId)->debug()<<FILE_FUN<< _application << "." << _serverName << " pid "<<pid<<" error, pid <= 0"<<endl;
        return;
    }
    else
    {
	    NODE_LOG(_serverId)->debug() << FILE_FUN<<_serverType<< "|pid|" << pid <<"|server|"<<_application << "." << _serverName << endl;
    }
    time_t now  = TNOW;
    setLastKeepAliveTime(now);
    setPid(pid);

    setState(ServerObject::Activating);
}


void ServerObject::setLastKeepAliveTime(time_t t,const string& adapter)
{
    Lock lock(*this);
    _keepAliveTime = t;

	// NODE_LOG(_serverId)->debug() << "setLastKeepAliveTime keepAliveTime:" << _keepAliveTime << ", now:" << TNOW << endl;

    map<string, time_t>::iterator it1 = _adapterKeepAliveTime.begin();
    if(adapter.empty())
    {
        while (it1 != _adapterKeepAliveTime.end() )
        {
            it1->second = t;
            it1++;
        }
        return;
    }
    map<string, time_t>::iterator it2 = _adapterKeepAliveTime.find(adapter);
    if( it2 != _adapterKeepAliveTime.end())
    {
        it2->second = t;
    }
    else
    {
	    NODE_LOG(_serverId)->error() << "ServerObject::setLastKeepAliveTime "<<adapter<<" not registed "<< endl;
    }
}

int ServerObject::getLastKeepAliveTime(const string &adapter)
{
    if(adapter.empty())
    {
        return _keepAliveTime;
    }
    map<string, time_t>::const_iterator it = _adapterKeepAliveTime.find(adapter);
    if( it != _adapterKeepAliveTime.end())
    {
        return it->second;
    }
    return -1;
}


bool ServerObject::isTimeOut(int iTimeout)
{
    Lock lock(*this);
    time_t now = TNOW;
    if(now - _keepAliveTime > iTimeout)
    {
	    NODE_LOG(_serverId)->error() << "ServerObject::isTimeOut server time out, keepAliveTime:" << _keepAliveTime << ", now:" << TNOW << ", diff:" << now - _keepAliveTime<<" > "<<iTimeout<< endl;
        return true;
    }
    map<string, time_t>::const_iterator it = _adapterKeepAliveTime.begin();
    while (it != _adapterKeepAliveTime.end() )
    {
        if(now - it->second > iTimeout)
        {
	        NODE_LOG(_serverId)->error() << "ServerObject::isTimeOut server "<< it->first<<" time out "<<now - it->second<<"|>|"<<iTimeout<<"|"<<TC_Common::tostr(_adapterKeepAliveTime)<< endl;
            return true;
        }
        it++;
    }
    return false;
}

bool ServerObject::isStartTimeOut()
{
	Lock lock(*this);
	int64_t now = TNOWMS;

	//int timeout = 11000;
	int timeout=_activatingTimeout >0 ? _activatingTimeout : 11000;
	if (now - _startTime >= timeout && now - _keepAliveTime >= timeout)
	{
		NODE_LOG(_serverId)->debug()<<FILE_FUN<<"server start time  out "<<now - _startTime << ">" <<timeout<<endl;
		return true;
	}
	NODE_LOG(_serverId)->debug()<<"server start time  out " << now - _startTime << "<" <<timeout<<endl;

	return false;
}

void ServerObject::setVersion( const string & version )
{
    Lock lock(*this);
    try
    {
        _version = version;
        AdminProxy::getInstance()->getRegistryProxy()->async_reportVersion(NULL,_application, _serverName, _nodeInfo.nodeName, version);

    }catch(...)
    {
        _noticed = false;
    }
}

void ServerObject::setPid(int64_t pid)
{
    Lock lock(*this);
    if (pid == _pid)
    {
        return;
    }
    NODE_LOG(_serverId)->debug()<<FILE_FUN<< _application << "." << _serverName << " pid changed! old pid:" << _pid << " new pid:" << pid << endl;
    _pid = pid;
}

void ServerObject::setPatchPercent(const int iPercent)
{
    Lock lock(*this);
    //下载结束仍需要等待本地copy  进度最多设置99% 本地copy结束后设置为100%
    _patchInfo.iPercent      = iPercent>99?99:iPercent;
    _patchInfo.iModifyTime   = TNOW;
}

void ServerObject::setPatchResult(const string &sPatchResult,const bool bSucc)
{
    Lock lock(*this);
    _patchInfo.sResult = sPatchResult;
    _patchInfo.bSucc   = bSucc;
}

void ServerObject::setPatchVersion(const string &sVersion)
{
    Lock lock(*this);
    _patchInfo.sVersion = sVersion;
}

string ServerObject::getPatchVersion()
{
    Lock lock(*this);
    return _patchInfo.sVersion;
}

int ServerObject::getPatchPercent(PatchInfo &tPatchInfo)
{
    NODE_LOG(_serverId)->debug() << "ServerObject::getPatchPercent, "<< _application << "_" << _serverName << "|"<< _serverId<< endl;

    Lock lock(*this);

    tPatchInfo              = _patchInfo;
    //是否正在发布
    tPatchInfo.bPatching    = (_state == ServerObject::Patching ||_state==ServerObject::BatchPatching)?true:false;

    if (tPatchInfo.bSucc == true || _state == ServerObject::Patching || _state == ServerObject::BatchPatching)
    {
	    NODE_LOG(_serverId)->debug() << "ServerObject::getPatchPercent, "<< _desc.application
            << "|" << _desc.serverName << "|succ:" << (tPatchInfo.bSucc?"true":"false") << "|" << toStringState(_state) << "|" << _patchInfo.iPercent << "%|" << _patchInfo.sResult << endl;
        return 0;
    }

	NODE_LOG(_serverId)->debug() << "ServerObject::getPatchPercent "<< _desc.application
           << "|" << _desc.serverName << "|succ:" << (tPatchInfo.bSucc?"true":"false") << "|" << toStringState(_state) << "|" << _patchInfo.iPercent << "%|" << _patchInfo.sResult << endl;
    return -1;
}

string ServerObject::decodeMacro(const string& value) const
{
    string tmp = value;
    map<string,string>::const_iterator it = _macro.begin();
    while(it != _macro.end())
    {
        tmp = TC_Common::replace(tmp, "${" + it->first + "}", TC_ConfigDomain::reverse_parse(it->second));
        ++it;
    }
    return tmp;
}

void ServerObject::setMacro(const map<string,string>& mMacro)
{
    Lock lock(*this);
    map<string,string>::const_iterator it1 = mMacro.begin();
    for(;it1!= mMacro.end();++it1)
    {
        map<string,string>::iterator it2 = _macro.find(it1->first);

        if(it2 != _macro.end())
        {
            it2->second = it1->second;
        }
        else
        {
            _macro[it1->first] = it1->second;
        }
    }
}

void ServerObject::setScript(const string &sStartScript,const string &sStopScript,const string &sMonitorScript )
{

    _startScript    = TC_File::simplifyDirectory(TC_Common::trim(sStartScript));
    _stopScript     = TC_File::simplifyDirectory(TC_Common::trim(sStopScript));
    _monitorScript  = TC_File::simplifyDirectory(TC_Common::trim(sMonitorScript));
}

bool ServerObject::getRemoteScript(const string &sFileName)
{
    string sFile = TC_File::simplifyDirectory(TC_Common::trim(sFileName));
    if(!sFile.empty())
    {
       CommandAddFile commands(this,sFile);
       commands.doProcess(); //拉取脚本
       return true;
    }
    return false;
}

void ServerObject::doMonScript()
{
    try
    {
        string sResult;
        time_t tNow = TNOW;
        if(TC_File::isAbsolute(_monitorScript) == true) //监控脚本
        {
            if(_state == ServerObject::Activating||tNow - _keepAliveTime > ServantHandle::HEART_BEAT_INTERVAL)
            {
                 map<string,string> mResult;
                 _activatorPtr->doScript(_monitorScript,sResult,mResult);
                 if(mResult.find("pid") != mResult.end() && TC_Common::isdigit(mResult["pid"]) == true)
                 {
	                 NODE_LOG(_serverId)->debug() << "ServerObject::doMonScript "<< _serverId << "|"<< mResult["pid"] << endl;
                     keepAlive(TC_Common::strto<int>(mResult["pid"]));
                 }
            }
        }
        else if(!_startScript.empty() || isTarsServer() == false)
        {
            int64_t pid = savePid();
            if(pid >= 0)
            {
                keepAlive(pid); 
            }
        }
    }
    catch (exception &e)
    {
	    NODE_LOG(_serverId)->error() << "ServerObject::doMonScript error:" << e.what() << endl;
    }
}

//checkServer时对服务所占用的内存上报到主控
void ServerObject::checkServer(int iTimeout)
{
    try
    {
        string sResult;

	    int flag = checkPid();

		if(flag != 0 && _state != ServerObject::Inactive)
		{
    		NODE_LOG(_serverId)->info() <<FILE_FUN<< _serverId<<"|" << toStringState(_state) << "|" << _pid << ", flag:" << flag << ", auto start:" << isAutoStart() << endl;
		    NODE_LOG("KeepAliveThread")->info() <<FILE_FUN<< _serverId<<"|" << toStringState(_state) << "|" << _pid << ", flag:" << flag << ", auto start:" << isAutoStart() << endl;

			//pid不存在, 同步状态
			setState(ServerObject::Inactive, true);
		}
		else if(flag == 0)
		{
			//pid存在
			if (_state == ServerObject::Activating)
			{
				if (_started && isStartTimeOut())
				{
					sResult="[alarm] activating,pid not exist";
					NODE_LOG(_serverId)->debug()<<FILE_FUN<<"|sResult:"<<sResult<<endl;
					NODE_LOG("KeepAliveThread")->debug()<<FILE_FUN<<" sResult:"<<sResult<<endl;

					NODE_LOG(_serverId)->debug()<<FILE_FUN<<_serverId<<" "<<sResult << "|pid: " << _pid  << ", state:" << toStringState(_state)<<"|_started:"<<_started<< endl;
					NODE_LOG("KeepAliveThread")->debug()<<FILE_FUN<<_serverId<<" "<<sResult << "|pid:" << _pid  << ", state:" << toStringState(_state)<<"|_started:"<<_started<< endl;
					CommandStop command(this, false);
					command.doProcess();
				}
			}
		}

        //正在运行程序心跳上报超时。
        int iRealTimeout = _timeout > 0?_timeout:iTimeout;
        if( _state == ServerObject::Active && isTimeOut(iRealTimeout))
        {
            sResult = "[alarm] zombie process,no keep alive msg for " + TC_Common::tostr(iRealTimeout) + " seconds";
            NODE_LOG(_serverId)->debug()<<FILE_FUN<<_serverId<<" "<<sResult << endl;
	        NODE_LOG("KeepAliveThread")->debug()<<FILE_FUN<<_serverId<<" "<<sResult << endl;
	        CommandStop command(this, true);
            command.doProcess();
        }

        //启动服务
        if( _state == ServerObject::Inactive && isAutoStart() == true)
        {
            sResult = sResult == ""?"[alarm] down, server is inactive":sResult;
            NODE_LOG(_serverId)->debug() <<FILE_FUN<<_serverId<<" "<<sResult << "|_state:" << toStringState(_state) << endl;
	        NODE_LOG("KeepAliveThread")->debug() <<FILE_FUN<<_serverId<<" "<<sResult << "|_state:" << toStringState(_state) << endl;

	        g_app.reportServer(_serverId, "", getNodeInfo().nodeName, sResult);

            CommandStart command(this);
            int ret = command.doProcess();
            NODE_LOG(_serverId)->debug() <<FILE_FUN<<"|start ret:" << ret << endl;
	        NODE_LOG("KeepAliveThread")->debug() <<FILE_FUN<<"|start ret:" << ret << endl;

	        //配置了coredump检测才进行操作
            if(_limitStateInfo.bEnableCoreLimit)
            {
                _serviceLimitResource->addExcStopRecord();
            }
        }
    }
    catch(exception &ex)
    {
        NODE_LOG(_serverId)->error()<<FILE_FUN << "ex:" << ex.what() << endl;
	    NODE_LOG("KeepAliveThread")->error()<<FILE_FUN << "ex:" << ex.what() << endl;
    }
    catch(...)
    {
        NODE_LOG(_serverId)->error()<<FILE_FUN << "unknown ex." << endl;
	    NODE_LOG("KeepAliveThread")->error()<<FILE_FUN << "unknown ex." << endl;
    }
}

/**
 * 属性上报单独出来
 */
void ServerObject::reportMemProperty()
{
    try
    {
        char sTmp[64] ={0};
        snprintf(sTmp,sizeof(sTmp), "%llu", _pid);
        string spid = string(sTmp);

        string filename = "/proc/" + spid + "/statm";
        string stream;
        stream = TC_File::load2str(filename);
        if(!stream.empty())
        {
            NODE_LOG("ReportMemThread")->debug()<<FILE_FUN<<"filename:"<<filename<<",stream:"<<stream<<endl;
            //>>改成上报物理内存
            vector<string> vtStatm = TC_Common::sepstr<string>(stream, " ");
            if (vtStatm.size() < 2)
            {
                NODE_LOG("ReportMemThread")->error() <<FILE_FUN<< "cannot get server[" + _serverId  + "] physical mem size|stream|" << stream  << endl;
            }
            else
            {
                stream = vtStatm[1];
            }

            if(TC_Common::isdigit(stream))
            {
                REPORT_MAX(_serverId, _serverId+".memsize", TC_Common::strto<int>(stream) * 4);
                NODE_LOG("ReportMemThread")->debug()<<FILE_FUN<<"report_max("<<_serverId<<".memsize,"<<TC_Common::strto<int>(stream)*4<<")OK."<<endl;
                return;
            }
            else
            {
                NODE_LOG("ReportMemThread")->error()<<FILE_FUN<<"stream : "<<stream<<" ,is not a digit."<<endl;
            }
       }
       else
       {
           NODE_LOG("ReportMemThread")->error()<<FILE_FUN<<"stream is empty: "<<stream<<endl;
       }
    }
    catch(exception &ex)
    {
        NODE_LOG("ReportMemThread")->error() << FILE_FUN << " ex:" << ex.what() << endl;
        TLOGERROR(FILE_FUN << " ex:" << ex.what() << endl);
    }
    catch(...)
    {
        NODE_LOG("ReportMemThread")->error() << FILE_FUN << " unknown error" << endl;
        TLOGERROR(FILE_FUN << "unknown ex." << endl);
    }
}

void ServerObject::checkCoredumpLimit()
{
    if(_state != ServerObject::Active)
    {
        NODE_LOG(_serverId)->debug() << FILE_FUN << getServerId() <<", server is inactive"<<endl;
        return;
    }

	if(!_limitStateInfo.bEnableCoreLimit)
	{
		// NODE_LOG(_serverId)->debug() << FILE_FUN << getServerId() <<", server is disable corelimit"<<endl;
		return;
	}

    Lock lock(*this);
    if(_limitStateInfo.eCoreType == EM_AUTO_LIMIT)
    {
        bool bNeedClose=false;
        int iRet =_serviceLimitResource->IsCoreLimitNeedClose(bNeedClose);

        bool bNeedUpdate = (iRet==2)?true:false;

        if(_limitStateUpdated && bNeedClose)
        {
            _limitStateUpdated = setServerCoreLimit(true)?false:true;//设置成功后再屏蔽更新
            _limitStateInfo.bCloseCore = bNeedClose;
        }
        else if(bNeedUpdate && !bNeedClose)
        {
            setServerCoreLimit(false);
            _limitStateInfo.bCloseCore = bNeedClose;
        }
    }
    else
    {
        if(_limitStateUpdated)
        {
            _limitStateUpdated = setServerCoreLimit(_limitStateInfo.bCloseCore)?false:true; //设置成功后再屏蔽更新
        }
    }
}

bool ServerObject::setServerCoreLimit(bool bCloseCore)
{
    string sResult = "succ";
    const string sCmd = (bCloseCore)?("tars.closecore yes"):("tars.closecore no");

    CommandNotify command(this,sCmd);
    int iRet = command.doProcess(sResult);

    NODE_LOG(_serverId)->debug()<<FILE_FUN<<"setServerCoreLimit|"<<_serverId<<"|"<<sCmd<<"|"<<sResult<<endl;

    return (iRet==0);

}

//服务重启时设置
void ServerObject::setLimitInfoUpdate(bool bUpdate)
{
    Lock lock(*this);

    _limitStateUpdated = true;
}

//手动配置有更新时调用
void ServerObject::setServerLimitInfo(const ServerLimitInfo& tInfo)
{
    Lock lock(*this);

    _limitStateInfo = tInfo;

    NODE_LOG(_serverId)->debug() << FILE_FUN << _serverId << "|" << _limitStateInfo.str() << endl;

    _serviceLimitResource->setLimitCheckInfo(tInfo.iMaxExcStopCount,tInfo.iCoreLimitTimeInterval,tInfo.iCoreLimitExpiredTime);

    _activatorPtr->setLimitInfo(tInfo.iActivatorTimeInterval, tInfo.iActivatorMaxCount, tInfo.iActivatorPunishInterval);

    _limitStateUpdated = true;
}

//重置core计数信息
void ServerObject::resetCoreInfo()
{
    _serviceLimitResource->resetRunTimeData();
}

void ServerObject::setStarted(bool bStarted)
{
	Lock lock(*this);
	_started=bStarted;
}

void ServerObject::setStartTime(int64_t iStartTime)
{
	Lock lock(*this);
	_startTime = iStartTime;
}

void ServerObject::onUpdateServerResult(int result)
{
    if(result < 0)
    {
        _noticed = false;
        _noticeFailTimes ++;

        TLOGERROR("Update server:"<<_application<<"."<<_serverName<<" failed, error times:"<<_noticeFailTimes<<", result:"<<result<< endl);
    }
    else
    {
        _noticed = true;
        _noticeFailTimes = 0;

        TLOGDEBUG("Update server: "<<_application<<"."<<_serverName<<" ok"<<endl);
    }
}

void ServerObject::callback_updateServer(tars::Int32 ret)
{
    onUpdateServerResult(ret);
}

void ServerObject::callback_updateServer_exception(tars::Int32 ret)
{
    onUpdateServerResult(ret == 0 ? -1 : ret);
}
