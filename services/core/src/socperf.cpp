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

#include "parameters.h"
#include "socperf_trace.h"

namespace OHOS {
namespace SOCPERF {
namespace {
    const int64_t TIME_INTERVAL = 8;
    const int32_t CANCEL_CMDID_PREFIX = 100000;
    const std::string DEFAULT_MODE = "default";
    const std::string SPLIT_COLON = ":";
    const std::string ACTION_MODE_STRING = "actionmode";
    const std::string WEAK_ACTION_STRING = "weakaction";
    const int32_t DEVICEMODE_PARAM_NUMBER = 2;
    const int32_t MODE_TYPE_INDEX = 0;
    const int32_t MODE_NAME_INDEX = 1;
    const int32_t PERF_REQUEST_CMD_ID_EVENT_FLING           = 10008;
    const int32_t PERF_REQUEST_CMD_ID_EVENT_TOUCH_DOWN      = 10010;
    const int32_t PERF_REQUEST_CMD_ID_EVENT_TOUCH_UP        = 10040;
    const int32_t PERF_REQUEST_CMD_ID_EVENT_DRAG            = 10092;

}
SocPerf::SocPerf()
{
}

SocPerf::~SocPerf()
{
    socperfThreadWrap_ = nullptr;
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
    CompleteEvent();
    return true;
}

bool SocPerf::CreateThreadWraps()
{
    socperfThreadWrap_ = std::make_shared<SocPerfThreadWrap>();
    if (!socperfThreadWrap_) {
        SOC_PERF_LOGE("Failed to Create socPerfThreadWrap");
        return false;
    }
    SOC_PERF_LOGD("Success to Create All threadWrap threads");
    return true;
}

void SocPerf::InitThreadWraps()
{
    socperfThreadWrap_->InitResourceNodeInfo();
}

bool SocPerf::CompleteEvent()
{
    if (socPerfConfig_.perfActionsInfo_.find(PERF_REQUEST_CMD_ID_EVENT_TOUCH_DOWN) !=
        socPerfConfig_.perfActionsInfo_.end() &&
        socPerfConfig_.perfActionsInfo_.find(PERF_REQUEST_CMD_ID_EVENT_TOUCH_UP) ==
        socPerfConfig_.perfActionsInfo_.end()) {
        SOC_PERF_LOGI("complete event %{public}d", PERF_REQUEST_CMD_ID_EVENT_TOUCH_UP);
        CopyEvent(PERF_REQUEST_CMD_ID_EVENT_TOUCH_DOWN, PERF_REQUEST_CMD_ID_EVENT_TOUCH_UP);
    }

    if (socPerfConfig_.perfActionsInfo_.find(PERF_REQUEST_CMD_ID_EVENT_FLING) !=
        socPerfConfig_.perfActionsInfo_.end() &&
        socPerfConfig_.perfActionsInfo_.find(PERF_REQUEST_CMD_ID_EVENT_DRAG) ==
        socPerfConfig_.perfActionsInfo_.end()) {
        SOC_PERF_LOGI("complete event %{public}d", PERF_REQUEST_CMD_ID_EVENT_DRAG);
        CopyEvent(PERF_REQUEST_CMD_ID_EVENT_FLING, PERF_REQUEST_CMD_ID_EVENT_DRAG);
    }
    return true;
}

void SocPerf::CopyEvent(const int32_t oldCmdId, const int32_t newCmdId)
{
    std::shared_ptr<Actions> oldActions = socPerfConfig_.perfActionsInfo_[oldCmdId];
    std::shared_ptr<Actions> newActions = std::make_shared<Actions>(newCmdId, oldActions->name);
    newActions->id = newCmdId;
    newActions->name = oldActions->name;
    newActions->actionList = oldActions->actionList;
    newActions->modeMap = oldActions->modeMap;
    newActions->isLongTimePerf = oldActions->isLongTimePerf;
    newActions->interaction = oldActions->interaction;
    socPerfConfig_.perfActionsInfo_[newCmdId] = newActions;
}

void SocPerf::PerfRequest(int32_t cmdId, const std::string& msg)
{
    if (!enabled_ || !perfRequestEnable_) {
        SOC_PERF_LOGD("SocPerf disabled!");
        return;
    }
    if (!CheckTimeInterval(true, cmdId)) {
        SOC_PERF_LOGD("cmdId %{public}d can not trigger, because time interval", cmdId);
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
    SOCPERF_TRACE_BEGIN(trace_str);
    DoFreqActions(socPerfConfig_.perfActionsInfo_[matchCmdId], EVENT_INVALID, ACTION_TYPE_PERF);
    SOCPERF_TRACE_END();
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
    if (!CheckTimeInterval(onOffTag, cmdId)) {
        SOC_PERF_LOGD("cmdId %{public}d can not trigger, because time interval", cmdId);
        return;
    }
    int32_t matchCmdId = MatchDeviceModeCmd(cmdId, true);
    SOC_PERF_LOGD("cmdId[%{public}d]matchCmdId[%{public}d]onOffTag[%{public}d]msg[%{public}s]",
        cmdId, matchCmdId, onOffTag, msg.c_str());

    std::string trace_str(__func__);
    trace_str.append(",cmdId[").append(std::to_string(matchCmdId)).append("]");
    trace_str.append(",onOff[").append(std::to_string(onOffTag)).append("]");
    trace_str.append(",msg[").append(msg).append("]");
    SOCPERF_TRACE_BEGIN(trace_str);
    DoFreqActions(socPerfConfig_.perfActionsInfo_[matchCmdId], onOffTag ? EVENT_ON : EVENT_OFF, ACTION_TYPE_PERF);
    SOCPERF_TRACE_END();
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
    SOCPERF_TRACE_BEGIN(trace_str);
    socperfThreadWrap_->UpdatePowerLimitBoostFreq(onOffTag);
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RSS, "LIMIT_BOOST",
                    OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                    "CLIENT_ID", ACTION_TYPE_POWER,
                    "ON_OFF_TAG", onOffTag);
    SOCPERF_TRACE_END();
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
    SOCPERF_TRACE_BEGIN(trace_str);
    socperfThreadWrap_->UpdateThermalLimitBoostFreq(onOffTag);
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RSS, "LIMIT_BOOST",
                    OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                    "CLIENT_ID", ACTION_TYPE_THERMAL,
                    "ON_OFF_TAG", onOffTag);
    SOCPERF_TRACE_END();
}

void SocPerf::SendLimitRequestEventOff(std::shared_ptr<SocPerfThreadWrap> threadWrap,
    int32_t clientId, int32_t resId, int32_t eventId)
{
    auto iter = limitRequest_[clientId].find(resId);
    if (iter != limitRequest_[clientId].end()
        && limitRequest_[clientId][resId] != INVALID_VALUE) {
        auto resAction = std::make_shared<ResAction>(
            limitRequest_[clientId][resId], 0, clientId, EVENT_OFF, -1, MAX_INT_VALUE);
        threadWrap->UpdateLimitStatus(eventId, resAction, resId);
        limitRequest_[clientId].erase(iter);
    }
}

void SocPerf::SendLimitRequestEventOn(std::shared_ptr<SocPerfThreadWrap> threadWrap,
    int32_t clientId, int32_t resId, int64_t resValue, int32_t eventId)
{
    if (resValue != INVALID_VALUE && resValue != RESET_VALUE) {
        auto resAction = std::make_shared<ResAction>(resValue, 0, clientId, EVENT_ON, -1, MAX_INT_VALUE);
        threadWrap->UpdateLimitStatus(eventId, resAction, resId);
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

    if (!socPerfConfig_.IsValidResId(realResId)) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    SendLimitRequestEventOff(socperfThreadWrap_, clientId, realResId, INNER_EVENT_ID_DO_FREQ_ACTION);
    SendLimitRequestEventOff(socperfThreadWrap_, clientId, levelResId, INNER_EVENT_ID_DO_FREQ_ACTION_LEVEL);
    SendLimitRequestEventOn(socperfThreadWrap_, clientId, resId, resValue, eventId);
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
    std::string trace_str(__func__);
    trace_str.append(",clientId[").append(std::to_string(clientId)).append("]");
    trace_str.append(",msg[").append(msg).append("]");
    for (int32_t i = 0; i < (int32_t)tags.size(); i++) {
        trace_str.append(",tags[").append(std::to_string(tags[i])).append("]");
        trace_str.append(",configs[").append(std::to_string(configs[i])).append("]");
        SendLimitRequestEvent(clientId, tags[i], configs[i]);
    }
    SOCPERF_TRACE_BEGIN(trace_str);
    SOC_PERF_LOGI("socperf limit %{public}s", trace_str.c_str());
    SOCPERF_TRACE_END();
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
    socperfThreadWrap_->ClearAllAliveRequest();
}

void SocPerf::SetThermalLevel(int32_t level)
{
    if (!enabled_) {
        SOC_PERF_LOGE("SocPerf disabled!");
        return;
    }
    std::string trace_str(__func__);
    trace_str.append(",level[").append(std::to_string(level)).append("]");
    SOCPERF_TRACE_BEGIN(trace_str);
    SOC_PERF_LOGI("ThermalLevel:%{public}d", level);
    SOCPERF_TRACE_END();
    thermalLvl_ = level;
    socperfThreadWrap_->thermalLvl_ = level;
}

std::shared_ptr<ResActionItem> SocPerf::DoPerfRequestThremalLvl(int32_t cmdId, std::shared_ptr<Action> originAction,
    int32_t onOff, std::shared_ptr<ResActionItem> curItem, int64_t endTime)
{
    if (curItem == nullptr) {
        return curItem;
    }

    if (socPerfConfig_.perfActionsInfo_[originAction->thermalCmdId_] == nullptr) {
        SOC_PERF_LOGE("cmd %{public}d is not exist", originAction->thermalCmdId_);
        return curItem;
    }

    std::shared_ptr<Actions> cmdConfig = socPerfConfig_.perfActionsInfo_[originAction->thermalCmdId_];

    // select the Nearest thermallevel action
    std::shared_ptr<Action> action = nullptr;
    for (auto iter = cmdConfig->actionList.begin(); iter != cmdConfig->actionList.end(); iter++) {
        if ((*iter)->thermalLvl_ <= thermalLvl_) {
            if (action == nullptr || action->thermalLvl_ <= (*iter)->thermalLvl_) {
                action = *iter;
            }
        }
    }
    if (action == nullptr) {
        return curItem;
    }

    for (int32_t i = 0; i < (int32_t)action->variable.size() - 1; i += RES_ID_AND_VALUE_PAIR) {
        if (!socPerfConfig_.IsValidResId(action->variable[i])) {
            continue;
        }

        auto resActionItem = std::make_shared<ResActionItem>(action->variable[i]);
        resActionItem->resAction = std::make_shared<ResAction>(action->variable[i + 1], originAction->duration,
            ACTION_TYPE_PERFLVL, onOff, cmdId, endTime);
        curItem->next = resActionItem;
        curItem = curItem->next;
    }
    return curItem;
}

void SocPerf::DoFreqActions(std::shared_ptr<Actions> actions, int32_t onOff, int32_t actionType)
{
    std::shared_ptr<ResActionItem> header = nullptr;
    std::shared_ptr<ResActionItem> curItem = nullptr;
    auto now = std::chrono::system_clock::now();
    int64_t curMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    for (auto iter = actions->actionList.begin(); iter != actions->actionList.end(); iter++) {
        std::shared_ptr<Action> action = *iter;
        if (action->duration == 0 && onOff == EVENT_INVALID) {
            continue;
        }
        int64_t endTime = action->duration == 0 ? MAX_INT_VALUE : curMs + action->duration;
        for (int32_t i = 0; i < (int32_t)action->variable.size() - 1; i += RES_ID_AND_VALUE_PAIR) {
            if (!socPerfConfig_.IsValidResId(action->variable[i])) {
                continue;
            }

            auto resActionItem = std::make_shared<ResActionItem>(action->variable[i]);
            resActionItem->resAction = std::make_shared<ResAction>(action->variable[i + 1], action->duration,
                actionType, onOff, actions->id, endTime);
            if (actions->interaction == false) {
                resActionItem->resAction->interaction = false;
            }
            if (curItem) {
                curItem->next = resActionItem;
            } else {
                header = resActionItem;
            }
            curItem = resActionItem;
        }
        if (action->thermalCmdId_ != INVALID_THERMAL_CMD_ID && thermalLvl_ >= socPerfConfig_.minThermalLvl_) {
            curItem = DoPerfRequestThremalLvl(actions->id, action, onOff, curItem, endTime);
        }
    }
    socperfThreadWrap_->DoFreqActionPack(header);
    socperfThreadWrap_->PostDelayTask(header);
}

void SocPerf::RequestDeviceMode(const std::string& mode, bool status)
{
    SOC_PERF_LOGD("device mode %{public}s status changed to %{public}d", mode.c_str(), status);

    if (mode.empty() || mode.length() > MAX_RES_MODE_LEN) {
        return;
    }

    std::vector<std::string> modeParamList = Split(mode, SPLIT_COLON);
    if (modeParamList.size() != DEVICEMODE_PARAM_NUMBER) {
        SOC_PERF_LOGE("device mode %{public}s format is wrong.", mode.c_str());
        return;
    }

    const std::string modeType = modeParamList[MODE_TYPE_INDEX];
    const std::string modeName = modeParamList[MODE_NAME_INDEX];
    if (modeType == ACTION_MODE_STRING && modeName == WEAK_ACTION_STRING) {
        socperfThreadWrap_->SetWeakInteractionStatus(status);
        return;
    }
    auto iter = socPerfConfig_.sceneResourceInfo_.find(modeType);
    if (iter == socPerfConfig_.sceneResourceInfo_.end()) {
        SOC_PERF_LOGD("No matching device mode found.");
        return;
    }

    const std::shared_ptr<SceneResNode> sceneResNode = iter->second;
    const std::vector<std::shared_ptr<SceneItem>> items = sceneResNode->items;
    const int32_t persistMode = sceneResNode->persistMode;

    const std::string modeStr = MatchDeviceMode(modeName, status, items);
    if (persistMode == REPORT_TO_PERFSO && socPerfConfig_.scenarioFunc_) {
        const std::string msgStr = modeType + ":" + modeStr;
        SOC_PERF_LOGD("send deviceMode to PerfScenario : %{public}s", msgStr.c_str());
        socPerfConfig_.scenarioFunc_(msgStr);
    }
}

std::string SocPerf::MatchDeviceMode(const std::string& mode, bool status,
    const std::vector<std::shared_ptr<SceneItem>>& items)
{
    std::lock_guard<std::mutex> lock(mutexDeviceMode_);

    if (!status) {
        recordDeviceMode_.erase(mode);
        return DEFAULT_MODE;
    }

    std::string itemName = DEFAULT_MODE;
    for (const auto& iter : items) {
        if (iter->name == mode) {
            recordDeviceMode_.insert(mode);
            if (iter->req == REPORT_TO_PERFSO) {
                itemName = mode;
            }
        } else {
            recordDeviceMode_.erase(iter->name);
        }
    }
    return itemName;
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

    for (const auto& iter : actions->modeMap) {
        auto deviceMode = recordDeviceMode_.find(iter->mode);
        if (deviceMode != recordDeviceMode_.end()) {
            int32_t deviceCmdId = iter->cmdId;
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

bool SocPerf::CheckTimeInterval(bool onOff, int32_t cmdId)
{
    std::lock_guard<std::mutex> lock(mutexBoostTime_);
    auto now = std::chrono::system_clock::now();
    uint64_t curMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    int32_t cancelCmdId = cmdId + CANCEL_CMDID_PREFIX;
    int32_t recordCmdId = cmdId;
    if (onOff) {
        boostTime_[cancelCmdId] = 0;
    }
    if (!onOff) {
        recordCmdId = cancelCmdId;
    }
    if (boostTime_.find(recordCmdId) == boostTime_.end()) {
        boostTime_[recordCmdId] = curMs;
        return true;
    }
    if (curMs - boostTime_[recordCmdId] > TIME_INTERVAL) {
        boostTime_[recordCmdId] = curMs;
        return true;
    }
    return false;
}
} // namespace SOCPERF
} // namespace OHOS
