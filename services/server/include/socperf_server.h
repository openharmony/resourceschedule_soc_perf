/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef SOC_PERF_SERVICES_SERVER_INCLUDE_SOCPERF_SERVER_H
#define SOC_PERF_SERVICES_SERVER_INCLUDE_SOCPERF_SERVER_H

#include "singleton.h"
#include "soc_perf_stub.h"
#include "socperf.h"
#include "system_ability.h"
#include "socperf_lru_cache.h"
#include "tokenid_kit.h"
#include "accesstoken_kit.h"

namespace OHOS {
namespace SOCPERF {
    using namespace OHOS::Security;
class SocPerfServer : public SystemAbility, public SocPerfStub,
    public std::enable_shared_from_this<SocPerfServer> {
DISALLOW_COPY_AND_MOVE(SocPerfServer);
DECLARE_SYSTEM_ABILITY(SocPerfServer);
DECLARE_DELAYED_SINGLETON(SocPerfServer);

public:
    /**
     * @brief Sending a performance request.
     *
     * @param cmdId Scene id defined in config file.
     * @param msg Additional string info, which is used for other extensions.
     */
    virtual ErrCode PerfRequest(int32_t cmdId, const std::string& msg) override;

    /**
     * @brief Sending a performance request.
     *
     * @param cmdId Scene id defined in config file.
     * @param onOffTag Indicates the start of end of a long-term frequency increase event.
     * @param msg Additional string info, which is used for other extensions.
     */
    virtual ErrCode PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg) override;

    /**
     * @brief Sending a power limit boost request.
     *
     * @param onOffTag Indicates the start of end of a power limit boost event.
     * @param msg Additional string info, which is used for other extensions.
     */
    virtual ErrCode PowerLimitBoost(bool onOffTag, const std::string& msg) override;

    /**
     * @brief Sending a thermal limit boost request.
     *
     * @param onOffTag Indicates the start of end of a thermal limit boost event.
     * @param msg Additional string info, which is used for other extensions.
     */
    virtual ErrCode ThermalLimitBoost(bool onOffTag, const std::string& msg) override;

    /**
     * @brief Sending a limit request.
     *
     * @param clientId Used to indentify the caller of frequency limiting, such as
     * the thermal module or power consumption module.
     * @param configs Indicates the specific value to be limited.
     * @param msg Additional string info, which is used for other extensions.
     */
    virtual ErrCode LimitRequest(int32_t clientId,
        const std::vector<int32_t>& tags, const std::vector<int64_t>& configs, const std::string& msg) override;

    /**
     * @brief set socperf server status, enable or disable
     *
     * @param status true means enable socperfserver, false means disable socperfserver
     * @param msg the reason why we need change socperfserver status
     */
    virtual ErrCode SetRequestStatus(bool status, const std::string& msg) override;

    /**
     * @brief set thermal level intermal for perfquest
     *
     * @param level thermal level
     */
    virtual ErrCode SetThermalLevel(int32_t level) override;

    /**
     * @brief send the device mode, enable or disable
     *
     * @param mode the mode which will to be changed
     * @param status true means socperfserver enter the device mode, false quit the device mode
     */
    virtual ErrCode RequestDeviceMode(const std::string& mode, bool status) override;

    /**
     * @brief get cmd Id count, cmdID is trigger, its count++
     * @param msg the reason
     * @param funcResult return cmdId count, as 10000:xx,10001:xx
     */
    virtual ErrCode RequestCmdIdCount(const std::string& msg, std::string& funcResult) override;
    int32_t Dump(int32_t fd, const std::vector<std::u16string>& args) override;

public:
    SocPerfServer(int32_t systemAbilityId, bool runOnCreate);

protected:
    void OnStart() override;
    void OnStop() override;

private:
    SocPerf socPerf;
    std::mutex permissionCacheMutex_;
    bool AllowDump();
    bool HasPerfPermission();
    SocPerfLRUCache<AccessToken::AccessTokenID, int32_t> permissionCache_;
};
} // namespace SOCPERF
} // namespace OHOS

#endif // SOC_PERF_SERVICES_SERVER_INCLUDE_SOCPERF_SERVER_H
