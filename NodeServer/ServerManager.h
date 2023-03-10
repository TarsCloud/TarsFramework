//
// Created by jarod on 2022/5/7.
//

#ifndef FRAMEWORK_SERVERMANAGER_H
#define FRAMEWORK_SERVERMANAGER_H

#include "AdminReg.h"
#include "util/tc_thread.h"
#include "util/tc_singleton.h"

using namespace tars;

class ServerManager : public TC_Thread ,public TC_Singleton<ServerManager>
{
public:
	/**
	 *
	 * @param adminObj
	 */
	void initialize(const string &adminObj);

	/**
	 *
	 */
	void terminate();

	/**
	 * 增强的发布接口
	 * pushRequest 插入发布请求到队列
	 * @param req  发布请求
	 * @return  int 0成功 其它失败
	 */
	int patchPro(const PatchRequest & req, string & result);

	/**
	 * 关闭node
	 * @return  int
	 */
	int shutdown( string &resul);

	/**
	 * 载入指定服务
	 * @param application    服务所属应用名
	 * @param serverName  服务名
	 * @return  int
	 */
	int loadServer( const string& application, const string& serverName, string &result);

	/**
	 * 删除服务
	 * @param application    服务所属应用名
	 * @param serverName  服务名
	 * @return  int
	 */
	int destroyServer( const string& application, const string& serverName, string &result) ;

	/**
	 * 启动指定服务
	 * @param application    服务所属应用名
	 * @param serverName  服务名
	 * @return  int
	 */
	int startServer( const string& application, const string& serverName, string &result) ;

	/**
	 * 停止指定服务
	 * @param application    服务所属应用名
	 * @param serverName  服务名
	 * @return  int
	 */
	int stopServer( const string& application, const string& serverName, string &result) ;

	/**
	 * 通知服务
	 * @param application
	 * @param serverName
	 * @param result
	 * @param current
	 *
	 * @return int
	 */
	int notifyServer( const string& application, const string& serverName, const string &command, string &result);

	/**
	 * 获取指定服务在node的信息
	 * @param application    服务所属应用名
	 * @param serverName  服务名
	 * @return  ServerState
	 */
	int getStateInfo(const std::string & application,const std::string & serverName,ServerStateInfo &info,std::string &result);

	/**
	 * 发布服务进度
	 * @param application  服务所属应用名
	 * @param serverName  服务名
	 * @out tPatchInfo  下载信息
	 * @return  int
	 */
	int getPatchPercent( const string& application, const string& serverName, PatchInfo &tPatchInfo);

	/**
	 *
	 * @param sFullCacheName
	 * @param sBackupPath
	 * @param sKey
	 * @return
	 */
	int delCache(const std::string& sFullCacheName,  const std::string& sBackupPath,  const std::string& sKey, std::string &result);

	/**
	* 列举某个app下面某个服务的日志文件列表，以文件最后修改时间倒排序
	*/
	int getLogFileList(const string& application, const string& serverName, vector<string>& logFileList);

	/**
	* 获取某个日志文件的内容，cmd表示参数， 比如 tail -1000 | grep xxx
	*/
	int getLogData(const string& application, const string& serverName,const string& logFile, const string& cmd, string& fileData);

	/**
	 * 获取节点的负载
	 * @param application
	 * @param serverName
	 * @param pid
	 * @param fileData
	 * @param current
	 * @return
	 */
	int getNodeLoad(const string& application, const string& serverName, int pid, string& fileData);

	/**
	 * 强制docker做一次login
	 * @return
	 */
	int forceDockerLogin(vector<string> &result);

protected:
	virtual void run();

	void createAdminPrx();
protected:
	bool _terminate = false;
	std::mutex _mutex;
	std::condition_variable _cond;
	string _adminObj;
	//AdminRegPrx _adminPrx;
    map<string, AdminRegPrx> _adminPrxs;
};


#endif //FRAMEWORK_SERVERMANAGER_H
