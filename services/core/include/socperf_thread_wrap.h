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

#ifndef SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_THREAD_WRAP_H
#define SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_THREAD_WRAP_H

#ifdef SOCPERF_ADAPTOR_FFRT
#include "ffrt.h"
#include "ffrt_inner.h"
#else
#include "event_handler.h"
#endif
#include "socperf_common.h"
#include "socperf_config.h"
namespace OHOS { namespace SOCPERF { class GovResNode; } }
namespace OHOS { namespace SOCPERF { class ResAction; } }
namespace OHOS { namespace SOCPERF { class ResNode; } }
namespace OHOS { namespace SOCPERF { class ResStatus; } }

namespace OHOS {
namespace SOCPERF {
enum SocPerfInnerEvent : uint32_t {
    INNER_EVENT_ID_INIT_RESOURCE_NODE_INFO = 0,
    INNER_EVENT_ID_DO_FREQ_ACTION,
    INNER_EVENT_ID_DO_FREQ_ACTION_PACK,
    INNER_EVENT_ID_DO_FREQ_ACTION_DELAYED,
    INNER_EVENT_ID_POWER_LIMIT_BOOST_FREQ,
    INNER_EVENT_ID_THERMAL_LIMIT_BOOST_FREQ,
    INNER_EVENT_ID_DO_FREQ_ACTION_LEVEL,
    INNER_EVENT_ID_CLEAR_ALL_ALIVE_REQUEST
};

enum RssExeEventType {
    SOCPERF_EVENT_IVALID = 0,
    SOCPERF_EVENT_WIRTE_NODE = 1,
};

namespace {
    const std::string QOSID_STRING = "qosId";
    const std::string VALUE_STRING = "value";
}

#ifdef SOCPERF_ADAPTOR_FFRT
class SocPerfThreadWrap {
public:
    explicit SocPerfThreadWrap();
    ~SocPerfThreadWrap();
#else
class SocPerfThreadWrap : public AppExecFwk::EventHandler {
public:
    explicit SocPerfThreadWrap(const std::shared_ptr<AppExecFwk::EventRunner>& runner);
    ~SocPerfThreadWrap() override;
    void ProcessEvent(const AppExecFwk::InnerEvent::Pointer& event) override;
#endif
    void InitResourceNodeInfo();
    void DoFreqActionPack(std::shared_ptr<ResActionItem> head);
    void UpdatePowerLimitBoostFreq(bool powerLimitBoost);
    void UpdateThermalLimitBoostFreq(bool thermalLimitBoost);
    void UpdateLimitStatus(int32_t eventId, std::shared_ptr<ResAction> resAction, int32_t resId);
#ifdef SOCPERF_ADAPTOR_FFRT
    void PostDelayTask(std::shared_ptr<ResActionItem> queueHead);
#else
    void PostDelayTask(int32_t resId, std::shared_ptr<ResAction> resAction);
#endif
    void ClearAllAliveRequest();
public:
    int32_t thermalLvl_ = INVALID_THERMAL_LVL;

private:
    static const int32_t SCALES_OF_MILLISECONDS_TO_MICROSECONDS = 1000;
    std::unordered_map<int32_t, std::shared_ptr<ResStatus>> resStatusInfo_;
    SocPerfConfig &socPerfConfig_ = SocPerfConfig::GetInstance();
#ifdef SOCPERF_ADAPTOR_FFRT
    ffrt::queue socperfQueue_;
#endif
    bool powerLimitBoost_ = false;
    bool thermalLimitBoost_ = false;
    int boostResCnt = 0;

private:
    void InitResStatus();
    void SendResStatus();
    void ReportToPerfSo(std::vector<int32_t>& qosId, std::vector<int64_t>& value, std::vector<int64_t>& endTime);
    void ReportToRssExe(std::vector<int32_t>& qosId, std::vector<int64_t>& value, std::vector<int64_t>& endTime);
    bool GetResValueByLevel(int32_t resId, int32_t level, int64_t& resValue);
    void UpdateResActionList(int32_t resId, std::shared_ptr<ResAction> resAction, bool delayed);
    void UpdateResActionListByDelayedMsg(int32_t resId, int32_t type,
        std::shared_ptr<ResAction> resAction, std::shared_ptr<ResStatus> resStatus);
    void UpdateResActionListByInstantMsg(int32_t resId, int32_t type,
        std::shared_ptr<ResAction> resAction, std::shared_ptr<ResStatus> resStatus);
    void UpdateCandidatesValue(int32_t resId, int32_t type);
    void InnerArbitrateCandidatesValue(int32_t type, std::shared_ptr<ResStatus> resStatus);
    void ArbitrateCandidate(int32_t resId);
    void ArbitratePairRes(int32_t resId, bool perfRequestLimit);
    void ProcessLimitCase(int32_t resId);
    bool ArbitratePairResInPerfLvl(int32_t resId);
    void UpdatePairResValue(int32_t minResId, int64_t minResValue, int32_t maxResId, int64_t maxResValue);
    void UpdateCurrentValue(int32_t resId, int64_t value);
    bool ExistNoCandidate(int32_t resId, std::shared_ptr<ResStatus> resStatus);
    void DoFreqAction(int32_t resId, std::shared_ptr<ResAction> resAction);
    void DoFreqActionLevel(int32_t resId, std::shared_ptr<ResAction> resAction);
    void HandleResAction(int32_t resId, int32_t type,
        std::shared_ptr<ResAction> resAction, std::shared_ptr<ResStatus> resStatus);
#ifdef SOCPERF_ADAPTOR_FFRT
    void DoWeakInteraction(std::shared_ptr<Actions> actions, int32_t onOff, int32_t actionType);
    void WeakInteraction();
#endif
};
} // namespace SOCPERF
} // namespace OHOS

#endif // SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_THREAD_WRAP_H
