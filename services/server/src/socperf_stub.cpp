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

#include "socperf_stub.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "parameters.h"
#include "socperf_ipc_interface_code.h"
#include "socperf_log.h"
#include "tokenid_kit.h"

namespace OHOS {
namespace SOCPERF {
namespace {
    constexpr int32_t HIVIEW_UID = 1201;
    constexpr int32_t MSG_STRING_MAX_LEN = 1024;
    constexpr int32_t MSG_VECTOR_MAX_LEN = 1024;
    constexpr int32_t MSG_VECTOR_INVALID_LEN = 0;
    const int32_t ENG_MODE = OHOS::system::GetIntParameter("const.debuggable", 0);
}
int32_t SocPerfStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
    MessageParcel &reply, MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (GetDescriptor() != remoteDescriptor || !HasPerfPermission()) {
        return ERR_INVALID_STATE;
    }
    switch (code) {
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_PERF_REQUEST): {
            StubPerfRequest(data);
            break;
        }
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_PERF_REQUEST_EX): {
            StubPerfRequestEx(data);
            break;
        }
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_POWER_LIMIT_BOOST_FREQ): {
            StubPowerLimitBoost(data);
            break;
        }
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_THERMAL_LIMIT_BOOST_FREQ): {
            StubThermalLimitBoost(data);
            break;
        }
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_LIMIT_REQUEST): {
            StubLimitRequest(data);
            break;
        }
        default:
            return OnRemoteRequestExt(code, data, reply, option);
    }
    return ERR_OK;
}

int32_t SocPerfStub::OnRemoteRequestExt(uint32_t code, MessageParcel &data,
    MessageParcel &reply, MessageOption &option)
{
    switch (code) {
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_SET_STATUS): {
            StubSetRequestStatus(data);
            break;
        }
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_SET_THERMAL_LEVEL): {
            StubSetThermalLevel(data);
            break;
        }
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_SET_DEVICE_MODE): {
            StubRequestDeviceMode(data);
            break;
        }
        case static_cast<uint32_t>(SocPerfInterfaceCode::TRANS_IPC_ID_REQUEST_CMDID_COUNT): {
            StubRequestDeviceMode(data, reply);
            break;
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return ERR_OK;
}

int32_t SocPerfStub::StubPerfRequest(MessageParcel &data)
{
    int32_t cmdId = 0;
    READ_PARCEL(data, Int32, cmdId, ERR_INVALID_STATE, SocPerfStub);

    std::string msg;
    READ_PARCEL(data, String, msg, ERR_INVALID_STATE, SocPerfStub);
    if (msg.length() > MSG_STRING_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    PerfRequest(cmdId, msg);
    return ERR_OK;
}

int32_t SocPerfStub::StubPerfRequestEx(MessageParcel &data)
{
    int32_t cmdId = 0;
    READ_PARCEL(data, Int32, cmdId, ERR_INVALID_STATE, SocPerfStub);

    bool onOffTag = false;
    READ_PARCEL(data, Bool, onOffTag, ERR_INVALID_STATE, SocPerfStub);

    std::string msg;
    READ_PARCEL(data, String, msg, ERR_INVALID_STATE, SocPerfStub);
    if (msg.length() > MSG_STRING_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    PerfRequestEx(cmdId, onOffTag, msg);
    return ERR_OK;
}

int32_t SocPerfStub::StubPowerLimitBoost(MessageParcel &data)
{
    bool onOffTag = false;
    READ_PARCEL(data, Bool, onOffTag, ERR_INVALID_STATE, SocPerfStub);

    std::string msg;
    READ_PARCEL(data, String, msg, ERR_INVALID_STATE, SocPerfStub);
    if (msg.length() > MSG_STRING_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    PowerLimitBoost(onOffTag, msg);
    return ERR_OK;
}

int32_t SocPerfStub::StubThermalLimitBoost(MessageParcel &data)
{
    bool onOffTag = false;
    READ_PARCEL(data, Bool, onOffTag, ERR_INVALID_STATE, SocPerfStub);

    std::string msg;
    READ_PARCEL(data, String, msg, ERR_INVALID_STATE, SocPerfStub);
    if (msg.length() > MSG_STRING_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    ThermalLimitBoost(onOffTag, msg);
    return ERR_OK;
}

int32_t SocPerfStub::StubLimitRequest(MessageParcel &data)
{
    int32_t clientpid;
    READ_PARCEL(data, Int32, clientId, ERR_INVALID_STATE, SocPerfStub);

    std::vector<int32_t> tags;
    READ_PARCEL(data, Int32Vector, &tags, ERR_INVALID_STATE, SocPerfStub);
    if (tags.size() <= MSG_VECTOR_INVALID_LEN || tags.size() > MSG_VECTOR_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    std::vector<int64_t> configs;
    READ_PARCEL(data, Int32Vector, &configs, ERR_INVALID_STATE, SocPerfStub);
    if (configs.size() <= MSG_VECTOR_INVALID_LEN || configs.size() > MSG_VECTOR_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    std::string msg;
    READ_PARCEL(data, String, msg, ERR_INVALID_STATE, SocPerfStub);
    if (msg.length() > MSG_STRING_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    LimitRequest(clientId, tags, configs, msg);
    return ERR_OK;
}

int32_t SocPerfStub::StubSetRequestStatus(MessageParcel &data)
{
    bool requestEnable;
    READ_PARCEL(data, Bool, requestEnable, ERR_INVALID_STATE, SocPerfStub);

    std::string msg;
    READ_PARCEL(data, String, msg, ERR_INVALID_STATE, SocPerfStub);
    if (msg.length() > MSG_STRING_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    SetRequestStatus(requestEnable, msg);
    return ERR_OK;
}

int32_t SocPerfStub::StubSetThermalLevel(MessageParcel &data)
{
    int32_t levelId;
    READ_PARCEL(data, Int32, levelId, ERR_INVALID_STATE, SocPerfStub);

    SetThermalLevel(levelId);
    return ERR_OK;
}

int32_t SocPerfStub::StubRequestDeviceMode(MessageParcel &data)
{
    std::string mode;
    READ_PARCEL(data, String, mode, ERR_INVALID_STATE, SocPerfStub);
    if (msg.length() > MSG_STRING_MAX_LEN) {
        return ERR_INVALID_STATE;
    }

    bool status;
    READ_PARCEL(data, Bool, status, ERR_INVALID_STATE, SocPerfStub);

    RequestDeviceMode(mode, status);
    return ERR_OK;
}

int32_t SocPerfStub::StubRequestDeviceMode(MessageParcel &data, MessageParcel &reply)
{
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    std::string msg;
    if ((ENG_MODE == 0 && callingUid != HIVIEW_UID) || !data.ReadString(msg)) {
        SOC_PERF_LOGE("not have right to do RequestCmdIdCount");
        return ERR_INVALID_STATE;
    }
    if (!reply.WriteString(RequestCmdIdCount(msg))) {
        SOC_PERF_LOGE("write RequestCmdIdCount ret failed");
        return ERR_INVALID_STATE;
    }
    return ERR_OK;
}

bool SocPerfStub::HasPerfPermission()
{
    uint32_t accessToken = IPCSkeleton::GetCallingTokenID();
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(accessToken);
    if (int(tokenType) == OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP) {
        uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
        if (!Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(fullTokenId)) {
            SOC_PERF_LOGE("Invalid Permission to SocPerf, token[%{public}u] tokenType[%{public}d]",
                accessToken, (int)tokenType);
            return false;
        }
    }
    return true;
}
} // namespace SOCPERF
} // namespace OHOS
