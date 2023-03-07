﻿/**
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

#ifndef __AMIN_REGISTRY_H__
#define __AMIN_REGISTRY_H__

#include "util/tc_common.h"
#include "AdminReg.h"
#include "Registry.h"
#include "Patch.h"
#include "EndpointF.h"
#include "DbProxy.h"

using namespace tars;

struct PrepareInfo
{
	string application;
	string serverName;
	string patchId;
	string runType;
	string baseImage;
	string patchFile;
	string md5;

	const string key()
	{
		return application + "." + serverName + "." + patchId + "." + runType + "." + baseImage;
	}
};

/**
 * 管理控制接口类
 */
class AdminRegistryImp: public AdminReg
{
public:
    /**
     * 构造函数
     */
    AdminRegistryImp(){};

    /**
     * 初始化
     */
    virtual void initialize();

    /**
     ** 退出
     */
    virtual void destroy() {};

	/**
	 * 连接关闭
	 * @param current
	 * @return
	 */
	virtual int doClose(CurrentPtr current);

public:

	/**
	 * 上报
	 * @param nodeName
	 * @param current
	 * @return
	 */
	virtual int reportNode(const ReportNode &rn, CurrentPtr current);

	/**
	 * 下线(长连接, 有代理的模式下, 代理主动通知)
	 * @param rn
	 * @param current
	 * @return
	 */
	virtual int deleteNode(const ReportNode &rn, CurrentPtr current);

	/**
	 * 上报结果
	 * @param funcName
	 * @param result
	 * @param current
	 * @return
	 */
	virtual int reportResult(int requestId, const string &funcName, int ret, const string &result, CurrentPtr current);

	/**
     * 
     * @param taskList 
     * 
     * @return string, TaskNo
     */
   virtual int addTaskReq(const TaskReq &taskReq, CurrentPtr current);

    /**
     * 获取任务状态
     *
     * @param taskNo : 任务列表id
     *
     * @return 任务状态
     */
    virtual int getTaskRsp(const string &taskNo, TaskRsp &taskRsp, CurrentPtr current);

    /**
     * 获取TaskRsp信息
     * 
     * @param application 
     * @param serverName 
     * @param command 
     * @param current 
     * 
     * @return vector<TaskRsp> 
     */
    virtual int getTaskHistory(const string & application, const string & serverName, const string & command, vector<TaskRsp> &taskRsp, CurrentPtr current);

	/**
     * 设置任务状态
     * 
     * @param itemNo 
     * @param startTime 
     * @param endTime 
     * @param status 
     * @param log 
     * @param current 
     * 
     * @return int 
     */
    virtual int setTaskItemInfo(const string & itemNo, const map<string, string> &info, CurrentPtr current);
	virtual int setTaskItemInfo_inner(const string & itemNo, const map<string, string> &info);

    /***********application****************/
    /**
     * 卸载服务
     * 
     * @param application 
     * @param serverName 
     * @param nodeName 
     * @param current 
     * 
     * @return int 
     */
    virtual int undeploy(const string & application, const string & serverName, const string & nodeName, const string &user, string &log, CurrentPtr current);
	virtual int undeploy_inner(const string & application, const string & serverName, const string & nodeName, const string &user, string &log);

    /**
     * 获取application列表
     *
     * @param null
     * @param out result : 结果描述
     *
     * @return application列表
     */
    virtual vector<string> getAllApplicationNames(string &result, CurrentPtr current);

    /**
     * 获取node列表
     *
     * @param null
     * @param out result : 结果描述
     *
     * @return node 列表
     */
    virtual vector<string> getAllNodeNames(string &result, CurrentPtr current);

    /**
     * 获取node版本
     * @param name   node名称
     * @param version   node版本
     * @param out result 结果描述
     * @return  0-成功 others-失败
     */
    virtual int getNodeVesion(const string &nodeName, string &version, string & result, CurrentPtr current);

    /**
     * ping node
     *
     * @param name: node id
     * @param out result : 结果描述
     *
     * @return : true-ping通；false-不通
     */
    virtual bool pingNode(const string & name, string &result, CurrentPtr current);

    /**
     * 停止 node
     *
     * @param name: node id
     * @param out result : 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int shutdownNode(const string & name, string &result, CurrentPtr current);

    /**
     * 获取server列表
     *
     * @param name: null
     * @param out result : 结果描述
     *
     * @return: server列表及相关信息
     */
    virtual vector<vector<string> > getAllServerIds(string &result, CurrentPtr current);

    /**
     * 获取特定server状态
     *
     * @param application: 应用
     * @param serverName : server名
     * @param nodeNmae   : node id
     * @param out state  : 状态
     * @param out result : 结果描述
     *
     * @return : 处理结果
     */
    virtual int getServerState(const string & application, const string & serverName, const string & nodeName, ServerStateDesc &state, string &result, CurrentPtr current);

     /**
     * 获取特定ip所属group
     *
     * @param sting: ip
     * @param out int  : group id
     * @param out result : 结果描述
     *
     * @return : 处理结果
     */

    virtual int getGroupId(const string & ip,int &groupId, string &result, CurrentPtr current);

	/**
	 *
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param result
	 * @param current
	 * @return
	 */
	virtual int destroyServer(const string & application, const string & serverName, const string & nodeName,
			string &result, CurrentPtr current);
    /**
     * 启动特定server
     *
     * @param application: 应用
     * @param serverName : server名
     * @param nodeName   : node id
     * @param out result : 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int startServer(const string & application, const string & serverName, const string & nodeName,
            string &result, CurrentPtr current);
	virtual int startServer_inner(const string & application, const string & serverName, const string & nodeName,
		string &result);

    /**
     * 停止特定server
     *
     * @param application: 应用
     * @param serverName : server名
     * @param nodeName   : node id
     * @param out result : 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int stopServer(const string & application, const string & serverName, const string & nodeName,
            string &result, CurrentPtr current);
	virtual int stopServer_inner(const string & application, const string & serverName, const string & nodeName,
		string &result);

    /**
     * 重启特定server
     *
     * @param application: 应用
     * @param serverName : server名
     * @param nodeName   : node id
     * @param out result : 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int restartServer(const string & application, const string & serverName, const string & nodeName,
            string &result, CurrentPtr current);
	virtual int restartServer_inner(const string & application, const string & serverName, const string & nodeName,
		string &result);
    /**
     * 通知服务
     * @param application
     * @param serverName
     * @param nodeName
     * @param command
     * @param result
     * @param current
     *
     * @return int
     */
    virtual int notifyServer(const string & application, const string & serverName, const string & nodeName,
            const string &command, string &result, CurrentPtr current);
    virtual int notifyServer_inner(const string & application, const string & serverName, const string & nodeName,
    		const string &command, string &result);

    /**
     * 批量发布
     *
     * @param PatchRequest : 发布请求
     * @param out result   : 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int batchPatch(const PatchRequest & req, string &result, CurrentPtr current);
	virtual int prepareInfo_inner(PrepareInfo &pi, string &result);
	virtual int preparePatch_inner(const PrepareInfo &pi, string &result);
	virtual int batchPatch_inner(const PatchRequest & req, string &result);

    /**
     * 发布成功
     * 
     * @param req 
     * @param result 
     * @param current 
     * 
     * @return int 
     */
    virtual int updatePatchLog(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ, CurrentPtr current);
	virtual int updatePatchLog_inner(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ);

    /**
    * 获取服务发布进度
    * @param application  服务所属应用名
    * @param serverName  服务名
    * @param nodeName   :node id
    * @out tPatchInfo  :发布百分比
    * @return :0-成功 others-失败
    */
    virtual int getPatchPercent(const string &application, const string &serverName,const string & nodeName,
            PatchInfo &tPatchInfo, CurrentPtr current);
	virtual int getPatchPercent_inner(const string &application, const string &serverName, const string & nodeName,
		PatchInfo &tPatchInfo);

	/**
	 *
	 * @param sFullCacheName
	 * @param sBackupPath
	 * @param sKey
	 * @param result
	 * @param current
	 * @return
	 */
	virtual tars::Int32 delCache(const string &nodeName, const std::string & sFullCacheName, const std::string &sBackupPath, const std::string & sKey, std::string &result,CurrentPtr current);

    /**
     * 加载特定server
     *
     * @param application: 应用
     * @param serverName : server名
     * @param nodeName   : node id
     * @param out result : 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int loadServer(const string & application, const string & serverName, const string & nodeName, string &result, CurrentPtr current);

    /**
     * 获取相应模板
     *
     * @param profileName: 模板名称
     * @param out profileTemplate: 模板内容
     * @param out resultDesc: 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int getProfileTemplate(const std::string & profileName,std::string &profileTemplate, std::string & resultDesc, CurrentPtr current);

    /**
     * 获取务服相应模板
     *
     * @param application: 应用
     * @param serverName : server名
     * @param nodeName   : node id
     * @param out profileTemplate: 模板内容
     * @param out resultDesc: 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int getServerProfileTemplate(const string & application, const string & serverName, const string & nodeName,std::string &profileTemplate, std::string & resultDesc, CurrentPtr current);

    /**
     * node通过接口获取连接上主控的node ip
     * @param sNodeIp:  node 的ip
     *
     * @return 0-成功 others-失败
     */
    virtual int getClientIp(std::string &sClientIp,CurrentPtr current);

	/**
	 * 获取日志数据
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param logFile
	 * @param cmd
	 * @param fileData
	 * @param current
	 * @return
	 */
    virtual int getLogData(const std::string & application,const std::string & serverName,const std::string & nodeName,const std::string & logFile,const std::string & cmd,std::string &fileData,CurrentPtr current);

	/**
	 * 获取日志文件列表
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param logFileList
	 * @param current
	 * @return
	 */
    virtual int getLogFileList(const std::string & application,const std::string & serverName,const std::string & nodeName,vector<std::string> &logFileList,CurrentPtr current);

	/**
	 * 获取node的负载
	 * @param application
	 * @param serverName
	 * @param nodeName
	 * @param pid
	 * @param fileData
	 * @param current
	 * @return
	 */
	virtual int getNodeLoad(const string& application, const string& serverName, const std::string & nodeName, int pid, string& fileData, CurrentPtr current);

	/**
	 * 删除发布文件
	 * @param application
	 * @param serverName
	 * @param patchFile
	 * @param current
	 * @return
	 */
	virtual int deletePatchFile(const string &application, const string &serverName, const string & patchFile, CurrentPtr current);

	/**
	 * 获取框架所有的服务
	 * @param servers
	 * @param current
	 * @return
	 */
	virtual int getServers(vector<FrameworkServer> &servers, CurrentPtr current);

	/**
	 * 检查服务是否活着
	 * @param server
	 * @param current
	 * @return
	 */
	virtual int checkServer(const FrameworkServer &server, CurrentPtr current);

	/**
	 * 获取框架版本
	 * @param version
	 * @param current
	 * @return
	 */
    virtual int getVersion(string &version, CurrentPtr current);

	/**
	 * 更新服务的庄国泰
	 * @param application
	 * @param serverName
	 * @param nodeList
	 * @param bActive
	 * @param current
	 * @return
	 */
	virtual int updateServerFlowState(const string& application, const string& serverName, const vector<string>& nodeList, bool bActive, CurrentPtr current);

	/**
	 * 强制docker login
	 * @param nodeName
	 * @param current
	 * @return
	 */
	virtual int forceDockerLogin(const string &nodeName, vector<string> &result, CurrentPtr current);

	/**
	 * 检查主控是否可以登录docker仓库
	 * @param registry
	 * @param userName
	 * @param password
	 * @param rsp
	 * @param current
	 * @return
	 */
	virtual int checkDockerRegistry(const string & registry, const string & userName, const string & password, string &result, CurrentPtr current);

	/**
	 * 调用主控拉取镜像
	 * @param baseImageId
	 * @param current
	 * @return
	 */
	virtual int dockerPull(const string & baseImageId, CurrentPtr current);

	/**
	 *
	 * @param nodeNames
	 * @param heartbeats
	 * @param current
	 * @return
	 */
	virtual int getNodeList(const vector<string> &nodeNames, map<string, string> &heartbeats, CurrentPtr current);

    /**
     * 卸载服务
     *
     * @param application: 应用
     * @param serverName : server名
     * @param nodeName   : node id
     * @param out profileTemplate: 模板内容
     * @param out resultDesc: 结果描述
     *
     * @return : 0-成功 others-失败
     */
    virtual int uninstallServer(const string &application, const string &serverName, const string &nodeName, string &result, CurrentPtr current);


    /**
    * 是否有服务
    *
    * @param application: 服务基本信心 
    * @param serverName: 是否替换
    * @return : 返回值详见tarsErrCode枚举值
    */
    virtual int hasServer(const string &application, const string &serverName, bool &has, CurrentPtr current);

    /**
    * 新增服务
    *
    * @param conf: 服务基本信心 
    * @param replace: 是否替换()
    * @return : 返回值详见tarsErrCode枚举值
    */
    virtual int insertServerConf(const ServerConf &conf, bool replace, CurrentPtr current);

    /**
    * 新增adapter
    *
    * @param conf: 服务基本信心 
    * @param replace: 是否替换()
    * @return : 返回值详见tarsErrCode枚举值
    */
    virtual int insertAdapterConf(const string &sApplication, const string &sServerName, const string &sNodeName, const AdapterConf &conf, bool replace, CurrentPtr current);

	/**
	 * 插入配置文件
	 * @param sFullServerName
	 * @param fileName
	 * @param content
	 * @param level
	 * @param replace
	 * @return
	 */
	virtual int insertConfigFile(const string &sFullServerName, const string &fileName, const string &content, const string &sNodeName, int level, bool replace, CurrentPtr current);

	/**
	 *
	 * @param sFullServerName
	 * @param fileName
	 * @param sNodeName
	 * @param level
	 * @param configId
	 * @param current
	 * @return
	 */
	virtual int getConfigFileId(const string &sFullServerName, const string &fileName, const string &sNodeName, int level, int &configId, CurrentPtr current);

	/**
	 * 新增配置文件
	 *
	 * @param replace: 是否替换(false时, 如果有冲突就insert失败)
	 * @return : 返回值详见tarsErrCode枚举值
	 */
	virtual int insertHistoryConfigFile(int configId, const string &reason, const string &content, bool replace, CurrentPtr current);

	/**
	 * 注册插件
	 * @param conf
	 * @param current
	 * @return
	 */
	virtual int registerPlugin(const PluginConf &conf, CurrentPtr current);

	/**
	 * 是否有有开发权限
	 *
	 * @return : 返回值详见tarsErrCode枚举值
	 */
	virtual int hasDevAuth(const string &application, const string & serverName, const string & uid, bool &has, CurrentPtr current);

	/**
	 * 是否有运维权限
	 *
	 * @return : 返回值详见tarsErrCode枚举值
	 */
	virtual int hasOpeAuth(const string & application, const string & serverName, const string & uid, bool &has, CurrentPtr current);

	/**
	 * 是否有有管理员权限
	 *
	 * @return : 返回值详见tarsErrCode枚举值
	 */
	virtual int hasAdminAuth(const string & uid, bool &has, CurrentPtr current);

	/**
	 * 解析ticket, uid不为空则有效, 否则无效需要重新登录
	 *
	 * @return : 返回值详见tarsErrCode枚举值
	 */
	virtual int checkTicket(const string & ticket, string &uid, CurrentPtr current);

	/**
	 * 获取server tree
	 * @param tree
	 * @param current
	 * @return
	 */
	virtual int getServerTree(vector<ServerTree> &tree, CurrentPtr current);

	/**
	 *
	 * @param server
	 * @param packageType
	 * @param defaultVersion
	 * @param pack
	 * @param current
	 * @return
	 */
	virtual int getPatchPackage(const string &application, const string &serverName, int packageType, int defaultVersion, PatchPackage &pack, CurrentPtr current);

	/**
	 *
	 * @param fullServerName
	 * @param serverList
	 * @return
	 */
	virtual int getServerNameList(const vector<ApplicationServerName> &fullServerName, vector<map<string, string>> &serverList, CurrentPtr current);

protected:
	/**
	 * 删除太早的历史记录
	 * @param application
	 * @param serverName
	 */
    void deleteHistorys(const string &application, const string &serverName);

//    string getRemoteLogIp(const string& serverIp);
protected:

	string getServerType(const std::string & application, const std::string & serverName, const std::string & nodeName);
    PatchPrx _patchPrx;
	RegistryPrx _registryPrx;
	string	 _remoteLogIp;
    string   _remoteLogObj;
	string   _dockerSocket;
};


#endif
