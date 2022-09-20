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

#ifndef __DB_PROXY_H__
#define __DB_PROXY_H__

#include "util/tc_common.h"
#include "util/tc_config.h"
#include "util/tc_monitor.h"
#include "util/tc_mysql.h"
#include "util/tc_file.h"
#include "util/tc_singleton.h"
#include "Node.h"
#include "servant/RemoteLogger.h"
#include "AdminReg.h"

using namespace tars;
/**
 * 主控获取node信息异常
 */
struct TarsNodeNotRegistryException : public TarsException
{
    TarsNodeNotRegistryException(const string &buffer) : TarsException(buffer){};
    TarsNodeNotRegistryException(const string &buffer, int err) : TarsException(buffer, err){};
    ~TarsNodeNotRegistryException() throw(){};
};

/**
 *  数据库操作类
 */
class DbProxy : public tars::TC_Singleton<DbProxy>
{
public:

    /**
     * 构造函数
     */
    DbProxy(){}

    /**
     * 初始化
     * @param pconf 配置文件
     * @return 0-成功 others-失败
     */
    int init(TC_Config *pconf);

    /**
     * 获取框架服务
     */
    int getFramework(vector<tars::FrameworkServer> &servers);
    /**
     * 获取特定node id的对象代理
     * @param nodeName : node id
     * @return :  对象代理的智能指针
     */
    NodePrx getNodePrx(const string & nodeName);

    /**
     * 增加异步任务
     * 
     * @param taskNo 
     * @param taskReq 
     * @param serial 
     * 
     * @return string 
     */
    int addTaskReq(const TaskReq &taskReq);

    /**
     * 获取
     * 
     * @param taskNo 
     * 
     * @return TaskRsp 
     */
    int getTaskRsp(const string &taskNo, TaskRsp &taskRsp);

    /**
     * 获取历史的Task信息
     * 
     * @param application 
     * @param serverName 
     * @param command 
     * 
     * @return vector<TaskRsp> 
     */
    int getTaskHistory(const string & application, const string & serverName, const string & command, vector<TaskRsp> &taskRsp);

    /**
     * 设置task item信息
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
    int setTaskItemInfo(const string & itemNo, const map<string, string> &info);

    /**
     * 下线服务
     * 
     * @param application 
     * @param serverName 
     * @param nodeName 
     * 
     * @return int 
     */
    int undeploy(const string & application, const string & serverName, const string & nodeName, const string &user, string &log);

    /**
     * 获取活动node列表endpoint信息
     * @param out result 结果描述
     * @return map : 对应id node的obj
     */
    map<string, string> getActiveNodeList(string & result);

    /**
     * 获取node版本
    * @param name   node名称
     * @param version   node版本
     * @param out result 结果描述
     * @return  0-成功 others-失败
     */
    int getNodeVersion(const string &nodeName, string &version, string & result);

    /**
     * 获取在该node部署的server列表
     *
     * @param app:       应用
     * @param serverName: server 名
     * @param nodeName : node id
     *
     * @return server信息列表
     */
    vector<ServerDescriptor> getServers(const string & app, const string & serverName, const string & nodeName,bool withDnsServer = false);

    /**
     * 根据应用和服务名，获取部署的node列表
     */
    int getNodeList(const string & app, const string & serverName, vector<string>& nodeNames);

    /**
     * 获取server的配置模板
     * @param sTemplateName 模板名称
     * @param sResultDesc 结果描述
     * @return 模板内容
     */
    string getProfileTemplate(const string & sTemplateName, string & sResultDesc);

    /**
     * 根据patchId获取patch tgz包
     * 
     * @param patchId 
     * 
     * @return string 
     */
    int getInfoByPatchId(const string &patchId, string &patchFile, string &md5);

    /**
     * 更新发布成功信息
     * 
     * @param patchId 
     * 
     * @return int 
     */
    int updatePatchByPatchId(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ);

    /**
     * 删除历史数据包
     * 
     */   
    vector<string> deleteHistorys(const string &application, const string &serverName);

    /**
     * 从数据库中删除服务
     * 
     */      
    int uninstallServer(const string &application, const string &serverName, const string &nodeName, string &result);


    /**
    * 是否有服务
    *
    * @param application: 服务基本信心 
    * @param serverName: 是否替换
    * @return : 返回值详见tarsErrCode枚举值
    */
    int hasServer(const string &application, const string &serverName, bool &has);

    /**
    * 新增服务
    *
    * @param conf: 服务基本信心 
    * @param replace: 是否替换()
    * @return : 返回值详见tarsErrCode枚举值
    */
    int insertServerConf(const ServerConf &conf, bool replace);

    /**
    * 新增adapter
    *
    * @param conf: 服务基本信心 
    * @param replace: 是否替换()
    * @return : 返回值详见tarsErrCode枚举值
    */
    int insertAdapterConf(const string &sApplication, const string &sServerName, const string &sNodeName, const AdapterConf &conf, bool replace);

	/**
	 * 插入配置文件
	 * @param sFullServerName
	 * @param fileName
	 * @param content
	 * @param level
	 * @param replace
	 * @return
	 */
	int insertConfigFile(const string &sFullServerName, const string &fileName, const string &content, const string &sNodeName, int level, bool replace);

	/**
	 * 获取配置Id
	 * @param sFullServerName
	 * @param fileName
	 * @param sNodeName
	 * @param level
	 * @param configId
	 * @return
	 */
	int getConfigFileId(const string &sFullServerName, const string &fileName, const string &sNodeName, int level, int &configId);

	/**
	 * 新增配置文件
	 *
	 * @param replace: 是否替换(false时, 如果有冲突就insert失败)
	 * @return : 返回值详见tarsErrCode枚举值
	 */
	int insertHistoryConfigFile(int configId, const string &reason, const string &content, bool replace);

	/**
	 *
	 * @param conf
	 * @return
	 */
	int registerPlugin(const PluginConf &conf);

	/**
	 * 用户权限flag
	 */
	struct UserFlag
	{
		string flag;
		string role;
	};
	/**
	 * 获取用户的权限
	 * @param uid
	 * @param data
	 * @return
	 */
	int getAuth(const string &uid, vector<UserFlag> &data);

	/**
	 * 获取ticket
	 * @param ticket
	 * @param uid
	 * @return
	 */
	int getTicket(const string &ticket, string &uid);

	/**
	 *
	 * @param tree
	 * @return
	 */
	int getServerTree(vector<ServerTree> &tree);

	/**
	 *
	 * @param server
	 * @param defaultVersion
	 * @param pack
	 * @return
	 */
	int getPatchPackage(const string &application, const string &serverName, int defaultVersion, PatchPackage &pack);
	/**
	 * get server name list
	 * @param fullServerName
	 * @param serverList
	 * @return
	 */
	int getServerNameList(const vector<ApplicationServerName> &fullServerName, vector<map<string, string>> &serverList);

protected:
    /**
     * 获取server的配置模板
     * @param sTemplateName 模板名称
     * @param mapRecursion 被递归到的模板
     * @param sResultDesc 结果描述
     * @return 模板内容
     */
    string getProfileTemplate(const string & sTemplateName, map<string, int> & mapRecursion, string & sResultDesc);

public:
    /**
     * 更新server状态
     *
     * @param app:       应用
     * @param serverName: server 名
     * @param nodeName : node id
     * @param stateFields: 更新状态字段名
     * @param state :  server状态
     *
     * @return server信息列表
     */
    int updateServerState(const string & app, const string & serverName, const string & nodeName,
            const string & stateFields, tars::ServerState state, int processId = -1);

    int updateServerFlowState(const string & app, const string & serverName, const vector<string>& nodeList, bool bActive);
    int updateServerFlowStateOne(const string & app, const string & serverName, const string& nodeName, bool bActive);

    /**
     * 获取application列表
     * @param null
     * @param out reuslt
     * @return application列表
     */
    vector<string> getAllApplicationNames(string & result);


    /**
     * 获取server列表
     * @param null
     * @return node 列表
     */
    vector<vector<string> > getAllServerIds(string & result);

    /**
     * 设置server发布版本
     * @param app:       应用
     * @param serverName: server 名
     * @param nodeName : node id
     * @param version  : server 版本
     * @param user     : 发布者
     */
    int setPatchInfo(const string & app, const string & serverName, const string & nodeName,
            const string & version, const string & user);

//    /**
//     * 自动伸缩时调用的灰度发布接口
//     *
//     * @param app:       应用
//     * @param servername: server 名
//     * @param nodename : node id
//     * @param status  : 流量状态,NORMAL-正常流量,NO_FLOW-无流量
//     *
//     * @return : 0-成功 others-失败
//     */
//    int gridPatchServer(const string & app, const string & servername, const string & nodename, const string &status);

//    /**
//     * 轮询数据库，将心跳超时的registry设为不存活
//     * @param iTiemout 超时时间
//     * @return
//     */
//    int checkRegistryTimeout(unsigned uTimeout);
//
//    /**
//     * 更新registry信息到db
//     */
//    int updateRegistryInfo2Db(bool bRegHeartbeatOff);

//    /**
//     * 根据ip获取组id
//     * @return int <0 失败 其它正常
//     */
//    int getGroupId(const string& ip);

    /**
     * 加载IP物理分组信息
    */
//    int loadIPPhysicalGroupInfo();

    /**
     * 只保留必要的数据
     */
    void doReserveDb(const string path, TC_Config *pconf);

protected:
    //mysql连接对象
    static vector<TC_Mysql*> _mysqlReg;
	static vector<TC_ThreadMutex*> _mysqlLocks;

    //匹配分组信息
//    static vector<map<string,string>> _serverGroupRule;

    //用于初始化保护
    static TC_ThreadLock _mutex;

//    //分组信息
//    static map<string,int> _serverGroupCache;

    //保留历史发布记录
    static int _patchHistory;
};

#define DBPROXY		DbProxy::getInstance()
#endif
