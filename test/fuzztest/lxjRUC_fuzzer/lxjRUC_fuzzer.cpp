/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "lxjRUC_fuzzer.h"
#include "socperf_client.h"
#include "securec.h"

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>


namespace FuzzUtils {

    // ============================================================================
    // Constants and Macros
    // ============================================================================

    

    constexpr int32_t MIN_FUZZ_INPUT_SIZE = 4;
    constexpr int32_t MAX_FUZZ_INPUT_SIZE = 4096;
    constexpr int32_t DEFAULT_STRING_LENGTH = 256;
    constexpr int32_t DEFAULT_MODE_STRING_LENGTH = 128;
    constexpr int32_t DEFAULT_VECTOR_SIZE = 10;
    constexpr int32_t MAX_STRING_LENGTH = 1024;
    constexpr int32_t MAX_VECTOR_SIZE = 100;

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

        uint8_t ExtractUInt8();
        int8_t ExtractInt8();
        uint16_t ExtractUInt16();
        int16_t ExtractInt16();
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

    uint8_t DataExtractor::ExtractUInt8()
    {
        const uint8_t *ptr = SafeRead(sizeof(uint8_t));
        if (ptr == nullptr)  {
            return 0;
        }
        uint8_t value = 0;
        int ret = memcpy_s(&value, sizeof(value), ptr, sizeof(uint8_t));
        if (ret != 0) {
            printf("memcpy_s failed in ExtractUInt8\n");
        }
        return value;
    }

    int8_t DataExtractor::ExtractInt8()
    {
        const uint8_t *ptr = SafeRead(sizeof(int8_t));
        if (ptr == nullptr)  {
            return 0;
        }
        int8_t value = 0;
        int ret = memcpy_s(&value, sizeof(value), ptr, sizeof(int8_t));
        if (ret != 0) {
            printf("memcpy_s failed in ExtractInt8\n");
        }
        return value;
    }

    uint16_t DataExtractor::ExtractUInt16()
    {
        const uint8_t *ptr = SafeRead(sizeof(uint16_t));
        if (ptr == nullptr)  {
            return 0;
        }
        uint16_t value = 0;
        int ret = memcpy_s(&value, sizeof(value), ptr, sizeof(uint16_t));
        if (ret != 0) {
            printf("memcpy_s failed in ExtractUInt16\n");
        }
        return value;
    }

    int16_t DataExtractor::ExtractInt16()
    {
        const uint8_t *ptr = SafeRead(sizeof(int16_t));
        if (ptr == nullptr)  {
            return 0;
        }
        int16_t value = 0;
        int ret = memcpy_s(&value, sizeof(value), ptr, sizeof(int16_t));
        if (ret != 0) {
            printf("memcpy_s failed in ExtractInt16\n");
        }
        return value;
    }

    int32_t DataExtractor::ExtractInt32()
    {
        const uint8_t *ptr = SafeRead(sizeof(int32_t));
        if (ptr == nullptr)  {
            return 0;
        }
        int32_t value = 0;
        int ret = memcpy_s(&value, sizeof(value), ptr, sizeof(int32_t));
        if (ret != 0) {
            printf("memcpy_s failed in ExtractInt32\n");
        }
        return value;
    }

    int64_t DataExtractor::ExtractInt64()
    {
        const uint8_t *ptr = SafeRead(sizeof(int64_t));
        if (ptr == nullptr)  {
            return 0;
        }
        int64_t value = 0;
        int ret = memcpy_s(&value, sizeof(value), ptr, sizeof(int64_t));
        if (ret != 0) {
            printf("memcpy_s failed in ExtractInt64\n");
        }
        return value;
    }

    bool DataExtractor::ExtractBool()
    {
        const uint8_t *ptr = SafeRead(sizeof(uint8_t));
        if (ptr == nullptr) {
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
        int ret = memcpy_s(&value, sizeof(value), ptr, sizeof(float));
        if (ret != 0) {
            printf("memcpy_s failed in ExtractInt64\n");
        }
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

        if (len > 0) {
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

using namespace OHOS::SOCPERF;
namespace OHOS {
    enum SocPerfApiId {
        PERF_REQUEST = 0,
        PERF_REQUEST_EX,
        POWER_LIMIT_BOOST,
        THERMAL_LIMIT_BOOST,
        LIMIT_REQUEST,
        SET_REQUEST_STATUS,
        SET_THERMAL_LEVEL,
        REQUEST_DEVICE_MODE,
        REQUEST_CMD_ID_COUNT,
        SOC_PERF_API_COUNT,
    };

    bool FuzzPerfRequest(SocPerfClient& client, DataExtractor& det)
    {
        int32_t cmdId = det.ExtractInt32();
        int32_t len = det.ExtractUInt16();
        if (len > MAX_STRING_LENGTH) {
            len = DEFAULT_STRING_LENGTH;
        }
        std::string msg = det.ExtractString(len);
        client.PerfRequest(cmdId, msg);
        return true;
    }

    bool FuzzPerfRequestEx(SocPerfClient& client, DataExtractor& det)
    {
        int32_t cmdId = det.ExtractInt32();
        bool onOffTag = det.ExtractBool();
        int32_t len = det.ExtractUInt16();
        if (len > MAX_STRING_LENGTH) {
            len = DEFAULT_STRING_LENGTH;
        }
        std::string msg = det.ExtractString(len);
        client.PerfRequestEx(cmdId, onOffTag, msg);
        return true;
    }

    bool FuzzPowerLimitBoost(SocPerfClient& client, DataExtractor& det)
    {
        bool onOffTag = det.ExtractBool();
        int32_t len = det.ExtractUInt16();
        if (len > MAX_STRING_LENGTH) {
            len = DEFAULT_STRING_LENGTH;
        }
        std::string msg = det.ExtractString(len);
        client.PowerLimitBoost(onOffTag, msg);
        return true;
    }

    bool FuzzThermalLimitBoost(SocPerfClient& client, DataExtractor& det)
    {
        bool onOffTag = det.ExtractBool();
        int32_t len = det.ExtractUInt16();
        if (len > MAX_STRING_LENGTH) {
            len = DEFAULT_STRING_LENGTH;
        }
        std::string msg = det.ExtractString(len);
        client.ThermalLimitBoost(onOffTag, msg);
        return true;
    }

    bool FuzzLimitRequest(SocPerfClient& client, DataExtractor& det)
    {
        int32_t clientId = det.ExtractInt32();
        int32_t vecLen = det.ExtractInt8();
        if (vecLen > MAX_VECTOR_SIZE) {
            vecLen = DEFAULT_VECTOR_SIZE;
        }
        std::vector<int32_t> tags = det.ExtractInt32Vector(vecLen);
        std::vector<int64_t> configs = det.ExtractInt64Vector(vecLen);
        int32_t strLen = det.ExtractUInt16();
        if (strLen > MAX_STRING_LENGTH) {
            strLen = DEFAULT_STRING_LENGTH;
        }
        std::string msg = det.ExtractString(strLen);
        client.LimitRequest(clientId, tags, configs, msg);
        return true;
    }

    bool FuzzSetRequestStatus(SocPerfClient& client, DataExtractor& det)
    {
        bool status = det.ExtractBool();
        int32_t strLen = det.ExtractUInt16();
        if (strLen > MAX_STRING_LENGTH) {
            strLen = DEFAULT_STRING_LENGTH;
        }
        std::string msg = det.ExtractString(strLen);
        client.SetRequestStatus(status, msg);
        return true;
    }

    bool FuzzSetThermalLevel(SocPerfClient& client, DataExtractor& det)
    {
        int32_t level = det.ExtractInt32();
        client.SetThermalLevel(level);
        return true;
    }

    bool FuzzRequestDeviceMode(SocPerfClient& client, DataExtractor& det)
    {
        int32_t strLen = det.ExtractUInt16();
        if (strLen > MAX_STRING_LENGTH) {
            strLen = DEFAULT_MODE_STRING_LENGTH;
        }
        std::string mode = det.ExtractString(strLen);
        bool status = det.ExtractBool();
        client.RequestDeviceMode(mode, status);
        return true;
    }

    bool FuzzRequestCmdIdCount(SocPerfClient& client, DataExtractor& det)
    {
        int32_t strLen = det.ExtractUInt16();
        if (strLen > MAX_STRING_LENGTH) {
            strLen = DEFAULT_STRING_LENGTH;
        }
        std::string msg = det.ExtractString(strLen);
        client.RequestCmdIdCount(msg);
        return true;
    }

    bool TestSocPerfClientAPI(const uint8_t* data, size_t size)
    {
        SocPerfClient& client = SocPerfClient::GetInstance();
        DataExtractor det(data, size);
        uint8_t choice = det.ExtractUInt8();
        choice = choice % SOC_PERF_API_COUNT;

        switch (choice) {
            case PERF_REQUEST:
                FuzzPerfRequest(client, det);
                break;
            case PERF_REQUEST_EX:
                FuzzPerfRequestEx(client, det);
                break;
            case POWER_LIMIT_BOOST:
                FuzzPowerLimitBoost(client, det);
                break;
            case THERMAL_LIMIT_BOOST:
                FuzzThermalLimitBoost(client, det);
                break;
            case LIMIT_REQUEST:
                FuzzLimitRequest(client, det);
                break;
            case SET_REQUEST_STATUS:
                FuzzSetRequestStatus(client, det);
                break;
            case SET_THERMAL_LEVEL:
                FuzzSetThermalLevel(client, det);
                break;
            case REQUEST_DEVICE_MODE:
                FuzzRequestDeviceMode(client, det);
                break;
            case REQUEST_CMD_ID_COUNT:
                FuzzRequestCmdIdCount(client, det);
                break;
            default:
                break;
        }
        
        const uint8_t MODE = 2;
        if (choice % MODE == 0) {
            client.ResetClient();
        }
        return true;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr || size < MIN_FUZZ_INPUT_SIZE) {
        return 0;
    }
    if (size > MAX_FUZZ_INPUT_SIZE) {
        return 0;
    }
    OHOS::TestSocPerfClientAPI(data, size);
    return 0;
}

