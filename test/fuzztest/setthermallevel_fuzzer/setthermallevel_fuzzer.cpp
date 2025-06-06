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

#include "setthermallevel_fuzzer.h"
#include "socperf_fuzz_mock.h"
#include "isoc_perf.h"

namespace OHOS {
namespace SOCPERF {
    bool TestSetThermalLeve(const uint8_t* data, size_t size)
    {
        SocperfStubTest socPerfStub;
        MessageParcel request;
        request.WriteInterfaceToken(SocPerfStub::GetDescriptor());
        request.WriteBuffer(data, size);
        MessageParcel reply;
        MessageOption option;
        uint32_t requestIpcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_SET_THERMAL_LEVEL);
        socPerfStub.OnRemoteRequest(requestIpcId, request, reply, option);
        return true;
    }
} // namespace SOCPERF
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::SOCPERF::MockProcess();
    OHOS::SOCPERF::TestSetThermalLeve(data, size);
    return 0;
}

