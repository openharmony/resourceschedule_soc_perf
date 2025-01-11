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

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "soc_perf_stub.h"
#include "token_setproc.h"

namespace OHOS {
namespace SOCPERF {
class SocperfStubTest : public SocPerfStub {
public:
    SocperfStubTest() {}
    ErrCode PerfRequest(int32_t cmdId, const std::string &msg) override
    {
        return ERR_OK;
    }
    ErrCode PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string &msg) override
    {
        return ERR_OK;
    }
    ErrCode PowerLimitBoost(bool onOffTag, const std::string &msg) override
    {
        return ERR_OK;
    }
    ErrCode ThermalLimitBoost(bool onOffTag, const std::string &msg) override
    {
        return ERR_OK;
    }
    ErrCode LimitRequest(int32_t clientId, const std::vector <int32_t> &tags, const std::vector <int64_t> &configs,
        const std::string &msg) override
    {
        return ERR_OK;
    }
    ErrCode SetRequestStatus(bool status, const std::string &msg) override
    {
        return ERR_OK;
    }
    ErrCode SetThermalLevel(int32_t level) override
    {
        return ERR_OK;
    }
    ErrCode RequestDeviceMode(const std::string &mode, bool status) override
    {
        return ERR_OK;
    }
    ErrCode RequestCmdIdCount(const std::string& msg, std::string& funcResult) override
    {
        return ERR_OK;
    }
};
void MockProcess()
{
    static const char *perms[] = {
        "ohos.permission.REPORT_RESOURCE_SCHEDULE_EVENT"
    };
    uint64_t tokenId;
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = sizeof(perms) / sizeof(perms[0]),
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "socperf_test",
        .aplStr = "system_core",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}
}
}
#endif