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

#include "socperf.h"
#include "hisysevent.h"
#include "hitrace_meter.h"
#include "parameters.h"

namespace OHOS {
namespace SOCPERF {
SocPerf::SocPerf()
{
}

SocPerf::~SocPerf()
{
}

bool SocPerf::Init()
{
    if (!socPerfConfig_.Init()) {
        SOC_PERF_LOGE("Failed to init SocPerf config");
        return false;
    }

    if (!CreateThreadWraps()) {
        SOC_PERF_LOGE("Failed to create threadwraps threads");
        return false;
    }
    InitThreadWraps();
    enabled_ = true;
    return true;
}

bool SocPerf::CreateThreadWraps()
{
    socperfThreadWraps_ = std::vector<std::shared_ptr<SocPerfThreadWrap>>(MAX_QUEUE_NUM);
    for (int32_t i = 0; i < (int32_t)socperfThreadWraps_.size(); i++) {
        if (!socPerfConfig_.wrapSwitch_[i]) {
            socperfThreadWraps_[i] = nullptr;
            continue;
        }
#ifdef SOCPERF_ADAPTOR_FFRT
        auto socPerfThreadWrap = std::make_shared<SocPerfThreadWrap>();
#else
        auto runner = AppExecFwk::EventRunner::Create("socperf#" + std::to_string(i));
        if (!runner) {
            SOC_PERF_LOGE("Failed to Create EventRunner");
            return false;
        }
        auto socPerfThreadWrap = std::make_shared<SocPerfThreadWrap>(runner);
#endif
        if (!socPerfThreadWrap) {
            SOC_PERF_LOGE("Failed to Create socPerfThreadWrap");
            return false;
        }
        socperfThreadWraps_[i] = socPerfThreadWrap;
    }
    SOC_PERF_LOGD("Success to Create All threadWrap threads");
    return true;
}

void SocPerf::InitThreadWraps()
{
    for (auto iter = socPerfConfig_.resourceNodeInfo_.begin(); iter != socPerfConfig_.resourceNodeInfo_.end(); ++iter) {
        std::shared_ptr<ResourceNode> resourceNode = iter->second;
        auto threadWrap = GetThreadWrapByResId(resourceNode->id);
        if (!threadWrap) {
            continue;
        }
#ifdef SOCPERF_ADAPTOR_FFRT
        threadWrap->InitResourceNodeInfo(resourceNode);
#else
        auto event = AppExecFwk::InnerEvent::Get(INNER_EVENT_ID_INIT_RESOURCE_NODE_INFO, resourceNode);
        threadWrap->SendEvent(event);
#endif
    }
}

std::shared_ptr<SocPerfThreadWrap> SocPerf::GetThreadWrapByResId(int32_t resId) const
{
    if (!socPerfConfig_.IsValidResId(resId)) {
        return nullptr;
    }
    return socperfThreadWraps_[resId / socPerfConfig_.GetResIdNumsPerType(resId)];
}

void SocPerf::PerfRequest(int32_t cmdId, const std::string& msg)
{
    if (!enabled_ || !perfRequestEnable_) {
        SOC_PERF_LOGD("SocPerf disabled!");
        return;
    }
    if (socPerfConfig_.perfActionsInfo_.find(cmdId) == socPerfConfig_.perfActionsInfo_.end()) {
        SOC_PERF_LOGD("Invalid PerfRequest cmdId[%{public}d]", cmdId);
        return;
    }

    int32_t matchCmdId = MatchDeviceModeCmd(cmdId, false);
    SOC_PERF_LOGD("cmdId[%{public}d]matchCmdId[%{public}d]msg[%{public}s]", cmdId, matchCmdId, msg.c_str());

    std::string trace_str(__func__);
    trace_str.append(",cmdId[").append(std::to_string(matchCmdId)).append("]");
    trace_str.append(",msg[").append(msg).append("]");
    StartTrace(HITRACE_TAG_OHOS, trace_str, -1);
    DoFreqActions(socPerfConfig_.perfActionsInfo_[matchCmdId], EVENT_INVALID, ACTION_TYPE_PERF);
    FinishTrace(HITRACE_TAG_OHOS);
    UpdateCmdIdCount(cmdId);
}

void SocPerf::PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg)
{
    if (!enabled_ || !perfRequestEnable_) {
        SOC_PERF_LOGD("SocPerf disabled!");
        return;
    }
    if (socPerfConfig_.perfActionsInfo_.find(cmdId) == socPerfConfig_.perfActionsInfo_.end()) {
        SOC_PERF_LOGD("Invalid PerfRequestEx cmdId[%{public}d]", cmdId);
        return;
    }

    int32_t matchCmdId = MatchDeviceModeCmd(cmdId, true);
    SOC_PERF_LOGD("cmdId[%{public}d]matchCmdId[%{public}d]onOffTag[%{public}d]msg[%{public}s]",
        cmdId, matchCmdId, onOffTag, msg.c_str());

    std::string trace_str(__func__);
    trace_str.append(",cmdId[").append(std::to_string(matchCmdId)).append("]");
    trace_str.append(",onOff[").append(std::to_string(onOffTag)).append("]");
    trace_str.append(",msg[").append(msg).append("]");
    StartTrace(HITRACE_TAG_OHOS, trace_str, -1);
    DoFreqActions(socPerfConfig_.perfActionsInfo_[matchCmdId], onOffTag ? EVENT_ON : EVENT_OFF, ACTION_TYPE_PERF);
    FinishTrace(HITRACE_TAG_OHOS);
    if (onOffTag) {
        UpdateCmdIdCount(cmdId);
    }
}

void SocPerf::PowerLimitBoost(bool onOffTag, const std::string& msg)
{
    if (!enabled_) {
        SOC_PERF_LOGD("SocPerf disabled!");
        return;
    }
    SOC_PERF_LOGI("onOffTag[%{public}d]msg[%{public}s]", onOffTag, msg.c_str());

    std::string trace_str(__func__);
    trace_str.append(",onOff[").append(std::to_string(onOffTag)).append("]");
    trace_str.append(",msg[").append(msg).append("]");
    StartTrace(HITRACE_TAG_OHOS, trace_str, -1);
    for (auto threadWrap : socperfThreadWraps_) {
        if (threadWrap) {
#ifdef SOCPERF_ADAPTOR_FFRT
            threadWrap->UpdatePowerLimitBoostFreq(onOffTag);
#else
            auto event = AppExecFwk::InnerEvent::Get(INNER_EVENT_ID_POWER_LIMIT_BOOST_FREQ, onOffTag ? 1 : 0);
            threadWrap->SendEvent(event);
#endif
        }
    }
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RSS, "LIMIT_BOOST",
                    OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                    "CLIENT_ID", ACTION_TYPE_POWER,
                    "ON_OFF_TAG", onOffTag);
    FinishTrace(HITRACE_TAG_OHOS);
}

void SocPerf::ThermalLimitBoost(bool onOffTag, const std::string& msg)
{
    if (!enabled_) {
        SOC_PERF_LOGD("SocPerf disabled!");
        return;
    }
    SOC_PERF_LOGI("onOffTag[%{public}d]msg[%{public}s]", onOffTag, msg.c_str());
    std::string trace_str(__func__);
    trace_str.append(",onOff[").append(std::to_string(onOffTag)).append("]");
    trace_str.append(",msg[").append(msg).append("]");
    StartTrace(HITRACE_TAG_OHOS, trace_str, -1);
    for (auto threadWrap : socperfThreadWraps_) {
        if (threadWrap) {
#ifdef SOCPERF_ADAPTOR_FFRT
            threadWrap->UpdateThermalLimitBoostFreq(onOffTag);
#else
            auto event = AppExecFwk::InnerEvent::Get(INNER_EVENT_ID_THERMAL_LIMIT_BOOST_FREQ, onOffTag ? 1 : 0);
            threadWrap->SendEvent(event);
#endif
        }
    }
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RSS, "LIMIT_BOOST",
                    OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                    "CLIENT_ID", ACTION_TYPE_THERMAL,
                    "ON_OFF_TAG", onOffTag);
    FinishTrace(HITRACE_TAG_OHOS);
}

void SocPerf::SendLimitRequestEventOff(std::shared_ptr<SocPerfThreadWrap> threadWrap,
    int32_t clientId, int32_t resId, int32_t eventId)
{
    auto iter = limitRequest_[clientId].find(resId);
    if (iter != limitRequest_[clientId].end()
        && limitRequest_[clientId][resId] != INVALID_VALUE) {
        auto resAction = std::make_shared<ResAction>(
            limitRequest_[clientId][resId], 0, clientId, EVENT_OFF, -1, MAX_INT_VALUE);
#ifdef SOCPERF_ADAPTOR_FFRT
        threadWrap->UpdateLimitStatus(eventId, resAction, resId);
#else
        auto event = AppExecFwk::InnerEvent::Get(eventId, resAction, resId);
        threadWrap->SendEvent(event);
#endif
        limitRequest_[clientId].erase(iter);
    }
}

void SocPerf::SendLimitRequestEventOn(std::shared_ptr<SocPerfThreadWrap> threadWrap,
    int32_t clientId, int32_t resId, int64_t resValue, int32_t eventId)
{
    if (resValue != INVALID_VALUE && resValue != RESET_VALUE) {
        auto resAction = std::make_shared<ResAction>(resValue, 0, clientId, EVENT_ON, -1, MAX_INT_VALUE);
#ifdef SOCPERF_ADAPTOR_FFRT
        threadWrap->UpdateLimitStatus(eventId, resAction, resId);
#else
        auto event = AppExecFwk::InnerEvent::Get(eventId, resAction, resId);
        threadWrap->SendEvent(event);
#endif
        limitRequest_[clientId].insert(std::pair<int32_t, int32_t>(resId, resValue));
    }
}

void SocPerf::SendLimitRequestEvent(int32_t clientId, int32_t resId, int64_t resValue)
{
    int32_t eventId = 0;
    int32_t realResId = 0;
    int32_t levelResId = 0;
    if (resId > RES_ID_ADDITION) {
        realResId = resId - RES_ID_ADDITION;
        levelResId = resId;
        eventId = INNER_EVENT_ID_DO_FREQ_ACTION_LEVEL;
    } else {
        realResId = resId;
        levelResId = resId + RES_ID_ADDITION;
        eventId = INNER_EVENT_ID_DO_FREQ_ACTION;
    }

    auto threadWrap = GetThreadWrapByResId(realResId);
    if (!threadWrap) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    SendLimitRequestEventOff(threadWrap, clientId, realResId, INNER_EVENT_ID_DO_FREQ_ACTION);
    SendLimitRequestEventOff(threadWrap, clientId, levelResId, INNER_EVENT_ID_DO_FREQ_ACTION_LEVEL);
    SendLimitRequestEventOn(threadWrap, clientId, resId, resValue, eventId);
}

void SocPerf::LimitRequest(int32_t clientId,
    const std::vector<int32_t>& tags, const std::vector<int64_t>& configs, const std::string& msg)
{
    if (!enabled_) {
        SOC_PERF_LOGE("SocPerf disabled!");
        return;
    }
    if (tags.size() != configs.size()) {
        SOC_PERF_LOGE("tags'size and configs' size must be the same!");
        return;
    }
    if (clientId <= (int32_t)ACTION_TYPE_PERF || clientId >= (int32_t)ACTION_TYPE_MAX) {
        SOC_PERF_LOGE("clientId must be between ACTION_TYPE_PERF and ACTION_TYPE_MAX!");
        return;
    }
    for (int32_t i = 0; i < (int32_t)tags.size(); i++) {
        SOC_PERF_LOGI("clientId[%{public}d],tags[%{public}d],configs[%{public}lld],msg[%{public}s]",
            clientId, tags[i], (long long)configs[i], msg.c_str());
        SendLimitRequestEvent(clientId, tags[i], configs[i]);
    }
}

void SocPerf::SetRequestStatus(bool status, const std::string& msg)
{
    SOC_PERF_LOGI("requestEnable is changed to %{public}d, the reason is %{public}s", status, msg.c_str());
    perfRequestEnable_ = status;
    /* disable socperf sever, we should clear all alive request to avoid high freq for long time */
    if (!perfRequestEnable_) {
        ClearAllAliveRequest();
    }
}

void SocPerf::ClearAllAliveRequest()
{
    if (!enabled_) {
        SOC_PERF_LOGE("SocPerf disabled!");
        return;
    }
    for (int32_t i = 0; i < MAX_QUEUE_NUM; ++i) {
        if (socperfThreadWraps_[i] == nullptr) {
            continue;
        }
#ifdef SOCPERF_ADAPTOR_FFRT
        socperfThreadWraps_[i]->ClearAllAliveRequest();
#else
        auto event = AppExecFwk::InnerEvent::Get(INNER_EVENT_ID_CLEAR_ALL_ALIVE_REQUEST);
        socperfThreadWraps_[i]->SendEvent(event);
#endif
    }
}

void SocPerf::SetThermalLevel(int32_t level)
{
    thermalLvl_ = level;
}

bool SocPerf::DoPerfRequestThremalLvl(int32_t cmdId, std::shared_ptr<Action> action, int32_t onOff)
{
    if (socPerfConfig_.perfActionsInfo_[action->thermalCmdId_] == nullptr) {
        SOC_PERF_LOGE("cmd %{public}d is not exist", action->thermalCmdId_);
        return false;
    }
    // init DoFreqActions param
    std::string thermalLvlTag = std::string("ThremalLvl_").append(std::to_string(action->thermalCmdId_))
        .append("_").append(std::to_string(thermalLvl_));
    std::shared_ptr<Actions> perfLvlActionCmd = std::make_shared<Actions>(cmdId, thermalLvlTag);
    std::shared_ptr<Action> perfLvlAction = std::make_shared<Action>();
    // perfrequest thermal level action's duration is same as trigger
    perfLvlAction->duration = action->duration;
    std::shared_ptr<Actions> cmdConfig = socPerfConfig_.perfActionsInfo_[action->thermalCmdId_];

    // select the Nearest thermallevel action
    std::shared_ptr<Action> actionConfig = *(cmdConfig->actionList.begin());
    for (auto iter = cmdConfig->actionList.begin(); iter != cmdConfig->actionList.end(); iter++) {
        if (perfLvlAction->thermalLvl_ <= (*iter)->thermalLvl_ && (*iter)->thermalLvl_ <= thermalLvl_) {
            actionConfig = *iter;
        }
    }
    if (thermalLvl_ < actionConfig->thermalLvl_) {
        SOC_PERF_LOGE("thermal level is too low to trigger perf request level");
        return false;
    }

    // fill in the item of perfLvlAction
    perfLvlAction->thermalLvl_ = actionConfig->thermalLvl_;
    perfLvlAction->thermalCmdId_ = INVALID_THERMAL_CMD_ID;
    for (uint32_t i = 0; i < actionConfig->variable.size(); i++) {
        perfLvlAction->variable.push_back(actionConfig->variable[i]);
    }
    perfLvlActionCmd->actionList.push_back(perfLvlAction);

    // send cmd to socperf server wrapper
    DoFreqActions(perfLvlActionCmd, onOff, ACTION_TYPE_PERFLVL);
    return true;
}

void SocPerf::DoFreqActions(std::shared_ptr<Actions> actions, int32_t onOff, int32_t actionType)
{
    std::shared_ptr<ResActionItem> header[MAX_QUEUE_NUM] = { nullptr };
    std::shared_ptr<ResActionItem> curItem[MAX_QUEUE_NUM] = { nullptr };
    auto now = std::chrono::system_clock::now();
    int64_t curMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    for (auto iter = actions->actionList.begin(); iter != actions->actionList.end(); iter++) {
        std::shared_ptr<Action> action = *iter;
        // process thermal level
        if (action->thermalCmdId_ != INVALID_THERMAL_CMD_ID && thermalLvl_ > MIN_THERMAL_LVL) {
            DoPerfRequestThremalLvl(actions->id, action, onOff);
        }
        for (int32_t i = 0; i < (int32_t)action->variable.size() - 1; i += RES_ID_AND_VALUE_PAIR) {
            if (!socPerfConfig_.IsValidResId(action->variable[i])) {
                continue;
            }
            auto resActionItem = std::make_shared<ResActionItem>(action->variable[i]);
            int64_t endTime = action->duration == 0 ? MAX_INT_VALUE : curMs + action->duration;
            resActionItem->resAction = std::make_shared<ResAction>(action->variable[i + 1], action->duration,
                actionType, onOff, actions->id, endTime);
            int32_t id = action->variable[i] / socPerfConfig_.GetResIdNumsPerType(action->variable[i]);
            if (curItem[id]) {
                curItem[id]->next = resActionItem;
            } else {
                header[id] = resActionItem;
            }
            curItem[id] = resActionItem;
        }
    }
    for (int32_t i = 0; i < MAX_QUEUE_NUM; ++i) {
        if (!socperfThreadWraps_[i] || !header[i]) {
            continue;
        }
#ifdef SOCPERF_ADAPTOR_FFRT
        socperfThreadWraps_[i]->DoFreqActionPack(header[i]);
#else
        auto event = AppExecFwk::InnerEvent::Get(INNER_EVENT_ID_DO_FREQ_ACTION_PACK, header[i]);
        socperfThreadWraps_[i]->SendEvent(event);
#endif
        std::shared_ptr<ResActionItem> queueHead = header[i];
        while (queueHead) {
#ifdef SOCPERF_ADAPTOR_FFRT
            socperfThreadWraps_[i]->PostDelayTask(queueHead->resId, queueHead->resAction);
#else
            auto event = AppExecFwk::InnerEvent::Get(INNER_EVENT_ID_DO_FREQ_ACTION_DELAYED, queueHead->resAction,
                queueHead->resId);
            socperfThreadWraps_[i]->SendEvent(event, queueHead->resAction->duration);
#endif
            queueHead = queueHead->next;
        }
    }
}

void SocPerf::RequestDeviceMode(const std::string& mode, bool status)
{
    SOC_PERF_LOGD("device mode %{public}s status changed to %{public}d", mode.c_str(), status);

    if (mode.empty() || mode.length() > MAX_RES_MODE_LEN) {
        return;
    }

    auto iter = MUTEX_MODE.find(mode);
    std::lock_guard<std::mutex> lock(mutexDeviceMode_);
    if (status) {
        if (iter != MUTEX_MODE.end()) {
            for (auto res : iter->second) {
                recordDeviceMode_.erase(res);
            }
        }
        recordDeviceMode_.insert(mode);
    } else {
        recordDeviceMode_.erase(mode);
    }
}

int32_t SocPerf::MatchDeviceModeCmd(int32_t cmdId, bool isTagOnOff)
{
    std::shared_ptr<Actions> actions = socPerfConfig_.perfActionsInfo_[cmdId];
    if (actions->modeMap.empty() || (isTagOnOff && actions->isLongTimePerf)) {
        return cmdId;
    }

    std::lock_guard<std::mutex> lock(mutexDeviceMode_);
    if (recordDeviceMode_.empty()) {
        return cmdId;
    }

    for (auto mode : recordDeviceMode_) {
        auto iter = actions->modeMap.find(mode);
        if (iter != actions->modeMap.end()) {
            int32_t deviceCmdId = iter->second;
            if (socPerfConfig_.perfActionsInfo_.find(deviceCmdId) == socPerfConfig_.perfActionsInfo_.end()) {
                SOC_PERF_LOGW("Invaild actions cmdid %{public}d", deviceCmdId);
                return cmdId;
            }
            if (isTagOnOff && socPerfConfig_.perfActionsInfo_[deviceCmdId]->isLongTimePerf) {
                SOC_PERF_LOGD("long time perf not match cmdId %{public}d", deviceCmdId);
                return cmdId;
            }
            return deviceCmdId;
        }
    }
    return cmdId;
}

void SocPerf::UpdateCmdIdCount(int32_t cmdId)
{
    std::lock_guard<std::mutex> lock(mutexBoostCmdCount_);
    if (boostCmdCount_.find(cmdId) == boostCmdCount_.end()) {
        boostCmdCount_[cmdId] = 0;
    }
    boostCmdCount_[cmdId]++;
}

std::string SocPerf::RequestCmdIdCount(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(mutexBoostCmdCount_);
    std::stringstream ret;
    for (const auto& pair : boostCmdCount_) {
        if (ret.str().length() > 0) {
            ret << ",";
        }
        ret << pair.first << ":" << pair.second;
    }
    return ret.str();
}
} // namespace SOCPERF
} // namespace OHOS
