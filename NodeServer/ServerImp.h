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

#ifndef __SERVER_IMP_H_
#define __SERVER_IMP_H_
#include "Node.h"
#include "QueryF.h"
#include "ServerFactory.h"
#include "util/tc_common.h"



using namespace tars;
using namespace std;

class QueryImp : public QueryF
{
public:
	/**
	 * 初始化
	 */
	virtual void initialize()
	{
	};

	/**
	 * 退出
	 */
	virtual void destroy()
	{
	};

	virtual int doClose(CurrentPtr current);

	/**
	 * 根据id获取所有该对象的活动endpoint列表
	 */
	virtual vector<EndpointF> findObjectById(const string & id, CurrentPtr current);

	/**
	 * 根据id获取所有对象,包括活动和非活动对象
	 */
	virtual Int32 findObjectById4Any(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象所有endpoint列表
	 */
	Int32 findObjectById4All(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象同组endpoint列表
	 */
	Int32 findObjectByIdInSameGroup(const std::string & id, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象指定归属地的endpoint列表
	 */
	Int32 findObjectByIdInSameStation(const std::string & id, const std::string & sStation, vector<EndpointF> &activeEp, vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 根据id获取对象同set endpoint列表
	 */
	Int32 findObjectByIdInSameSet(const std::string & id,const std::string & setId,vector<EndpointF> &activeEp,vector<EndpointF> &inactiveEp, CurrentPtr current);

	/**
	 * 注册变化
	 * @param id
	 * @param current
	 * @return
	 */
	Int32 registerQuery(const std::string & id, const string& name, CurrentPtr current);

	/**
	 * 注册变化
	 * @param id
	 * @param current
	 * @return
	 */
	Int32 registerChange(const vector<std::string> & ids, const string& name, CurrentPtr current);

protected:

};

class ServerImp : public ServerF
{
public:
    /**
     * 构造函数
     */
    ServerImp()
    {
    }


    /**
     * 析构函数
     */
    virtual ~ServerImp()
    {
    }

    /**
     * 初始化
     */
    virtual void initialize();

    /**
     * 退出
     */
    virtual void destroy();

	/**
	 * 连接关闭
	 * @param current
	 * @return
	 */
	virtual int doClose(CurrentPtr current);

    /**
     * 上报心跳
     */
    virtual int keepAlive( const ServerInfo& serverInfo, CurrentPtr current ) ;

    /**
     * 激活中状态
     * @param serverInfo
     * @param current
     * @return
     */
    virtual int keepActiving( const ServerInfo& serverInfo, CurrentPtr current ) ;

    /**
     * 上报tars版本
     */
    virtual int reportVersion( const string &app,const string &serverName,const string &version,CurrentPtr current) ;

    /**
    * 获取最近keepalive的时间戳
    * @return 最后一次keepalive的时间戳
    */
    unsigned int getLatestKeepAliveTime(CurrentPtr current);

	/**
	 * 没有Servant时(模拟主控该接口)
	 * @param current
	 * @return
	 */
	virtual int doNoServant(CurrentPtr current, vector<char> &buffer);

private:
	QueryImp _queryImp;
};

typedef TC_AutoPtr<ServerImp> ServerImpPtr;

#endif

