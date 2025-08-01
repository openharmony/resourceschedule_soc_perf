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

#include "socperf_server.h"
#include <file_ex.h>
#include <string_ex.h>
#include "ipc_skeleton.h"
#include "parameters.h"
#include "system_ability_definition.h"
#ifdef RES_SCHED_SA_INIT
#include "res_sa_init.h"
#endif

namespace OHOS {
namespace SOCPERF {
constexpr int32_t HIVIEW_UID = 1201;
const bool REGISTER_RESULT =
    SystemAbility::MakeAndRegisterAbility(DelayedSingleton<SocPerfServer>::GetInstance().get());
const int32_t ENG_MODE = OHOS::system::GetIntParameter("const.debuggable", 0);

SocPerfServer::SocPerfServer() : SystemAbility(SOC_PERF_SERVICE_SA_ID, true)
{
}

SocPerfServer::~SocPerfServer()
{
}

void SocPerfServer::OnStart()
{
    if (!socPerf.Init()) {
        SOC_PERF_LOGE("SocPerf Init FAILED");
        return;
    }
    if (!Publish(DelayedSingleton<SocPerfServer>::GetInstance().get())) {
        SOC_PERF_LOGE("Register SystemAbility for SocPerf FAILED.");
        return;
    }
    SOC_PERF_LOGI("SocPerf Init End");
}

void SocPerfServer::OnStop()
{
}

bool SocPerfServer::AllowDump()
{
    if (ENG_MODE == 0) {
        SOC_PERF_LOGE("Not allow to dump SocPerfServer, mode:%{public}d", ENG_MODE);
        return false;
    }
    Security::AccessToken::AccessTokenID tokenId = IPCSkeleton::GetFirstTokenID();
    int32_t res = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.DUMP");
    if (res != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        SOC_PERF_LOGE("Not allow to dump SocPerfServer, permission state:%{public}d", res);
        return false;
    }
    return true;
}

int32_t SocPerfServer::Dump(int32_t fd, const std::vector<std::u16string>& args)
{
    if (!AllowDump()) {
        return ERR_PERMISSION_DENIED;
    }
    std::vector<std::string> argsInStr;
    std::transform(args.begin(), args.end(), std::back_inserter(argsInStr),
        [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });
    std::string result;
    result.append("usage: soc_perf service dump [<options>]\n")
        .append("    1. PerfRequest(cmdId, msg)\n")
        .append("    2. PerfRequestEx(cmdId, onOffTag, msg)\n")
        .append("    3. LimitRequest(clientId, tags, configs, msg)\n")
        .append("    -h: show the help.\n")
        .append("    -a: show all info.\n");
    if (!SaveStringToFd(fd, result)) {
        SOC_PERF_LOGE("Dump FAILED");
    }
    return ERR_OK;
}

ErrCode SocPerfServer::PerfRequest(int32_t cmdId, const std::string& msg)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.PerfRequest(cmdId, msg);
    return ERR_OK;
}

ErrCode SocPerfServer::PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.PerfRequestEx(cmdId, onOffTag, msg);
    return ERR_OK;
}

ErrCode SocPerfServer::PowerLimitBoost(bool onOffTag, const std::string& msg)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.PowerLimitBoost(onOffTag, msg);
    return ERR_OK;
}

ErrCode SocPerfServer::ThermalLimitBoost(bool onOffTag, const std::string& msg)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.ThermalLimitBoost(onOffTag, msg);
    return ERR_OK;
}

ErrCode SocPerfServer::LimitRequest(int32_t clientId,
    const std::vector<int32_t>& tags, const std::vector<int64_t>& configs, const std::string& msg)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.LimitRequest(clientId, tags, configs, msg);
    return ERR_OK;
}

ErrCode SocPerfServer::SetRequestStatus(bool status, const std::string &msg)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.SetRequestStatus(status, msg);
    return ERR_OK;
}

ErrCode SocPerfServer::SetThermalLevel(int32_t level)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.SetThermalLevel(level);
    return ERR_OK;
}
ErrCode SocPerfServer::RequestDeviceMode(const std::string& mode, bool status)
{
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    socPerf.RequestDeviceMode(mode, status);
    return ERR_OK;
}

ErrCode SocPerfServer::RequestCmdIdCount(const std::string& msg, std::string& funcResult)
{
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    if ((ENG_MODE == 0 && callingUid != HIVIEW_UID)) {
        SOC_PERF_LOGE("not have right to do RequestCmdIdCount");
        return ERR_PERMISSION_DENIED;
    }
    if (!HasPerfPermission()) {
        return ERR_PERMISSION_DENIED;
    }
    funcResult = socPerf.RequestCmdIdCount(msg);
    return ERR_OK;
}

const std::string NEEDED_PERMISSION = "ohos.permission.REPORT_RESOURCE_SCHEDULE_EVENT";

bool SocPerfServer::HasPerfPermission()
{
    uint32_t accessToken = IPCSkeleton::GetCallingTokenID();
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(accessToken);
    if (int(tokenType) == OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP) {
        uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
        if (!Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(fullTokenId)) {
            SOC_PERF_LOGE("Invalid Permission to SocPerf");
            return false;
        }
    }
    int32_t hasPermission = -1;
    std::lock_guard<std::mutex> lock(permissionCacheMutex_);
    if (permissionCache_.get(accessToken, hasPermission)) {
        return hasPermission == 0;
    }
    hasPermission = AccessToken::AccessTokenKit::VerifyAccessToken(accessToken, NEEDED_PERMISSION);
    permissionCache_.put(accessToken, hasPermission);
    if (hasPermission != 0) {
        SOC_PERF_LOGE("SocPerf: not have Permission");
        return false;
    }
#ifdef RES_SCHED_SA_INIT
    int32_t clientPId = IPCSkeleton::GetCallingPid();
    ResourceSchedule::ResSchedIpcThread::GetInstance().SetQos(clientPId);
#endif
    return true;
}
} // namespace SOCPERF
} // namespace OHOS
