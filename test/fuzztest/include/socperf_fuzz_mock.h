/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef TEST_FUZZTEST_SOCPERF_FUZZ_MOCK_H
#define TEST_FUZZTEST_SOCPERF_FUZZ_MOCK_H

#include "socperf_stub.h"

namespace OHOS {
namespace SOCPERF {
class SocperfStubTest : public SocPerfStub {
public:
    SocperfStubTest() {}
    void PerfRequest(int32_t cmdId, const std::string &msg) override {}
    void PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string &msg) override {}
    void PowerLimitBoost(bool onOffTag, const std::string &msg) override {}
    void ThermalLimitBoost(bool onOffTag, const std::string &msg) override {}
    void LimitRequest(int32_t clientId, const std::vector <int32_t> &tags, const std::vector <int64_t> &configs,
        const std::string &msg) override {}
    void SetRequestStatus(bool status, const std::string &msg) override {}
    void SetThermalLevel(int32_t level) override {}
    void RequestDeviceMode(const std::string &mode, bool status) override {}
    std::string RequestCmdIdCount(const std::string& msg) override
    {
        return "";
    }
};
}
}
#endif