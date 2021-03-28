#pragma once

#include <vector>
#include <map>
#include "DbHandle.h"
#include "util/tc_thread.h"
#include "util/tc_autoptr.h"
#include "util/tc_singleton.h"
#include "util/tc_readers_writer_data.h"
#include "Node.h"
#include "Registry.h"

struct LoadBalanceItem
{
    string      statTime;                     // 统计上报时间
    string      date;                         // 日期
    string      slaveName;                    // 被调servant
    string      slaveIp;                      // 被调ip地址
    int64_t     succCount{0};                 // 成功次数
    int64_t     timeoutCount{0};              // 超时次数
    int         aveTime{0};                   // 平均耗时
    int         aveCount{0};                  // 重复节点计数
    int         port{0};                      // 端口
    int         weight{0};                    // 权重
    int         weightType{0};                // 权重类型，参考LOAD_BALANCE_TYPE
};

struct LoadBalanceInfo
{
    vector<LoadBalanceItem> vtBalanceItem;    // 节点列表
    int aveTimeSum{0};                        // 节点平均耗时求和
};

/**
 *  处理异步操作的线程
 */
class LoadBalanceThread : public TC_Thread, public TC_Singleton<LoadBalanceThread>
{
    using LoadCache = std::map<string, LoadBalanceInfo>;
public:
    /*
     * 构造函数
     */
    LoadBalanceThread() = default;

    virtual ~LoadBalanceThread()
    {
        terminate();
    }

    void init();
    // 单例模式拷贝构造函数和赋值运算符都禁用
    LoadBalanceThread(const LoadBalanceThread &manager) = delete;
    LoadBalanceThread& operator=(const LoadBalanceThread &manager) = delete;

    /*
     * 线程执行函数
     */
    virtual void run();

    /*
     * 停止运行
     */
    void terminate();

    // 获取节点权重
    void getDynamicWeight(const string &servant, std::vector<tars::EndpointF> &vtEndpoints);

private:
    // 更新动态负载权重
    void updateDynamicWeight();
    // 从DB加载负载数据
    void getMonitorDataFromDB(const vector<string> &vtServer, LoadCache &loadCache);
    // 单节点获取权重
    void getDynamicWeight(const vector<LoadBalanceItem> &vtLoadBalanceInfo, tars::EndpointF &endpointF);
    // 合并去重相同节点的多条记录
    void mergeLoadInfo(LoadCache &loadCache);
    // 计算权重
    void calculateWeight(LoadCache &loadCache);
    // 更新双缓存
    void updateWeightCache(LoadCache &loadCache);

    // 获取负载因子权重比例
    inline int getProportion(const string &weightFactor)
    {
        auto iter = _mapLoad2Proportion.find(weightFactor);

        return _mapLoad2Proportion.end() != iter ? iter->second : 0;
    }

    // 打印loadCache
    void printLoadCache(LoadCache &loadCache);

private:
    /*
     * 线程结束标志
     */
    bool                                 _terminate{false};

    /*
     * 访问主控db的处理类
     */
    CDbHandle                             _db;

    // 存储动态负载信息的缓存
    tars::TC_ReadersWriterData<LoadCache> _loadCache;

    // 轮询间隔，默认120秒
    int                                   _loadInterval{120};
    // 动态负载因子权重比例配置
    map<string, int>                      _mapLoad2Proportion;
    // 是否开启动态负载均衡开关，0：关闭；非0：开启；
    int                                   _dynamicSwitch{0};
};

#define LOAD_BALANCE_INS LoadBalanceThread::getInstance()