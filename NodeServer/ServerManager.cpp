//
// Created by jarod on 2022/5/7.
//

#include "ServerManager.h"
#include "servant/Application.h"
#include "NodePush.h"
#include "ServerObject.h"
#include "ServerFactory.h"
#include "CommandDestroy.h"
#include "CommandStart.h"
#include "CommandStop.h"
#include "CommandPatch.h"
#include "CommandNotify.h"
#include "NodeServer.h"

extern BatchPatch * g_BatchPatchThread;

class AdminNodePushPrxCallback : public NodePushPrxCallback
{
public:
	AdminNodePushPrxCallback(AdminRegPrx &prx) : _adminPrx(prx)
	{
	}

	virtual void callback_forceDockerLogin(tars::Int32 requestId, const string &nodeName)
	{
		vector<string> result;

		int ret = ServerManager::getInstance()->forceDockerLogin(result);

		TLOG_DEBUG("requestId:" << requestId << ", ret:" << ret << endl);

		TarsOutputStream<BufferWriterString> os;
		os.write(result, 0);

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, os.getByteBuffer());
	}

	virtual void callback_getLogData(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName,  const std::string& logFile,  const std::string& cmd)
	{
		string result;

		int ret = ServerManager::getInstance()->getLogData(application, serverName, logFile, cmd, result);

		NODE_LOG(application + "." + serverName)->debug() << "requestId:" << requestId << ", " << logFile << ", " << cmd << ", ret:" << ret << ", " << result << endl;

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

	virtual void callback_getLogFileList(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName)
	{
		vector<string> result;

		int ret = ServerManager::getInstance()->getLogFileList(application, serverName, result);

		NODE_LOG(application + "." + serverName)->debug() << "requestId:" << requestId << ", ret:" << ret << endl;

		TarsOutputStream<BufferWriterString> os;
		os.write(result, 0);

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, os.getByteBuffer());
	}

	virtual void callback_getNodeLoad(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName, tars::Int32 pid)
	{
		string result;

		int ret = ServerManager::getInstance()->getNodeLoad(application, serverName, pid, result);

		NODE_LOG(application + "." + serverName)->debug() << "requestId:" << requestId << ", ret:" << ret << ", " << result << endl;

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

	virtual void callback_getPatchPercent(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName)
	{
		PatchInfo info;

		int ret = ServerManager::getInstance()->getPatchPercent(application, serverName, info);

		NODE_LOG(application + "." + serverName)->debug() << "requestId:" << requestId << ", " << info.writeToJsonString() << endl;

		TarsOutputStream<BufferWriterString> os;
		info.writeTo(os);

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__ ,  ret, os.getByteBuffer());
	}

	virtual void callback_getStateInfo(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName)
	{
		ServerStateInfo info;
		string result;

		int ret = ServerManager::getInstance()->getStateInfo(application, serverName, info, result);

		NODE_LOG(application + "." + serverName)->debug() << "requestId:" << requestId << ", " << info.writeToJsonString() << endl;

		TarsOutputStream<BufferWriterString> os;
		os.write(info, 0);
		os.write(result, 1);

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, os.getByteBuffer());
	}

	virtual void callback_loadServer(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName)
	{
		string result;

		int ret = ServerManager::getInstance()->loadServer(application, serverName, result);

		NODE_LOG(application + "." + serverName)->debug() << "requestId:" << requestId << ", " << result << endl;

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

	virtual void callback_notifyServer(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName,  const std::string& command)
	{
		string result;

		int ret = ServerManager::getInstance()->notifyServer(application, serverName, command, result);

		NODE_LOG(application + "." + serverName)->debug() << "requestId:" << requestId << ", " << command << ", " << result << endl;

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

	virtual void callback_patchPro(tars::Int32 requestId, const string &nodeName,  const tars::PatchRequest& req)
	{
		string result;

		int ret = ServerManager::getInstance()->patchPro(req, result);

		NODE_LOG(req.appname+ "." + req.servername)->debug() << "requestId:" << requestId << ", " << result << endl;

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

	virtual void callback_ping(tars::Int32 requestId, const string &nodeName)
	{
		TLOG_DEBUG("requestId:" << requestId << endl);
		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, 0, "ping succ");
	}

	virtual void callback_shutdown(tars::Int32 requestId, const string &nodeName)
	{
		string result;

		int ret = ServerManager::getInstance()->shutdown(result);

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

	virtual void callback_startServer(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName)
	{
		string result;

		int ret = ServerManager::getInstance()->startServer(application, serverName, result);

		NODE_LOG(application + "." + serverName)->debug() << result << endl;

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

	virtual void callback_stopServer(tars::Int32 requestId, const string &nodeName,  const std::string& application,  const std::string& serverName)
	{
		string result;

		int ret = ServerManager::getInstance()->stopServer(application, serverName, result);

		NODE_LOG(application + "." + serverName)->debug() << result << endl;

		_adminPrx->async_reportResult(NULL, requestId, __FUNCTION__, ret, result);
	}

protected:
	AdminRegPrx _adminPrx;
};

void ServerManager::terminate()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_terminate = true;
	_cond.notify_one();
}

void ServerManager::initialize(const string &adminObj)
{
	_adminObj = adminObj;

	createAdminPrx();

	start();
}

void ServerManager::createAdminPrx()
{
	AdminRegPrx prx = Application::getCommunicator()->stringToProxy<AdminRegPrx>(_adminObj);

	NodePushPrxCallbackPtr callback = new AdminNodePushPrxCallback(prx);

	prx->tars_set_push_callback(callback);

	_adminPrx = prx;
}

void ServerManager::run()
{
	ReportNode rn;
	PlatformInfo platformInfo;
	rn.nodeName = platformInfo.getNodeName();
	rn.sid = TC_UUIDGenerator::getInstance()->genID();

	int timeout = 10000;

	while(!_terminate)
	{
		time_t now = TNOWMS;

		try
		{
			_adminPrx->tars_set_timeout(timeout/2)->tars_hash(tars::hash<string>()(rn.nodeName))->reportNode(rn);
		}
		catch(exception &ex)
		{
			TLOG_ERROR("report admin, error:" << ex.what() << endl);
		}

		int64_t diff = timeout-(TNOWMS-now);

		if(diff > 0 )
		{
			std::unique_lock<std::mutex> lock(_mutex);

			if(diff > timeout)
			{
				diff = timeout;
			}
			_cond.wait_for(lock, std::chrono::milliseconds(diff));
		}
	}
}

int ServerManager::patchPro(const PatchRequest & req, string & result)
{
	TLOG_DEBUG(req.appname << "." << req.servername << endl);
	string serverId = req.appname + "." + req.servername;

	NODE_LOG(serverId)->debug() << FILE_FUN
								<< serverId + "_" + req.nodename << "|"
								<< req.groupname   << "|"
								<< req.version     << "|"
								<< req.user        << "|"
								<< req.servertype  << "|"
								<< req.patchobj    << "|"
								<< req.md5            << "|"
								<< req.ostype      << endl;

	try
	{
		//加载服务信息
		string sError("");
		ServerObjectPtr server = ServerFactory::getInstance()->loadServer(req.appname, req.servername, true, sError);
		if (!server)
		{
			result =FILE_FUN_STR+ "load " + serverId + "|" + sError;
			NODE_LOG(serverId)->error() << FILE_FUN << serverId + "_" + req.nodename << "|" << result << endl;
			if(sError.find("server state") != string::npos)
				return EM_TARS_SERVICE_STATE_ERR;
			else
				return EM_TARS_LOAD_SERVICE_DESC_ERR;
		}

		{
			TC_ThreadRecLock::Lock lock(*server);
			ServerObject::InternalServerState  eState = server->getInternalState();

			//去掉启动中不能发布的限制
			if (server->toStringState(eState).find("ing") != string::npos && eState != ServerObject::Activating)
			{
				result = FILE_FUN_STR+"cannot patch the server ,the server state is " + server->toStringState(eState);
				NODE_LOG(serverId)->error() <<FILE_FUN<< serverId << "|" << result << endl;
				return EM_TARS_SERVICE_STATE_ERR;
			}

			if(req.md5 == "")
			{
				result = FILE_FUN_STR+"parameter error, md5 not setted.";
				NODE_LOG(serverId)->error() <<FILE_FUN<< serverId << "|" << result << endl;
				return EM_TARS_PARAMETER_ERR;
			}

			if(CommandPatch::checkOsType())
			{
				string osType = CommandPatch::getOsType();
				if(req.ostype != "" && req.ostype != osType)
				{
					result = FILE_FUN_STR+"parameter error, req.ostype=" + req.ostype + ", local osType:" + osType;
					NODE_LOG(serverId)->error() <<FILE_FUN<< serverId << "|" << result << endl;
					return EM_TARS_PARAMETER_ERR;
				}
			}

			//保存patching前的状态，patch完成后恢复
			NODE_LOG(serverId)->debug() <<FILE_FUN<<serverId + "_" + req.nodename << "|saved state:" << server->toStringState(eState) << endl;
			server->setLastState(eState);
			server->setState(ServerObject::BatchPatching);
			//将百分比初始化为0，防止上次patch结果影响本次patch进度
			server->setPatchPercent(0);

			NODE_LOG(serverId)->debug()<<FILE_FUN << serverId + "_" + req.nodename << "|" << req.groupname   << "|preset success" << endl;
		}

		g_BatchPatchThread->push_back(req,server);
	}
	catch (std::exception & ex)
	{
		NODE_LOG(serverId)->error() << FILE_FUN << serverId + "_" + req.nodename << "|Exception:" << ex.what() << endl;

		result = FILE_FUN_STR+ex.what();

		if(result.find("reduplicate") != string::npos)
		{
			return EM_TARS_REQ_ALREADY_ERR;
		}

		return EM_TARS_UNKNOWN_ERR;
	}
	catch (...)
	{
		NODE_LOG(serverId)->error() << FILE_FUN << serverId + "_" + req.nodename << "|Unknown Exception" << endl;
		result = FILE_FUN_STR+"Unknown Exception";
		return EM_TARS_UNKNOWN_ERR;
	}

	NODE_LOG(serverId)->debug() <<FILE_FUN
								<< serverId + "_" + req.nodename << "|"
								<< req.groupname   << "|"
								<< req.version     << "|"
								<< req.user        << "|"
								<< req.servertype  << "|return success"<< endl;

	return EM_TARS_SUCCESS;
}

int ServerManager::shutdown( string &result)
{
	TLOG_DEBUG("" << endl);

	g_app.terminate();

	return 0;
}

int ServerManager::loadServer( const string& application, const string& serverName, string &result)
{
	string serverId = application + "." + serverName;

	string s;

	ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->loadServer(application,serverName,false,s);
	if (!pServerObjectPtr)
	{
		result =result+" error::cannot load server description.\n"+s;
		NODE_LOG(serverId)->error() << FILE_FUN << result << endl;

		return EM_TARS_LOAD_SERVICE_DESC_ERR;
	}
	result = result + " succ" + s;

	NODE_LOG(serverId)->debug() << "NodeImp::loadServer"<<result<<endl;

	return 0;
}

int ServerManager::startServer( const string& application, const string& serverName, string &result)
{
	string serverId = application + "." + serverName;
	int iRet = EM_TARS_UNKNOWN_ERR;

	try
	{
		string s;

		ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->loadServer(application, serverName, true, s);

		if (pServerObjectPtr)
		{
			bool bByNode = true;

			CommandStart command(pServerObjectPtr, bByNode);
			iRet = command.doProcess(s);

			if (iRet == 0)
			{
				result = "server is activating, please check: " + s;
			}
			else
			{
				result = "error:" + s;
			}
			NODE_LOG(serverId)->debug() << FILE_FUN << result << endl;
			return iRet;
		}

		//设置服务状态enable
		pServerObjectPtr = ServerFactory::getInstance()->getServer(application, serverName);
		if (pServerObjectPtr)
		{
			pServerObjectPtr->setEnabled(true);
		}

		result += "error::cannot load server description from registry.\n" + s;
		iRet = EM_TARS_LOAD_SERVICE_DESC_ERR;
	}
	catch ( exception& e )
	{
		NODE_LOG(serverId)->error() << FILE_FUN << " catch exception:" << e.what() << endl;
		result += e.what();
	}
	catch ( ... )
	{
		NODE_LOG(serverId)->error() << FILE_FUN <<" catch unkown exception" << endl;
		result += "catch unkown exception";
	}
	return iRet;
}

int ServerManager::stopServer( const string& application, const string& serverName, string &result)
{
	string serverId = application + "." + serverName;

	int iRet = EM_TARS_UNKNOWN_ERR;
	try
	{
		NODE_LOG(serverId)->debug() <<FILE_FUN<<result << endl;

		ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );

		if (pServerObjectPtr)
		{
			NODE_LOG(serverId)->debug() << FILE_FUN << " server exists to stop server" << endl;

			string s;
			bool bByNode = true;
			CommandStop command(pServerObjectPtr,true, bByNode);
			iRet = command.doProcess(s);
			if (iRet == 0 )
			{
				result = result+"succ:"+s;
			}
			else
			{
				result = result+"error:"+s;
			}
			NODE_LOG(serverId)->debug()<<FILE_FUN <<result << endl;
		}
		//可能改变了服务设置状态, 重新加载一次!
		pServerObjectPtr = ServerFactory::getInstance()->loadServer(application, serverName, false, result);

		if(!pServerObjectPtr)
		{
			result += "server is  not exist";
			iRet = EM_TARS_LOAD_SERVICE_DESC_ERR;
		}

		return iRet;
	}
	catch ( exception& e )
	{
		NODE_LOG(serverId)->error() << FILE_FUN<<" catch exception :" << e.what() << endl;
		result += e.what();
	}
	catch ( ... )
	{
		NODE_LOG(serverId)->error() << FILE_FUN<<" catch unkown exception" << endl;
		result += "catch unkown exception";
	}
	return EM_TARS_UNKNOWN_ERR;
}

int ServerManager::notifyServer( const string& application, const string& serverName, const string &message, string &result)
{
	string serverId = application + "." + serverName;

	try
	{
		if ( application == "tars" && serverName == "tarsnode" )
		{
			AdminFPrx pAdminPrx;    //服务管理代理
			pAdminPrx = Application::getCommunicator()->stringToProxy<AdminFPrx>("AdminObj@"+ServerConfig::Local);
			result = pAdminPrx->notify(message);
			return 0;
		}

		ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
		if ( pServerObjectPtr )
		{
			string s;
			CommandNotify command(pServerObjectPtr,message);
			int iRet = command.doProcess(s);
			if (iRet == 0 )
			{
				result = result + "succ:" + s;
			}
			else
			{
				result = result + "error:" + s;
			}
			return iRet;
		}

		result += "server is not exist";
	}
	catch ( exception& e )
	{
		NODE_LOG(serverId)->error() << "NodeImp::notifyServer catch exception :" << e.what() << endl;
		result += e.what();
	}
	catch ( ... )
	{
		NODE_LOG(serverId)->error() <<  "NodeImp::notifyServer catch unkown exception" << endl;
		result += "catch unkown exception";
	}
	return -1;
}

int ServerManager::getStateInfo(const std::string & application,const std::string & serverName,ServerStateInfo &info,std::string &result)
{
	string serverId = application + "." + serverName;

	result = string(__FUNCTION__)+" ["+application + "." + serverName+"] ";
	ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
	if ( pServerObjectPtr )
	{
		result += "succ";

		info.application = application;
		info.serverName = serverName;
		info.serverState = pServerObjectPtr->getState();
		info.processId = pServerObjectPtr->getPid();
		info.settingState = pServerObjectPtr->isEnabled()==true?Active:Inactive;

		NODE_LOG(serverId)->debug() << "NodeImp::getStateInfo " << result << endl;

		return EM_TARS_SUCCESS;
	}

	result += "server not exist";
	NODE_LOG(serverId)->error() << "NodeImp::getStateInfo " << result << endl;

	info.serverState = Inactive;
	info.processId = -1;
	info.settingState = Inactive;

	return EM_TARS_UNKNOWN_ERR;
}

int ServerManager::getPatchPercent( const string& application, const string& serverName, PatchInfo &tPatchInfo)
{
	string serverId = application + "." + serverName;

	string &result =  tPatchInfo.sResult;
	try
	{
		result = string(__FUNCTION__)+" ["+application + "." + serverName+"] ";

		ServerObjectPtr pServerObjectPtr = ServerFactory::getInstance()->getServer( application, serverName );
		if ( pServerObjectPtr )
		{
			result += "succ";
			NODE_LOG(serverId)->debug()<<FILE_FUN << tPatchInfo.writeToJsonString() << endl;
			return pServerObjectPtr->getPatchPercent(tPatchInfo);
		}

		result += "server not exist";
		NODE_LOG(serverId)->error() << FILE_FUN <<" "<< result<< endl;
		return EM_TARS_LOAD_SERVICE_DESC_ERR;
	}
	catch ( exception& e )
	{
		NODE_LOG(serverId)->error() << FILE_FUN << " catch exception :" << e.what() << endl;
		result += e.what();
	}
	catch ( ... )
	{
		NODE_LOG(serverId)->error() << FILE_FUN <<" catch unkown exception" << endl;
		result += "catch unkown exception";
	}

	return EM_TARS_UNKNOWN_ERR;
}

int ServerManager::getLogFileList(const string& application, const string& serverName, vector<string>& logFileList)
{
	string serverId = application + "." + serverName;

	string logPath = ServerConfig::LogPath + FILE_SEP + application + FILE_SEP + serverName + FILE_SEP;

	NODE_LOG(serverId)->debug() << "logpath:" << logPath << endl;

	vector<string> logFileListTmp;
	TC_File::listDirectory(logPath, logFileListTmp, false);

	for (size_t i = 0; i < logFileListTmp.size(); ++i)
	{
		string fileName = TC_File::extractFileName(logFileListTmp[i]);
		if(fileName.length() >= 5 && fileName.find(".log") != string::npos)
		{
			logFileList.push_back(fileName);
		}
	}
	return 0;
}

int ServerManager::getLogData(const string& application, const string& serverName,const string& logFile, const string& cmd, string& fileData)
{
	string serverId = application + "." + serverName;

	string filePath = ServerConfig::LogPath + FILE_SEP + application + FILE_SEP + serverName + FILE_SEP + logFile;
	if (!TC_File::isFileExistEx(filePath))
	{
		NODE_LOG(serverId)->debug() << "The log file: " << filePath << " is not exist" << endl;
		return -1;
	}

	string newCmd = TC_Common::replace(cmd, "__log_file_name__", filePath);

	NODE_LOG(serverId)->debug() << "[NodeImp::getLogData] cmd:" << newCmd << endl;

#if TARGET_PLATFORM_WINDOWS
	string tmpCmd;
    vector<string> subCmd = TC_Common::sepstr<string>(newCmd, "|");
    for(size_t i = 0; i < subCmd.size(); i++)
    {
        tmpCmd += ServerConfig::TarsPath + FILE_SEP + "tarsnode" + FILE_SEP + "util\\busybox.exe " + subCmd[i];
        if(i != subCmd.size() - 1)
        {
            tmpCmd += " | ";
        }
    }

    newCmd = tmpCmd;
#endif

	NODE_LOG(serverId)->debug() << "[NodeImp::getLogData] newcmd:" << newCmd << endl;

	string errstr;
	fileData = TC_Port::exec(newCmd.c_str(), errstr);
	if (fileData.empty()) {
		if (!errstr.empty()) {
			fileData = errstr;
			NODE_LOG(serverId)->error() << errstr << endl;
		}
	}

	return 0;
}

int ServerManager::getNodeLoad(const string& application, const string& serverName, int pid, string& fileData)
{
	string serverId = application + "." + serverName;

	NODE_LOG(serverId)->debug() << serverId << ", pid:" << pid << endl;
#if TARGET_PLATFORM_WINDOWS
	fileData = "not support!";
#else
	string cmd = "top -c -bw 160 -n 1 -o '%CPU' -o '%MEM'";

	string errstr;
	fileData = TC_Port::exec(cmd.c_str(), errstr);
	if (fileData.empty()) {
		if (!errstr.empty()) {
			fileData = errstr;
			NODE_LOG(serverId)->error() << errstr << endl;
		}
	}

	fileData += "#global-top-end#";

	if (pid > 0)
	{
		cmd = "top -b -n 1 -o '%CPU' -o '%MEM' -H -p " + TC_Common::tostr(pid);
		fileData += "\n\n";
		fileData += "#this-top-begin#" + string(100, '-');
		fileData += "\n";

		errstr.clear();
		fileData += TC_Port::exec(cmd.c_str(), errstr);
		if (fileData.empty()) {
			if (!errstr.empty()) {
				fileData += errstr;
				NODE_LOG(serverId)->error() << errstr << endl;
			}
		}
		// fileData += string(buf);
		fileData += "#this-top-end#";
	}

	// memset(buf, 0, sizeof(buf));
	cmd = "cd " + ServerConfig::TarsPath + FILE_SEP + "app_log; ls -alth core.*";
	fileData += "\n\n";
	fileData += "#core-file-begin#" + string(100, '-');
	fileData += "\n";

	errstr.clear();
	fileData = TC_Port::exec(cmd.c_str(), errstr);
	if (fileData.empty()) {
		if (!errstr.empty()) {
			fileData = errstr;
			NODE_LOG(serverId)->error() << errstr << endl;
		}
	}
	NODE_LOG(serverId)->debug() << fileData << endl;
#endif

	return 0;
}

int ServerManager::forceDockerLogin(vector<string> &result)
{
	TLOG_DEBUG("" << endl);
	result = g_app.getDockerPullThread()->checkDockerRegistries();

	return 0;
}