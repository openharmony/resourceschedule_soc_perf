/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This file contains SocPerfClient fuzzer test modules. */

#define FUZZ_PROJECT_NAME "socperfclient_fuzzer"

#include "socperf_client.h"
#include <cstring>
#include <vector>
#include <string>

using namespace OHOS::SOCPERF;

namespace {
// 测试用例选择器常量
constexpr int TEST_GET_INSTANCE = 0;
constexpr int TEST_PERF_REQUEST = 1;
constexpr int TEST_PERF_REQUEST_EX = 2;
constexpr int TEST_POWER_LIMIT_BOOST = 3;
constexpr int TEST_THERMAL_LIMIT_BOOST = 4;
constexpr int TEST_LIMIT_REQUEST = 5;
constexpr int TEST_SET_REQUEST_STATUS = 6;
constexpr int TEST_SET_THERMAL_LEVEL = 7;
constexpr int TEST_REQUEST_DEVICE_MODE = 8;
constexpr int TEST_REQUEST_CMD_ID_COUNT = 9;
constexpr int TEST_RESET_CLIENT = 10;
constexpr int TEST_ON_REMOTE_DIED = 11;
constexpr int TEST_COMBINED_OPERATIONS = 12;
constexpr int TEST_BOUNDARY_CONDITIONS = 13;
constexpr int TEST_RAPID_CALLS = 14;
constexpr int TEST_STATE_TRANSITIONS = 15;
constexpr int TEST_CASE_COUNT = 16;

// 测试用性能请求命令ID常量
constexpr int32_t FUZZ_PERF_CMD_BASE = 1000;
constexpr int32_t FUZZ_PERF_CMD_RECOVERY = 1001;
constexpr int32_t FUZZ_PERF_CMD_RECOVERY2 = 1002;
constexpr int32_t FUZZ_PERF_CMD_GAME_ENTER = 2000;
constexpr int32_t FUZZ_PERF_CMD_GAME_EXIT = 2001;
constexpr int32_t FUZZ_PERF_CMD_THERMAL_NORM = 3000;
constexpr int32_t FUZZ_PERF_CMD_THERMAL_HIGH = 3001;
constexpr int32_t FUZZ_PERF_CMD_EX_SPECIAL = 10000;

// 热等级常量
constexpr int32_t THERMAL_LEVEL_SLIGHT = 1;
constexpr int32_t THERMAL_LEVEL_MEDIUM = 2;
constexpr int32_t THERMAL_LEVEL_SEVERE = 3;
constexpr int32_t THERMAL_LEVEL_EXTREME = 4;
constexpr int32_t THERMAL_LEVEL_LARGE = 100;

// 循环及迭代常量
constexpr int TOGGLE_LOOP_COUNT = 5;
constexpr int RAPID_LOOP_COUNT = 10;
constexpr int TOGGLE_MODULO = 2;
constexpr int MAX_LIMIT_ARRAY_COUNT = 10;

// 数组与字符串大小常量
constexpr size_t LARGE_ARRAY_SIZE = 100;
constexpr int64_t LARGE_CONFIG_VALUE = 1000;
constexpr size_t LONG_MSG_LENGTH = 1024;
constexpr size_t LONG_MODE_LENGTH = 256;
constexpr int32_t SINGLE_LIMIT_CONFIG = 100;

// 模糊测试函数最小输入长度常量
constexpr size_t MIN_COMBINED_DATA_SIZE = 20;
constexpr size_t MIN_RAPID_DATA_SIZE = 8;

// 从fuzzer数据中提取值
template<typename T>
T ExtractValue(const uint8_t* data, size_t size, size_t& offset)
{
    if (offset + sizeof(T) > size) {
        return T{};
    }
    T value;
    memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

// 从fuzzer数据中提取字符串
std::string ExtractString(const uint8_t* data, size_t size, size_t& offset, size_t maxLen = 256)
{
    if (offset >= size) {
        return "";
    }

    size_t len = (size - offset) < maxLen ? (size - offset) : maxLen;
    if (len == 0) {
        return "";
    }

    std::string str(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    return str;
}

// 测试 GetInstance
void TestGetInstance()
{
    SocPerfClient& client = SocPerfClient::GetInstance();
    // 多次调用验证单例
    SocPerfClient& client2 = SocPerfClient::GetInstance();
    SocPerfClient& client3 = SocPerfClient::GetInstance();
}

// 测试 PerfRequest（必须覆盖）
void TestPerfRequest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t)) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取cmdId
    int32_t cmdId = ExtractValue<int32_t>(data, size, offset);

    // 提取msg字符串
    std::string msg = ExtractString(data, size, offset, 128);

    // 调用 PerfRequest
    client.PerfRequest(cmdId, msg);

    // 测试一些常见的cmdId值
    client.PerfRequest(0, "test");
    client.PerfRequest(1, "test");
    client.PerfRequest(FUZZ_PERF_CMD_EX_SPECIAL, "test");
    client.PerfRequest(-1, "test");
    client.PerfRequest(INT32_MAX, "");
    client.PerfRequest(INT32_MIN, "");
}

// 测试 PerfRequestEx（必须覆盖）
void TestPerfRequestEx(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t) + 1) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取cmdId
    int32_t cmdId = ExtractValue<int32_t>(data, size, offset);

    // 提取onOffTag
    bool onOffTag = (data[offset++] % TOGGLE_MODULO == 0);

    // 提取msg
    std::string msg = ExtractString(data, size, offset, 128);

    // 调用 PerfRequestEx
    client.PerfRequestEx(cmdId, onOffTag, msg);

    // 测试开关切换
    client.PerfRequestEx(cmdId, true, "on");
    client.PerfRequestEx(cmdId, false, "off");

    // 测试特殊cmdId
    client.PerfRequestEx(0, onOffTag, msg);
    client.PerfRequestEx(FUZZ_PERF_CMD_EX_SPECIAL, onOffTag, msg);
    client.PerfRequestEx(-1, onOffTag, msg);
}

// 测试 PowerLimitBoost（必须覆盖）
void TestPowerLimitBoost(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < 1) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取onOffTag
    bool onOffTag = (data[offset++] % TOGGLE_MODULO == 0);

    // 提取msg
    std::string msg = ExtractString(data, size, offset, 128);

    // 调用 PowerLimitBoost
    client.PowerLimitBoost(onOffTag, msg);

    // 测试开关切换
    client.PowerLimitBoost(true, "boost on");
    client.PowerLimitBoost(false, "boost off");
    client.PowerLimitBoost(true, "");
    client.PowerLimitBoost(false, "");

    // 测试快速切换
    for (int i = 0; i < TOGGLE_LOOP_COUNT; i++) {
        client.PowerLimitBoost(i % TOGGLE_MODULO == 0, "toggle");
    }
}

// 测试 ThermalLimitBoost（必须覆盖）
void TestThermalLimitBoost(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < 1) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取onOffTag
    bool onOffTag = (data[offset++] % TOGGLE_MODULO == 0);

    // 提取msg
    std::string msg = ExtractString(data, size, offset, 128);

    // 调用 ThermalLimitBoost
    client.ThermalLimitBoost(onOffTag, msg);

    // 测试开关切换
    client.ThermalLimitBoost(true, "thermal boost on");
    client.ThermalLimitBoost(false, "thermal boost off");

    // 测试快速切换
    for (int i = 0; i < TOGGLE_LOOP_COUNT; i++) {
        client.ThermalLimitBoost(i % TOGGLE_MODULO == 0, "thermal toggle");
    }
}

// 测试 LimitRequest（必须覆盖）
void TestLimitRequest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t) + TOGGLE_MODULO) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取clientId
    int32_t clientId = ExtractValue<int32_t>(data, size, offset);

    // 提取tags数组大小（限制最大MAX_LIMIT_ARRAY_COUNT个）
    uint8_t tagsCount = data[offset++] % MAX_LIMIT_ARRAY_COUNT + 1;
    std::vector<int32_t> tags;

    for (uint8_t i = 0; i < tagsCount && offset + sizeof(int32_t) <= size; i++) {
        int32_t tag = ExtractValue<int32_t>(data, size, offset);
        tags.push_back(tag);
    }

    // 提取configs数组大小（限制最大MAX_LIMIT_ARRAY_COUNT个）
    uint8_t configsCount = data[offset++] % MAX_LIMIT_ARRAY_COUNT + 1;
    std::vector<int64_t> configs;

    for (uint8_t i = 0; i < configsCount && offset + sizeof(int64_t) <= size; i++) {
        int64_t config = ExtractValue<int64_t>(data, size, offset);
        configs.push_back(config);
    }

    // 提取msg
    std::string msg = ExtractString(data, size, offset, 128);

    // 调用 LimitRequest
    client.LimitRequest(clientId, tags, configs, msg);

    // 测试空数组
    std::vector<int32_t> emptyTags;
    std::vector<int64_t> emptyConfigs;
    client.LimitRequest(clientId, emptyTags, emptyConfigs, "empty");

    // 测试单个元素
    std::vector<int32_t> singleTag = {1};
    std::vector<int64_t> singleConfig = {SINGLE_LIMIT_CONFIG};
    client.LimitRequest(0, singleTag, singleConfig, "single");

    // 测试特殊clientId
    client.LimitRequest(0, tags, configs, msg);
    client.LimitRequest(-1, tags, configs, msg);
    client.LimitRequest(INT32_MAX, tags, configs, msg);
}

// 测试 SetRequestStatus（必须覆盖）
void TestSetRequestStatus(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < 1) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取status
    bool status = (data[offset++] % TOGGLE_MODULO == 0);

    // 提取msg
    std::string msg = ExtractString(data, size, offset, 128);

    // 调用 SetRequestStatus
    client.SetRequestStatus(status, msg);

    // 测试enable/disable切换
    client.SetRequestStatus(true, "enable socperf");
    client.SetRequestStatus(false, "disable socperf");
    client.SetRequestStatus(true, "");
    client.SetRequestStatus(false, "");

    // 测试快速切换
    for (int i = 0; i < TOGGLE_LOOP_COUNT; i++) {
        client.SetRequestStatus(i % TOGGLE_MODULO == 0, "toggle status");
    }
}

// 测试 SetThermalLevel（必须覆盖）
void TestSetThermalLevel(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t)) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取level
    int32_t level = ExtractValue<int32_t>(data, size, offset);

    // 调用 SetThermalLevel
    client.SetThermalLevel(level);

    // 测试各种热等级
    client.SetThermalLevel(0);                    // 正常
    client.SetThermalLevel(THERMAL_LEVEL_SLIGHT); // 轻微发热
    client.SetThermalLevel(THERMAL_LEVEL_MEDIUM); // 中度发热
    client.SetThermalLevel(THERMAL_LEVEL_SEVERE); // 严重发热
    client.SetThermalLevel(THERMAL_LEVEL_EXTREME);// 极端发热
    client.SetThermalLevel(-1);                   // 负数
    client.SetThermalLevel(THERMAL_LEVEL_LARGE);  // 大数值
    client.SetThermalLevel(INT32_MAX);
    client.SetThermalLevel(INT32_MIN);
}

// 测试 RequestDeviceMode（必须覆盖）
void TestRequestDeviceMode(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < TOGGLE_MODULO) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 提取status
    bool status = (data[offset++] % TOGGLE_MODULO == 0);

    // 提取mode字符串
    std::string mode = ExtractString(data, size, offset, 64);

    // 调用 RequestDeviceMode
    client.RequestDeviceMode(mode, status);

    // 测试常见的设备模式
    client.RequestDeviceMode("game", true);
    client.RequestDeviceMode("game", false);
    client.RequestDeviceMode("video", true);
    client.RequestDeviceMode("video", false);
    client.RequestDeviceMode("camera", true);
    client.RequestDeviceMode("camera", false);
    client.RequestDeviceMode("performance", true);
    client.RequestDeviceMode("performance", false);
    client.RequestDeviceMode("powersave", true);
    client.RequestDeviceMode("powersave", false);

    // 测试空字符串
    client.RequestDeviceMode("", status);

    // 测试长字符串
    std::string longMode(LONG_MODE_LENGTH, 'a');
    client.RequestDeviceMode(longMode, status);
}

// 测试 RequestCmdIdCount（必须覆盖）
void TestRequestCmdIdCount(const uint8_t* data, size_t size)
{
    SocPerfClient& client = SocPerfClient::GetInstance();

    // 测试空msg
    std::string result1 = client.RequestCmdIdCount("");

    if (data != nullptr && size > 0) {
        size_t offset = 0;
        std::string msg = ExtractString(data, size, offset, 128);

        // 调用 RequestCmdIdCount
        std::string result2 = client.RequestCmdIdCount(msg);
    }

    // 测试各种msg
    client.RequestCmdIdCount("test");
    client.RequestCmdIdCount("query count");
    client.RequestCmdIdCount("statistics");
}

// 测试 ResetClient（必须覆盖）
void TestResetClient()
{
    SocPerfClient& client = SocPerfClient::GetInstance();

    // 调用 ResetClient
    client.ResetClient();

    // 重置后再次使用
    client.PerfRequest(FUZZ_PERF_CMD_BASE, "after reset");

    // 再次重置
    client.ResetClient();
}

// 综合测试：组合多个操作
void TestCombinedOperations(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < MIN_COMBINED_DATA_SIZE) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    // 1. 设置请求状态
    bool status = (data[offset++] % TOGGLE_MODULO == 0);
    client.SetRequestStatus(status, "combined test");

    // 2. 设置热等级
    int32_t level = ExtractValue<int32_t>(data, size, offset);
    client.SetThermalLevel(level);

    // 3. 发送性能请求
    int32_t cmdId = ExtractValue<int32_t>(data, size, offset);
    client.PerfRequest(cmdId, "perf request");

    // 4. 发送扩展性能请求
    bool onOffTag = (data[offset++] % TOGGLE_MODULO == 0);
    client.PerfRequestEx(cmdId, onOffTag, "perf request ex");

    // 5. 电源限制提升
    client.PowerLimitBoost(onOffTag, "power boost");

    // 6. 热限制提升
    client.ThermalLimitBoost(onOffTag, "thermal boost");

    // 7. 请求设备模式
    std::string mode = ExtractString(data, size, offset, 32);
    if (!mode.empty()) {
        client.RequestDeviceMode(mode, status);
    }

    // 8. 查询cmdId计数
    client.RequestCmdIdCount("combined query");
}

// 测试边界条件和异常场景
void TestBoundaryConditions()
{
    SocPerfClient& client = SocPerfClient::GetInstance();

    // 测试极端cmdId
    client.PerfRequest(INT32_MAX, "max cmdId");
    client.PerfRequest(INT32_MIN, "min cmdId");
    client.PerfRequest(0, "zero cmdId");
    client.PerfRequest(-1, "negative cmdId");

    // 测试极端thermal level
    client.SetThermalLevel(INT32_MAX);
    client.SetThermalLevel(INT32_MIN);
    client.SetThermalLevel(0);

    // 测试大量tags和configs
    std::vector<int32_t> largeTags(LARGE_ARRAY_SIZE, 1);
    std::vector<int64_t> largeConfigs(LARGE_ARRAY_SIZE, LARGE_CONFIG_VALUE);
    client.LimitRequest(0, largeTags, largeConfigs, "large arrays");

    // 测试空字符串
    client.PerfRequest(FUZZ_PERF_CMD_BASE, "");
    client.PerfRequestEx(FUZZ_PERF_CMD_BASE, true, "");
    client.PowerLimitBoost(true, "");
    client.ThermalLimitBoost(true, "");
    client.SetRequestStatus(true, "");
    client.RequestDeviceMode("", true);
    client.RequestCmdIdCount("");

    // 测试长字符串
    std::string longMsg(LONG_MSG_LENGTH, 'x');
    client.PerfRequest(FUZZ_PERF_CMD_BASE, longMsg);
    client.RequestDeviceMode(longMsg, true);
}

// 测试快速连续调用
void TestRapidCalls(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < MIN_RAPID_DATA_SIZE) {
        return;
    }

    SocPerfClient& client = SocPerfClient::GetInstance();
    size_t offset = 0;

    int32_t cmdId = ExtractValue<int32_t>(data, size, offset);

    // 快速连续调用PerfRequest
    for (int i = 0; i < RAPID_LOOP_COUNT; i++) {
        client.PerfRequest(cmdId + i, "rapid call");
    }

    // 快速切换状态
    for (int i = 0; i < RAPID_LOOP_COUNT; i++) {
        client.SetRequestStatus(i % TOGGLE_MODULO == 0, "rapid toggle");
        client.PowerLimitBoost(i % TOGGLE_MODULO == 0, "rapid boost");
        client.ThermalLimitBoost(i % TOGGLE_MODULO == 0, "rapid thermal");
    }

    // 快速改变热等级
    for (int i = 0; i < TOGGLE_LOOP_COUNT; i++) {
        client.SetThermalLevel(i);
    }
}

// 测试状态切换场景
void TestStateTransitions(const uint8_t* data, size_t size)
{
    SocPerfClient& client = SocPerfClient::GetInstance();

    // 测试启用->禁用->启用
    client.SetRequestStatus(true, "enable");
    client.PerfRequest(FUZZ_PERF_CMD_BASE, "request while enabled");
    client.SetRequestStatus(false, "disable");
    client.PerfRequest(FUZZ_PERF_CMD_RECOVERY, "request while disabled");
    client.SetRequestStatus(true, "enable again");

    // 测试设备模式切换
    client.RequestDeviceMode("game", true);
    client.PerfRequest(FUZZ_PERF_CMD_GAME_ENTER, "in game mode");
    client.RequestDeviceMode("game", false);
    client.PerfRequest(FUZZ_PERF_CMD_GAME_EXIT, "exit game mode");

    // 测试热等级变化
    client.SetThermalLevel(0);
    client.PerfRequest(FUZZ_PERF_CMD_THERMAL_NORM, "normal thermal");
    client.SetThermalLevel(THERMAL_LEVEL_SEVERE);
    client.PerfRequest(FUZZ_PERF_CMD_THERMAL_HIGH, "high thermal");
    client.SetThermalLevel(0);
}

// 测试 OnRemoteDied（模拟服务死亡场景）
void TestOnRemoteDied()
{
    SocPerfClient& client = SocPerfClient::GetInstance();

    // 先进行一些正常操作，建立连接
    client.PerfRequest(FUZZ_PERF_CMD_BASE, "before death");
    client.SetThermalLevel(THERMAL_LEVEL_SLIGHT);

    // 触发 ResetClient，模拟服务死亡后重置
    // OnRemoteDied内部会调用ResetClient
    client.ResetClient();

    // 验证重置后能否继续工作
    client.PerfRequest(FUZZ_PERF_CMD_RECOVERY, "after reset/death");
    client.SetRequestStatus(true, "reconnect");

    // 再次触发重置
    client.ResetClient();

    // 再次验证
    client.GetInstance();
    client.PerfRequest(FUZZ_PERF_CMD_RECOVERY2, "after second reset");
}

} // namespace

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < 1) {
        return 0;
    }

    // 使用第一个字节决定测试哪个功能
    uint8_t testSelector = data[0];
    const uint8_t* testData = data + 1;
    size_t testSize = size - 1;

    switch (testSelector % TEST_CASE_COUNT) {
        case TEST_GET_INSTANCE:
            TestGetInstance();
            break;
        case TEST_PERF_REQUEST:
            TestPerfRequest(testData, testSize);
            break;
        case TEST_PERF_REQUEST_EX:
            TestPerfRequestEx(testData, testSize);
            break;
        case TEST_POWER_LIMIT_BOOST:
            TestPowerLimitBoost(testData, testSize);
            break;
        case TEST_THERMAL_LIMIT_BOOST:
            TestThermalLimitBoost(testData, testSize);
            break;
        case TEST_LIMIT_REQUEST:
            TestLimitRequest(testData, testSize);
            break;
        case TEST_SET_REQUEST_STATUS:
            TestSetRequestStatus(testData, testSize);
            break;
        case TEST_SET_THERMAL_LEVEL:
            TestSetThermalLevel(testData, testSize);
            break;
        case TEST_REQUEST_DEVICE_MODE:
            TestRequestDeviceMode(testData, testSize);
            break;
        case TEST_REQUEST_CMD_ID_COUNT:
            TestRequestCmdIdCount(testData, testSize);
            break;
        case TEST_RESET_CLIENT:
            TestResetClient();
            break;
        case TEST_ON_REMOTE_DIED:
            // OnRemoteDied测试（通过ResetClient模拟）
            TestOnRemoteDied();
            break;
        case TEST_COMBINED_OPERATIONS:
            // 组合测试
            TestCombinedOperations(testData, testSize);
            break;
        case TEST_BOUNDARY_CONDITIONS:
            // 边界条件测试
            TestBoundaryConditions();
            break;
        case TEST_RAPID_CALLS:
            // 快速连续调用测试
            TestRapidCalls(testData, testSize);
            break;
        case TEST_STATE_TRANSITIONS:
            // 状态切换测试
            TestStateTransitions(testData, testSize);
            break;
        default:
            break;
    }

    return 0;
}
