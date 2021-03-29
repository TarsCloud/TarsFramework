#include "util.h"
#include "LoadBalanceThread.h"
#include "util/tc_timeprovider.h"

using namespace tars;

//初始化数据库实例用
extern TC_Config * g_pconf;

void LoadBalanceThread::init()
{
    _db.init(g_pconf);

    _loadInterval = TC_Common::strto<int>(g_pconf->get("/tars/loadbalance<loadinterval>", "120"));
    map<string, string> mapLoadProportion;
    if (g_pconf->getDomainMap("/tars/loadbalance/loadproportion", mapLoadProportion))
    {
        ostringstream log;
        const int SIZE(static_cast<int>(mapLoadProportion.size()));
        log << "mapLoadProportion size: " << SIZE << "|";
        for (const auto &proportionPair : mapLoadProportion)
        {
            auto proportion(TC_Common::strto<int>(proportionPair.second));
            auto finalProportion(proportion > 0 ? proportion : DEFAULT_WEIGHT / SIZE);
            _mapLoad2Proportion[proportionPair.first] = finalProportion;

            log << proportionPair.first << "-org proportion: " << proportion << ", "
                << "finalProportion: " << finalProportion << "; ";
        }

        TLOGDEBUG(log.str() << "|" << endl);
    }

    _dynamicSwitch = TC_Common::strto<int>(g_pconf->get("/tars/loadbalance<loadswitch>", "0"));

    TLOGDEBUG("init _dynamicSwitch: " << _dynamicSwitch << "|"
                                      << "_loadInterval: " << _loadInterval << endl);
}

void LoadBalanceThread::terminate()
{
    _terminate = true;
}

void LoadBalanceThread::run()
{
    TLOGDEBUG("LoadBalanceThread run _dynamicSwitch: " << _dynamicSwitch << "|"
                                                        << "_loadInterval: " << _loadInterval << endl);
    while (!_terminate && _dynamicSwitch)
    {
        try
        {
            updateDynamicWeight();
        }
        catch (exception &e)
        {
            TLOGERROR("LoadBalanceThread::run catch exception:" << e.what() << endl);
        }
        catch (...)
        {
            TLOGERROR("LoadBalanceThread::run catch unkown exception." << endl);
        }

        TC_Common::sleep(_loadInterval);
    }
}

void LoadBalanceThread::updateDynamicWeight()
{
    ostringstream log;
    vector<string> vtDynamicServer;
    // 加载所有设置了动态负载均衡的servant列表
    _db.getAllDynamicWeightServant(vtDynamicServer);
    const size_t QUERY_MAX_SIZE(300);
    LoadCache loadCache;
    vector<string> vtServer;
    for (const auto &servant : vtDynamicServer)
    {
        vtServer.push_back(servant);
        // 每次最多查询300个servant
        if (vtServer.size() >= QUERY_MAX_SIZE)
        {
            // 从DB加载负载数据
            getMonitorDataFromDB(vtServer, loadCache);
            vtServer.clear();
        }
    }

    // 不足300个一次查询完成
    if (!vtServer.empty())
    {
        getMonitorDataFromDB(vtServer, loadCache);
    }

    log << "vtDynamicServer size: " << vtDynamicServer.size() << "|"
        << "loadCache from db size: " << loadCache.size() << "|";

    TLOGDEBUG(log.str() << endl);

    // 打印一下从db里读到的原始数据
    printLoadCache(loadCache);
    // 合并去重相同节点的多条记录
    mergeLoadInfo(loadCache);
    // 计算权重
    calculateWeight(loadCache);
    // 更新双缓存，使用了移动语义，之后不要再使用loadCache
    updateWeightCache(loadCache);
}

void LoadBalanceThread::getMonitorDataFromDB(const vector<string> &vtServer, LoadCache &loadCache)
{
    tars::TC_Mysql::MysqlData statData;
    if (LOAD_BALANCE_DB_SUCCESS == _db.loadStatData(vtServer, statData))
    {
        auto SIZE(statData.size());
        for (decltype(SIZE) i = 0; i < SIZE; ++i)
        {
            LoadBalanceItem info;
            info.statTime = statData[i]["stattime"];
            info.date = statData[i]["f_date"];
            info.slaveName = statData[i]["slave_name"];
            info.slaveIp = statData[i]["slave_ip"];
            info.succCount = TC_Common::strto<int64_t>(statData[i]["succ_count"]);
            info.timeoutCount = TC_Common::strto<int64_t>(statData[i]["timeout_count"]);
            info.aveTime = TC_Common::strto<int>(statData[i]["ave_time"]);

            loadCache[info.slaveName].vtBalanceItem.push_back(info);
        }
    }
}

void LoadBalanceThread::getDynamicWeight(const string &servant, std::vector<tars::EndpointF> &vtEndpoints)
{
    if (servant.empty())
    {
        return;
    }

    ostringstream log;
    log << "servant: " << servant << "|";
    const auto &loadCache = _loadCache.getReaderData();
    auto iter = loadCache.find(servant);
    // obj查不到，可能上报的是服务名。用服务名再查一次
    if (loadCache.end() == iter)
    {
        auto pos(servant.find_last_of(static_cast<string>(".")));
        if (string::npos != pos)
        {
            iter = loadCache.find(servant.substr(0, pos));
        }
    }

    bool isFound(true);
    if (loadCache.end() != iter) // 动态负载的key是servant或者服务名
    {
        log << "vtEndpoints size: " << vtEndpoints.size() << "|";
        int weightSum(0); // 求和打日志用
        for (auto &endpoint : vtEndpoints)
        {
            if (LOAD_BALANCE_DYNAMIC_WEIGHT == endpoint.weightType)
            {
                getDynamicWeight(iter->second.vtBalanceItem, endpoint);
            }
            weightSum += endpoint.weight;

        }
        // 构建日志
        log << "weightSum: " << weightSum << "|";
        const size_t SIZE(vtEndpoints.size());
        for (size_t i = 0; i < SIZE; ++i)
        {
            const tars::EndpointF &endpoint = vtEndpoints[i];
            log << endpoint.host << ":"
                << endpoint.port << ", "
                << endpoint.weightType << ", "
                << endpoint.weight << ", "
                << (weightSum ? endpoint.weight * WEIGHT_PERCENT_UNIT / weightSum : 0) << "%; ";
        }

        log << "|";
    }
    else
    {
        isFound = false;
        log << "not find|";
    }

    // 如果动态负载均衡关闭或者未找到对应servant，所有启用动态负载均衡的节点均修改为轮询方式
    if (!_dynamicSwitch || !isFound)
    {
        log << "vtEndpoints size: " << vtEndpoints.size() << "|modify: ";
        for (auto &endpoint : vtEndpoints)
        {
            if (LOAD_BALANCE_DYNAMIC_WEIGHT == endpoint.weightType)
            {
                endpoint.weightType = LOAD_BALANCE_LOOP;

                log << endpoint.host << ", "
                    << endpoint.port << ", "
                    << endpoint.weightType << ", "
                    << endpoint.weight << "; ";
            }
        }

        TLOGDEBUG(log.str() << "|" << endl);
        return;
    }

    TLOGDEBUG(log.str() << endl);
}

void LoadBalanceThread::getDynamicWeight(const vector<LoadBalanceItem> &vtLoadBalanceInfo, tars::EndpointF &endpointF)
{
    for (const auto &info : vtLoadBalanceInfo)
    {
        if (info.slaveIp == endpointF.host)
        {
            endpointF.weightType = LOAD_BALANCE_STATIC_WEIGHT; // 这里赋值为静态权重类型是因为直接复用EndpointManager里面的静态权重算法
            endpointF.weight = info.weight;

            break;
        }
    }

    // 如果数据异常权重为0则修改为轮询的方式，避免出现无rpc节点可调用的问题
    if (0 == endpointF.weight)
    {
        endpointF.weightType = LOAD_BALANCE_LOOP;
    }
}

void LoadBalanceThread::mergeLoadInfo(LoadCache &loadCache)
{
    map<string, LoadBalanceItem> mapIp2LoadInfo;
    for (auto &loadPair : loadCache)
    {
        ostringstream log;
        bool isRepeat(false); // 存在重复的才需要清空原有数据重新赋值
        // 相同ip可能会存在多条记录要聚合去重运算
        for (auto &loadInfo : loadPair.second.vtBalanceItem)
        {
            auto iter = mapIp2LoadInfo.find(loadInfo.slaveIp);
            if (mapIp2LoadInfo.end() == iter)
            {
                loadInfo.aveCount = 1;
                mapIp2LoadInfo[loadInfo.slaveIp] = loadInfo;
            }
            else
            {
                iter->second.timeoutCount += loadInfo.timeoutCount;
                iter->second.succCount += loadInfo.succCount;
                iter->second.aveTime += loadInfo.aveTime;
                iter->second.aveCount++;
                isRepeat = true;

                log << loadInfo.slaveIp << ", "
                    << loadInfo.timeoutCount << ", "
                    << loadInfo.succCount << ", "
                    << loadInfo.aveTime << "; ";
            }

            // 提前计算好所有节点平均耗时之和，用于后面求平均值
            loadPair.second.aveTimeSum += loadInfo.aveTime;
        }

         if (isRepeat)
        {
            log << "|";
            // 旧数据清掉，赋值聚合后去重的结果
            loadPair.second.vtBalanceItem.clear();
            int aveTimeSum(0);
            for (auto &ip2LoadPair : mapIp2LoadInfo)
            {
                // 计算平均值
                ip2LoadPair.second.aveTime /= ip2LoadPair.second.aveCount;
                loadPair.second.vtBalanceItem.push_back(std::move(ip2LoadPair.second));
                aveTimeSum += ip2LoadPair.second.aveTime;
            }

            // 如果有重复节点，平均值之和偏高了，需要重新计算赋值
            loadPair.second.aveTimeSum = aveTimeSum;
        }

        mapIp2LoadInfo.clear();

        //TLOGDEBUG(log.str() << endl);
    }
}

void LoadBalanceThread::calculateWeight(LoadCache &loadCache)
{
    for (auto &loadPair : loadCache)
    {
        ostringstream log;
        const auto ITEM_SIZE(static_cast<int>(loadPair.second.vtBalanceItem.size()));
        int aveTime(loadPair.second.aveTimeSum / ITEM_SIZE);
        log << "aveTime: " << aveTime << "|"
            << "vtBalanceItem size: " << ITEM_SIZE << "|";
        for (auto &loadInfo : loadPair.second.vtBalanceItem)
        {
            // 按每台机器在总耗时的占比反比例分配权重：权重 = 初始权重 *（耗时总和 - 单台机器平均耗时）/ 耗时总和
            TLOGDEBUG("loadPair.second.aveTimeSum: " << loadPair.second.aveTimeSum << endl);
            int aveTimeWeight(loadPair.second.aveTimeSum ? (DEFAULT_WEIGHT * ITEM_SIZE * (loadPair.second.aveTimeSum - loadInfo.aveTime) / loadPair.second.aveTimeSum) : 0);
            aveTimeWeight = aveTimeWeight <= 0 ? MIN_WEIGHT : aveTimeWeight;
            // 超时率权重：超时率权重 = 初始权重 - 超时率 * 初始权重 * 90%，折算90%是因为100%超时时也可能是因为流量过大导致的，保留小流量试探请求
            int timeoutRateWeight(loadInfo.succCount ? (DEFAULT_WEIGHT - static_cast<int>(loadInfo.timeoutCount * TIMEOUT_WEIGHT_FACTOR / (loadInfo.succCount + loadInfo.timeoutCount)))
                                                     : (loadInfo.timeoutCount ? MIN_WEIGHT : DEFAULT_WEIGHT));
            // 各类权重乘对应比例后相加求和
            loadInfo.weight = aveTimeWeight * getProportion(TIME_CONSUMING_WEIGHT_PROPORTION) / WEIGHT_PERCENT_UNIT
                              + timeoutRateWeight * getProportion(TIMEOUT_WEIGHT_PROPORTION) / WEIGHT_PERCENT_UNIT ;

            log << "aveTimeWeight: " << aveTimeWeight << ", "
                << "timeoutRateWeight: " << timeoutRateWeight << ", "
                << "loadInfo.weight: " << loadInfo.weight << "; ";
        }

        TLOGDEBUG(log.str() << "|" << endl);
    }
}

void LoadBalanceThread::updateWeightCache(LoadCache &loadCache)
{
    auto &cache = _loadCache.getWriterData();
    // 每次更新都覆盖，过早的统计数据对于当前来说可能已经不能准确的反应当前负载状况了
    cache = std::move(loadCache);
    _loadCache.swap();

    TLOGDEBUG("updateWeightCache swap|" << endl);
}

void LoadBalanceThread::printLoadCache(LoadCache &loadCache)
{
    TLOGDEBUG("loadCache size: " << loadCache.size() << "|" << endl);
    for (auto &loadPair : loadCache)
    {
        // 分开多行打印，数量多的情况下打在一起不方便看日志
        ostringstream log;
        log << "servant: " << loadPair.first << "|"
            << "size: " << loadPair.second.vtBalanceItem.size() << "|";
        for (auto &loadInfo : loadPair.second.vtBalanceItem)
        {
            log << loadInfo.statTime << ", "
                << loadInfo.date << ", "
                << loadInfo.slaveName << ", "
                << loadInfo.slaveIp << ", "
                << loadInfo.succCount << ", "
                << loadInfo.timeoutCount << ", "
                << loadInfo.aveTime << ", "
                << loadInfo.aveCount << ", "
                << loadInfo.port << ", "
                << loadInfo.weight << ", "
                << loadInfo.weightType << "; ";
        }

        TLOGDEBUG(log.str() << "|" << endl);
    }
}
