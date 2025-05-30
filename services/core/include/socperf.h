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

#ifndef SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_H
#define SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_H

#include <set>
#include "libxml/tree.h"
#include "socperf_thread_wrap.h"
#include "socperf_config.h"

namespace OHOS {
namespace SOCPERF {
class SocPerf {
public:
    bool Init();
    void PerfRequest(int32_t cmdId, const std::string& msg);
    void PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg);
    void PowerLimitBoost(bool onOffTag, const std::string& msg);
    void ThermalLimitBoost(bool onOffTag, const std::string& msg);
    void LimitRequest(int32_t clientId,
        const std::vector<int32_t>& tags, const std::vector<int64_t>& configs, const std::string& msg);
    void SetRequestStatus(bool status, const std::string& msg);
    void SetThermalLevel(int32_t level);
    void RequestDeviceMode(const std::string& mode, bool status);
    std::string RequestCmdIdCount(const std::string& msg);
public:
    SocPerf();
    ~SocPerf();

private:
    bool enabled_ = false;
    std::shared_ptr<SocPerfThreadWrap> socperfThreadWrap_;
    std::set<std::string> recordDeviceMode_;
    std::vector<std::unordered_map<int32_t, int32_t>> limitRequest_ =
        std::vector<std::unordered_map<int32_t, int32_t>>(ACTION_TYPE_MAX);
    volatile bool perfRequestEnable_ = true;
    int32_t thermalLvl_ = DEFAULT_THERMAL_LVL;
    bool batteryLimitStatus_ = false;
    bool powerLimitStatus_ = false;
    SocPerfConfig &socPerfConfig_ = SocPerfConfig::GetInstance();
    std::unordered_map<int32_t, uint32_t> boostCmdCount_;
    std::unordered_map<int32_t, uint64_t> boostTime_;
private:
    std::mutex mutex_;
    std::mutex mutexDeviceMode_;
    std::mutex mutexBoostCmdCount_;
    std::mutex mutexBoostTime_;
    bool CreateThreadWraps();
    void InitThreadWraps();
    void DoFreqActions(std::shared_ptr<Actions> actions, int32_t onOff, int32_t actionType);
    std::shared_ptr<ResActionItem> DoPerfRequestThremalLvl(int32_t cmdId, std::shared_ptr<Action> originAction,
        int32_t onOff, std::shared_ptr<ResActionItem> curItem, int64_t endTime);
    void SendLimitRequestEvent(int32_t clientId, int32_t resId, int64_t resValue);
    int32_t MatchDeviceModeCmd(int32_t cmdId, bool isTagOnOff);
    void SendLimitRequestEventOff(std::shared_ptr<SocPerfThreadWrap> threadWrap,
        int32_t clientId, int32_t resId, int32_t eventId);
    void SendLimitRequestEventOn(std::shared_ptr<SocPerfThreadWrap> threadWrap,
        int32_t clientId, int32_t resId, int64_t resValue, int32_t eventId);
    void ClearAllAliveRequest();
    void UpdateCmdIdCount(int32_t cmdId);
    void CopyEvent(const int32_t oldCmdId, const int32_t newCmdId,
        std::unordered_map<int32_t, std::shared_ptr<Actions>>& perfActionsInfo);
    bool CheckTimeInterval(bool onOff, int32_t cmdId);
    bool CompleteEvent();
    std::string GetDeviceMode();
    int32_t GetMatchCmdId(int32_t cmdId, bool isTagOnOff);
    std::string MatchDeviceMode(const std::string& mode, bool status,
        const std::vector<std::shared_ptr<SceneItem>>& items);
    std::shared_ptr<Actions> GetActionsInfo(int32_t cmdId);
};
} // namespace SOCPERF
} // namespace OHOS
#endif // SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_H
