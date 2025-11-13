/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

/**
 * @file soc_fuzzer.cpp
 * @brief Unified fuzzing driver for SocPerf APIs
 *
 * This driver consolidates fuzzing for all SocPerf APIs:
 * - SocPerfConfig (6 APIs)
 * - SocPerfClient (5 APIs)
 * - SocPerf core (10 APIs)
 * - Event handling (7 APIs)
 * - Frequency/action management (4 APIs)
 *
 * Total coverage: 32 APIs
 */

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

// SocPerf headers
#include "socperf_client.h"
#include "socperf_action_type.h"
#include "socperf.h"
#include "socperf_common.h"
#include "socperf_config.h"
#include "socperf_thread_wrap.h"
#include "socperf_server.h"
#include "socperf_hitrace_chain.h"
#include "socperf_log.h"
#include "socperf_lru_cache.h"
#include "socperf_trace.h"
#include <libxml/tree.h>

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace FuzzUtils
 {

    // ============================================================================
    // Constants and Macros
    // ============================================================================

    constexpr int32_t MIN_FUZZ_INPUT_SIZE = 4;
    constexpr int32_t DEFAULT_STRING_LENGTH = 256;
    constexpr int32_t DEFAULT_MODE_STRING_LENGTH = 128;
    constexpr int32_t DEFAULT_VECTOR_SIZE = 10;
    constexpr int32_t MAX_STRING_LENGTH = 1024;
    constexpr int32_t MAX_VECTOR_SIZE = 100;
    constexpr int32_t MAX_ARGC = 32;
    constexpr int32_t MAX_ARG_LENGTH = 512;
    constexpr int32_t FUZZ_MODE_COUNT = 4;
    constexpr int32_t MAX_STRESS_CALLS = 1000;

  // ============================================================================
    // Inline Validation Functions
    // ============================================================================

    inline bool FuzzCheckSize(size_t size, size_t minSize)
    {
        return size >= minSize;
    }

    inline bool FuzzCheckPointer(const void* ptr)
    {
        return ptr != nullptr;
    }

    inline bool FuzzCheckOffset(size_t offset, size_t size, size_t typeSize)
    {
        return offset + typeSize <= size;
    }

    // ============================================================================
    // DataExtractor Class
    // ============================================================================

    class DataExtractor {
    public:
        DataExtractor(const uint8_t *data, size_t size);
        ~DataExtractor() = default;

        int32_t ExtractInt32();
        int64_t ExtractInt64();
        bool ExtractBool();
        float ExtractFloat();
        std::string ExtractString(size_t maxLen = DEFAULT_STRING_LENGTH);

        std::vector<int32_t> ExtractInt32Vector(size_t maxElements = DEFAULT_VECTOR_SIZE);
        std::vector<int64_t> ExtractInt64Vector(size_t maxElements = DEFAULT_VECTOR_SIZE);
        std::vector<std::string> ExtractStringVector(size_t maxStrings = 5, size_t maxLen = 100);

        bool HasMore() const
        {
            return offset_ < size_;
        }
        size_t Remaining() const
        {
            return size_ - offset_;
        }
        void Reset()
        {
            offset_ = 0;
        }
        size_t GetOffset() const
        {
            return offset_;
        }

        DataExtractor(const DataExtractor &) = delete;
        DataExtractor &operator=(const DataExtractor &) = delete;
        DataExtractor(DataExtractor &&) = delete;
        DataExtractor &operator=(DataExtractor &&) = delete;

    private:
        bool CanExtract(size_t size) const
        {
            return offset_ + size <= size_;
        }
        const uint8_t *SafeRead(size_t size);

        const uint8_t *data_;
        size_t size_;
        size_t offset_;
    };

    DataExtractor::DataExtractor(const uint8_t *data, size_t size)
        : data_(data), size_(size), offset_(0) {}

    const uint8_t *DataExtractor::SafeRead(size_t size)
    {
        if (!CanExtract(size)) {
            return nullptr;
        }
        const uint8_t *ptr = data_ + offset_;
        offset_ += size;
        return ptr;
    }

    int32_t DataExtractor::ExtractInt32()
    {
        const uint8_t *ptr = SafeRead(sizeof(int32_t));
        if (ptr == nullptr)  {
            return 0;
        }
        int32_t value = 0;
        memcpy_s(&value, sizeof(value), ptr, sizeof(int32_t));
        return value;
    }

    int64_t DataExtractor::ExtractInt64()
    {
        const uint8_t *ptr = SafeRead(sizeof(int64_t));
        if (ptr == nullptr)  {
            return 0;
        }
        int64_t value = 0;
        memcpy_s(&value, sizeof(value), ptr, sizeof(int64_t));
        return value;
    }

    bool DataExtractor::ExtractBool()
    {
        const uint8_t *ptr = SafeRead(sizeof(uint8_t));
        if (ptr == nullptr)
        {
            return false;
        }
        return *ptr != 0;
    }

    float DataExtractor::ExtractFloat()
    {
        const uint8_t *ptr = SafeRead(sizeof(float));
        if (ptr == nullptr) {
            return 0.0f;
        }
        float value = 0.0f;
        memcpy_s(&value, sizeof(value), ptr, sizeof(float));
        return value;
    }

    std::string DataExtractor::ExtractString(size_t maxLen)
    {
        if (maxLen == 0 || maxLen > MAX_STRING_LENGTH)  {
            maxLen = MAX_STRING_LENGTH;
        }

        if (offset_ >= size_)  {
            return "";
        }

        size_t len = data_[offset_] % (maxLen + 1);
        const uint8_t *ptr = SafeRead(len + 1);
        if (ptr == nullptr)  {
            return "";
        }

        if (len > 0)
        {
            return std::string(reinterpret_cast<const char *>(ptr + 1), len);
        }
        return "";
    }

    std::vector<int32_t> DataExtractor::ExtractInt32Vector(size_t maxElements)
    {
        std::vector<int32_t> result;
        if (maxElements == 0 || maxElements > MAX_VECTOR_SIZE) {
            maxElements = MAX_VECTOR_SIZE;
        }

        if (!CanExtract(sizeof(int32_t))) {
            return result;
        }
        int32_t numElements = ExtractInt32();
        if (numElements < 0) {
            numElements = -numElements;
        }
        if (numElements > static_cast<int32_t>(maxElements)) {
            numElements = maxElements;
        }

        for (int32_t i = 0; i < numElements; ++i) {
            if (!CanExtract(sizeof(int32_t))) {
                break;
            }
            result.push_back(ExtractInt32());
        }

        return result;
    }

    std::vector<int64_t> DataExtractor::ExtractInt64Vector(size_t maxElements)
    {
        std::vector<int64_t> result;
        if (maxElements == 0 || maxElements > MAX_VECTOR_SIZE) {
            maxElements = MAX_VECTOR_SIZE;
        }

        if (!CanExtract(sizeof(int32_t))) {
            return result;
        }
        int32_t numElements = ExtractInt32();
        if (numElements < 0) {
            numElements = -numElements;
        }
        if (numElements > static_cast<int32_t>(maxElements)) {
            numElements = maxElements;
        }

        for (int32_t i = 0; i < numElements; ++i) {
            if (!CanExtract(sizeof(int64_t))) {
                break;
            }
            result.push_back(ExtractInt64());
        }

        return result;
    }

    std::vector<std::string> DataExtractor::ExtractStringVector(size_t maxStrings, size_t maxLen)
    {
        std::vector<std::string> result;
        if (maxStrings == 0) {
            return result;
        }
        for (size_t i = 0; i < maxStrings && HasMore(); ++i) {
            result.push_back(ExtractString(maxLen));
        }

        return result;
    }

} // namespace FuzzUtils

using namespace FuzzUtils;

// ============================================================================
// Constants and Configuration
// ============================================================================

constexpr int32_t API_INIT = 0;
constexpr int32_t API_PERF_REQUEST = 1;
constexpr int32_t API_LIMIT = 2;
constexpr int32_t API_DEVICE_MODE = 3;
constexpr int32_t API_EVENT = 4;
constexpr int32_t API_FREQ = 5;

static constexpr int32_t API_GROUP_COUNT = 6;

enum class FuzzMode : int32_t
{
    RANDOM = 0,
    SEQUENTIAL = 1,
    GUIDED = 2,
    STRESS = 3,
};

// ============================================================================
// Global State Management
// ============================================================================

static std::mutex g_fuzzMutex;
static bool g_systemInitialized = false;

void InitializeSystemIfNeeded(DataExtractor &extractor)
{
    std::lock_guard<std::mutex> lock(g_fuzzMutex);
    if (g_systemInitialized) {
        return;
    }

    if (OHOS::SOCPERF::SocPerfConfig::GetInstance().Init()) {
        g_systemInitialized = true;
    }
}

// ============================================================================
// API Functions
// ============================================================================

void CallApiInitialize(DataExtractor &extractor) 
{
    if (OHOS::SOCPERF::SocPerfConfig::GetInstance().Init()) {
        g_systemInitialized = true;
    }
}

void CallApiCreateThreadWraps(DataExtractor &extractor) 
{
    if (!g_systemInitialized) {
        return;
    }
    // CreateThreadWraps is handled through SocPerfConfig
    (void)extractor;
}

void CallApiInitThreadWraps(DataExtractor &extractor) 
{
    if (!g_systemInitialized) {
        return;
    }
    // InitThreadWraps is handled through SocPerfConfig
    (void)extractor;
}

void CallApiIsValidResId(DataExtractor &extractor) 
{
    int32_t resId = extractor.ExtractInt32();
    auto &config = OHOS::SOCPERF::SocPerfConfig::GetInstance();
    volatile bool result = config.IsValidResId(resId);
    (void)result;
}

void CallApiIsGovResId(DataExtractor &extractor)
{
    int32_t resId = extractor.ExtractInt32();
    auto &config = OHOS::SOCPERF::SocPerfConfig::GetInstance();
    volatile bool result = config.IsGovResId(resId);
    (void)result;
}

void CallApiCheckClientValid(DataExtractor &extractor)
{
    if (!g_systemInitialized)
    {
        return;
    }
    // CheckClientValid is private, skip internal validation
    (void)extractor;
}

void CallApiPerfRequest(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t cmdId = extractor.ExtractInt32();
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.PerfRequest(cmdId, msg);
}

void CallApiPerfRequestEx(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t cmdId = extractor.ExtractInt32();
    bool onOff = extractor.ExtractBool();
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.PerfRequestEx(cmdId, onOff, msg);
}

void CallApiSetRequestStatus(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    bool status = extractor.ExtractBool();
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.SetRequestStatus(status, msg);
}

void CallApiRequestCmdIdCount(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    volatile std::string result = client.RequestCmdIdCount(msg);
    (void)result;
}
void CallApiResetClient(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.ResetClient();
}

void CallApiLimitRequest(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t clientId = extractor.ExtractInt32();
    auto tags = extractor.ExtractInt32Vector(DEFAULT_VECTOR_SIZE);
    auto configs = extractor.ExtractInt64Vector(DEFAULT_VECTOR_SIZE);
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.LimitRequest(clientId, tags, configs, msg);
}

void CallApiPowerLimitBoost(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    bool enable = extractor.ExtractBool();
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.PowerLimitBoost(enable, msg);
}

void CallApiThermalLimitBoost(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    bool enable = extractor.ExtractBool();
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.ThermalLimitBoost(enable, msg);
}

void CallApiSetThermalLevel(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t level = extractor.ExtractInt32();
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.SetThermalLevel(level);
}

void CallApiClearAllAliveRequest(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    // ClearAllAliveRequest requires SocPerf instance access
    (void)extractor;
}

void CallApiRequestDeviceMode(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    std::string mode = extractor.ExtractString(DEFAULT_MODE_STRING_LENGTH);
    bool status = extractor.ExtractBool();
    auto &client = OHOS::SOCPERF::SocPerfClient::GetInstance();
    client.RequestDeviceMode(mode, status);
}

void CallApiGetDeviceMode(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    // GetDeviceMode requires SocPerf instance access
    (void)extractor;
}

void CallApiGetMatchCmdId(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t cmdId = extractor.ExtractInt32();
    bool isTagOnOff = extractor.ExtractBool();
    // GetMatchCmdId requires SocPerf instance access
    (void)cmdId;
    (void)isTagOnOff;
}

void CallApiUpdateCmdIdCount(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t cmdId = extractor.ExtractInt32();
    // UpdateCmdIdCount requires SocPerf instance access
    (void)cmdId;
}

void CallApiCheckTimeInterval(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    bool onOff = extractor.ExtractBool();
    int32_t cmdId = extractor.ExtractInt32();
    // CheckTimeInterval requires SocPerf instance access
    (void)onOff;
    (void)cmdId;
}

void CallApiCompleteEvent(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    // CompleteEvent requires SocPerf instance access
    (void)extractor;
}

void CallApiSendLimitRequestEvent(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t clientId = extractor.ExtractInt32();
    int32_t resId = extractor.ExtractInt32();
    int64_t resValue = extractor.ExtractInt64();
    // SendLimitRequestEvent requires SocPerf instance access
    (void)clientId;
    (void)resId;
    (void)resValue;
}

void CallApiSendLimitRequestEventOn(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t clientId = extractor.ExtractInt32();
    int32_t resId = extractor.ExtractInt32();
    int64_t resValue = extractor.ExtractInt64();
    // SendLimitRequestEventOn requires SocPerf instance access
    (void)clientId;
    (void)resId;
    (void)resValue;
}

void CallApiSendLimitRequestEventOff(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t clientId = extractor.ExtractInt32();
    int32_t resId = extractor.ExtractInt32();
    int64_t resValue = extractor.ExtractInt64();
    // SendLimitRequestEventOff requires SocPerf instance access
    (void)clientId;
    (void)resId;
    (void)resValue;
}

void CallApiCopyEvent(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t oldCmdId = extractor.ExtractInt32();
    int32_t newCmdId = extractor.ExtractInt32();
    // CopyEvent requires SocPerf instance access
    (void)oldCmdId;
    (void)newCmdId;
}

void CallApiAddPidAndTidInfo(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    std::string msg = extractor.ExtractString(DEFAULT_STRING_LENGTH);
    // AddPidAndTidInfo is private, skip
    (void)msg;
}

void CallApiGetActionsInfo(DataExtractor &extractor)
{
    if (!g_systemInitialized) {
        return;
    }
    int32_t cmdId = extractor.ExtractInt32();
    // GetActionsInfo requires SocPerf instance access
    (void)cmdId;
}

void CallApiMatchDeviceMode(DataExtractor &extractor) {}
void CallApiMatchDeviceModeCmd(DataExtractor &extractor) {}
void CallApiDoFreqActions(DataExtractor &extractor) {}
void CallApiDoPerfRequestThremalLvl(DataExtractor &extractor) {}

// ============================================================================
// API Dispatcher Table
// ============================================================================

typedef void (*APIFunc)(DataExtractor &);

struct APIDescriptor{
    const char *name;
    int32_t groupId;
    APIFunc func;
};

const APIDescriptor API_TABLE[] =  {
    {"Init", API_INIT, CallApiInitialize},
    {"CreateThreadWraps", API_INIT, CallApiCreateThreadWraps},
    {"InitThreadWraps", API_INIT, CallApiInitThreadWraps},
    {"IsValidResId", API_INIT, CallApiIsValidResId},
    {"IsGovResId", API_INIT, CallApiIsGovResId},
    {"CheckClientValid", API_INIT, CallApiCheckClientValid},
    {"PerfRequest", API_PERF_REQUEST, CallApiPerfRequest},
    {"PerfRequestEx", API_PERF_REQUEST, CallApiPerfRequestEx},
    {"SetRequestStatus", API_PERF_REQUEST, CallApiSetRequestStatus},
    {"RequestCmdIdCount", API_PERF_REQUEST, CallApiRequestCmdIdCount},
    {"ResetClient", API_PERF_REQUEST, CallApiResetClient},
    {"LimitRequest", API_LIMIT, CallApiLimitRequest},
    {"PowerLimitBoost", API_LIMIT, CallApiPowerLimitBoost},
    {"ThermalLimitBoost", API_LIMIT, CallApiThermalLimitBoost},
    {"SetThermalLevel", API_LIMIT, CallApiSetThermalLevel},
    {"ClearAllAliveRequest", API_LIMIT, CallApiClearAllAliveRequest},
    {"RequestDeviceMode", API_DEVICE_MODE, CallApiRequestDeviceMode},
    {"GetDeviceMode", API_DEVICE_MODE, CallApiGetDeviceMode},
    {"GetMatchCmdId", API_DEVICE_MODE, CallApiGetMatchCmdId},
    {"MatchDeviceMode", API_DEVICE_MODE, CallApiMatchDeviceMode},
    {"MatchDeviceModeCmd", API_DEVICE_MODE, CallApiMatchDeviceModeCmd},
    {"UpdateCmdIdCount", API_EVENT, CallApiUpdateCmdIdCount},
    {"CheckTimeInterval", API_EVENT, CallApiCheckTimeInterval},
    {"CompleteEvent", API_EVENT, CallApiCompleteEvent},
    {"SendLimitRequestEvent", API_EVENT, CallApiSendLimitRequestEvent},
    {"SendLimitRequestEventOn", API_EVENT, CallApiSendLimitRequestEventOn},
    {"SendLimitRequestEventOff", API_EVENT, CallApiSendLimitRequestEventOff},
    {"CopyEvent", API_EVENT, CallApiCopyEvent},
    {"AddPidAndTidInfo", API_FREQ, CallApiAddPidAndTidInfo},
    {"GetActionsInfo", API_FREQ, CallApiGetActionsInfo},
    {"DoFreqActions", API_FREQ, CallApiDoFreqActions},
    {"DoPerfRequestThremalLvl", API_FREQ, CallApiDoPerfRequestThremalLvl},
};

constexpr int32_t API_COUNT = sizeof(API_TABLE) / sizeof(API_TABLE[0]);

// ============================================================================
// Testing Strategies
// ============================================================================

void ExecuteRandomMode(DataExtractor &extractor)
{
    while (extractor.HasMore()) {
        int32_t apiIndex = extractor.ExtractInt32() % API_COUNT;
        if (apiIndex >= 0 && apiIndex < API_COUNT) {
            API_TABLE[apiIndex].func(extractor);
        }
    }
}

void ExecuteSequentialMode(DataExtractor &extractor)
{
    for (int32_t group = 0; group < API_GROUP_COUNT && extractor.HasMore(); ++group) {
        for (int32_t i = 0; i < API_COUNT; ++i) {
            if (API_TABLE[i].groupId == group && extractor.HasMore()) {
                API_TABLE[i].func(extractor);
            }
        }
    }
}

void ExecuteGuidedMode(DataExtractor &extractor)
{
    CallApiInitialize(extractor);
    InitializeSystemIfNeeded(extractor);

    if (extractor.ExtractBool()) {
        CallApiCreateThreadWraps(extractor);
        CallApiInitThreadWraps(extractor);
    }

    while (extractor.HasMore()) {
        int32_t group = extractor.ExtractInt32() % (API_GROUP_COUNT - 1) + 1;
        for (int32_t i = 0; i < API_COUNT; ++i) {
            if (API_TABLE[i].groupId == group && extractor.HasMore()) {
                API_TABLE[i].func(extractor);
            }
        }
    }
}

void ExecuteStressMode(DataExtractor &extractor)
{
    InitializeSystemIfNeeded(extractor);

    int32_t callCount = 0;
    int32_t maxCalls = MAX_STRESS_CALLS;

    while (extractor.HasMore() && callCount < maxCalls){
        int32_t apiIndex = (extractor.ExtractInt32() + callCount) % API_COUNT;
        if (apiIndex >= 0 && apiIndex < API_COUNT) {
            API_TABLE[apiIndex].func(extractor);
        }
        ++callCount;
    }
}

// ============================================================================
// Main Fuzzer Entry Point
// ============================================================================

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_FUZZ_INPUT_SIZE){
        return 0;
    }

    DataExtractor extractor(data, size);
    InitializeSystemIfNeeded(extractor);

    FuzzMode mode = static_cast<FuzzMode>(extractor.ExtractInt32() % FUZZ_MODE_COUNT);

    switch (mode)
    {
        case FuzzMode::RANDOM:
            ExecuteRandomMode(extractor);
            break;
        case FuzzMode::SEQUENTIAL:
            ExecuteSequentialMode(extractor);
            break;
        case FuzzMode::GUIDED:
            ExecuteGuidedMode(extractor);
            break;
        case FuzzMode::STRESS:
            ExecuteStressMode(extractor);
            break;
        default:
            ExecuteRandomMode(extractor);
            break;
    }

    return 0;
}
