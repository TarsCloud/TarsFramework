#pragma once

#include <string>
#include "util/tc_common.h"

static const int DEFAULT_WEIGHT(100);
static const int MIN_WEIGHT(10);
static const int WEIGHT_PERCENT_UNIT(100);
static const int TIMEOUT_WEIGHT_FACTOR(90);
static const std::string TIMEOUT_WEIGHT_PROPORTION("timeout-weight");
static const std::string TIME_CONSUMING_WEIGHT_PROPORTION("timeout-consuming");

// 注意要与EndpointManager EndpointWeightType定义保持一致
enum LOAD_BALANCE_TYPE
{
    LOAD_BALANCE_LOOP = 0,                // 默认轮询方式
    LOAD_BALANCE_STATIC_WEIGHT,           // 静态权重
    LOAD_BALANCE_DYNAMIC_WEIGHT,          // 动态权重
};

enum LOAD_BALANCE_DB_RESULT
{
    LOAD_BALANCE_DB_SUCCESS = 0,          // 加载成功
    LOAD_BALANCE_DB_EMPTY_LIST,           // 目标服务列表为空
    LOAD_BALANCE_DB_FAILED,               // 查询失败，可能是DB抛异常了
};