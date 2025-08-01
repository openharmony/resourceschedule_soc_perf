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
#include "socperf_thread_wrap.h"

#include <set>               // for set
#include <unistd.h>          // for open, write
#include <fcntl.h>           // for O_RDWR, O_CLOEXEC

#include "res_exe_type.h"
#include "res_sched_exe_client.h"
#include "socperf.h"
#include "socperf_trace.h"

namespace OHOS {
namespace SOCPERF {

SocPerfThreadWrap::SocPerfThreadWrap() : socperfQueue_("socperf", ffrt::queue_attr().qos(ffrt::qos_user_interactive))
{
}

SocPerfThreadWrap::~SocPerfThreadWrap()
{
}

void SocPerfThreadWrap::InitResourceNodeInfo()
{
    std::function<void()>&& initResourceNodeInfoFunc = [this]() {
        for (auto iter = socPerfConfig_.resourceNodeInfo_.begin();
            iter != socPerfConfig_.resourceNodeInfo_.end(); ++iter) {
            std::shared_ptr<ResourceNode> resourceNode = iter->second;
            if (resourceNode == nullptr) {
                continue;
            }
            auto resStatus = std::make_shared<ResStatus>();
            resStatusInfo_.insert(std::pair<int32_t, std::shared_ptr<ResStatus>>(resourceNode->id, resStatus));
        }
        InitResStatus();
    };
    socperfQueue_.submit(initResourceNodeInfoFunc);
}

void SocPerfThreadWrap::DoFreqActionPack(std::shared_ptr<ResActionItem> head)
{
    if (head == nullptr) {
        return;
    }
    std::function<void()>&& doFreqActionPackFunc = [this, head]() {
        std::shared_ptr<ResActionItem> queueHead = head;
        while (queueHead) {
            if (socPerfConfig_.IsValidResId(queueHead->resId)) {
                UpdateResActionList(queueHead->resId, queueHead->resAction, false);
            }
            queueHead = queueHead->next;
        }
        SendResStatus();
    };
    socperfQueue_.submit(doFreqActionPackFunc);
}

void SocPerfThreadWrap::UpdatePowerLimitBoostFreq(bool powerLimitBoost)
{
    std::function<void()>&& updatePowerLimitBoostFreqFunc = [this, powerLimitBoost]() {
        this->powerLimitBoost_ = powerLimitBoost;
        for (auto iter = resStatusInfo_.begin(); iter != resStatusInfo_.end(); ++iter) {
            if (resStatusInfo_[iter->first] == nullptr) {
                continue;
            }
            ArbitrateCandidate(iter->first);
        }
        SendResStatus();
    };
    socperfQueue_.submit(updatePowerLimitBoostFreqFunc);
}

void SocPerfThreadWrap::UpdateThermalLimitBoostFreq(bool thermalLimitBoost)
{
    std::function<void()>&& updateThermalLimitBoostFreqFunc = [this, thermalLimitBoost]() {
        this->thermalLimitBoost_ = thermalLimitBoost;
        for (auto iter = resStatusInfo_.begin(); iter != resStatusInfo_.end(); ++iter) {
            if (resStatusInfo_[iter->first] == nullptr) {
                continue;
            }
            ArbitrateCandidate(iter->first);
        }
        SendResStatus();
    };
    socperfQueue_.submit(updateThermalLimitBoostFreqFunc);
}

void SocPerfThreadWrap::UpdateLimitStatus(int32_t eventId, std::shared_ptr<ResAction> resAction, int32_t resId)
{
    if (resAction == nullptr) {
        return;
    }
    std::function<void()>&& updateLimitStatusFunc = [this, eventId, resId, resAction]() {
        if (eventId == INNER_EVENT_ID_DO_FREQ_ACTION) {
            DoFreqAction(resId, resAction);
        } else if (eventId == INNER_EVENT_ID_DO_FREQ_ACTION_LEVEL) {
            DoFreqActionLevel(resId, resAction);
        }
        SendResStatus();
        if (resAction->onOff && resStatusInfo_[resId] != nullptr) {
            HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RSS, "LIMIT_REQUEST",
                            OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                            "CLIENT_ID", resAction->type,
                            "RES_ID", resId,
                            "CONFIG", resStatusInfo_[resId]->candidate);
        }
    };
    socperfQueue_.submit(updateLimitStatusFunc);
}

void SocPerfThreadWrap::ClearAllAliveRequest()
{
    std::function<void()>&& updateLimitStatusFunc = [this]() {
        for (const auto& item : this->resStatusInfo_) {
            if (item.second == nullptr) {
                continue;
            }
            std::list<std::shared_ptr<ResAction>>& resActionList = item.second->resActionList[ACTION_TYPE_PERF];
            resActionList.clear();
            UpdateCandidatesValue(item.first, ACTION_TYPE_PERF);
        }
        SendResStatus();
    };
    socperfQueue_.submit(updateLimitStatusFunc);
}

void SocPerfThreadWrap::DoFreqAction(int32_t resId, std::shared_ptr<ResAction> resAction)
{
    if (!socPerfConfig_.IsValidResId(resId) || resAction == nullptr) {
        return;
    }
    UpdateResActionList(resId, resAction, false);
}

void SocPerfThreadWrap::InitResStatus()
{
    std::vector<int32_t> qosId;
    std::vector<int64_t> value;
    std::vector<int64_t> endTime;
    std::vector<int32_t> qosIdToRssEx;
    std::vector<int64_t> valueToRssEx;
    std::vector<int64_t> endTimeToRssEx;
    for (auto iter = resStatusInfo_.begin(); iter != resStatusInfo_.end(); ++iter) {
        int32_t resId = iter->first;
        if (socPerfConfig_.resourceNodeInfo_.find(resId) != socPerfConfig_.resourceNodeInfo_.end()) {
            if (socPerfConfig_.resourceNodeInfo_[resId]->persistMode == REPORT_TO_PERFSO) {
                qosId.push_back(resId);
                value.push_back(NODE_DEFAULT_VALUE);
                endTime.push_back(MAX_INT_VALUE);
            } else {
                qosIdToRssEx.push_back(resId);
                valueToRssEx.push_back(NODE_DEFAULT_VALUE);
                endTimeToRssEx.push_back(MAX_INT_VALUE);
            }
        }
    }
    ReportToPerfSo(qosId, value, endTime);
    ReportToRssExe(qosIdToRssEx, valueToRssEx, endTimeToRssEx);
}

void SocPerfThreadWrap::SetWeakInteractionStatus(bool enable)
{
    std::function<void()>&& weakInteractionFunc = [this, enable]() {
        std::string trace_str("SetWeakInteractionStatus");
        trace_str.append("[").append(enable ? "true" : "false").append("]");
        StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF, trace_str.c_str());
        weakInteractionStatus_ = enable;
        WeakInteraction();
        SOC_PERF_LOGI("SetWeakInteractionStatus is %{public}d.", enable);
        FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF);
    };
    socperfQueue_.submit(weakInteractionFunc);
}

void SocPerfThreadWrap::WeakInteraction()
{
    for (int i = 0; i < (int)socPerfConfig_.interAction_.size(); i++) {
        std::shared_ptr<InterAction> interAction = socPerfConfig_.interAction_[i];
        if (weakInteractionStatus_ && boostResCnt == 0 && interAction->status == BOOST_STATUS) {
            interAction->status = BOOST_END_STATUS;
            std::function<void()>&& updateLimitStatusFunc = [this, i]() {
                socPerfConfig_.interAction_[i]->status = WEAK_INTERACTION_STATUS;
                DoWeakInteraction(
                    socPerfConfig_.configPerfActionsInfo_[DEFAULT_CONFIG_MODE][socPerfConfig_.interAction_[i]->cmdId],
                    EVENT_ON, socPerfConfig_.interAction_[i]->actionType);
                std::string trace_str("WeakInteraction");
                trace_str.append(",cmdId[").append(std::to_string(socPerfConfig_.interAction_[i]->cmdId)).append("]");
                trace_str.append(",onOff[").append(std::to_string(EVENT_ON)).append("]");
                StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF, trace_str.c_str());
                FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF);
            };
            ffrt::task_attr taskAttr;
            taskAttr.delay(interAction->delayTime * SCALES_OF_MILLISECONDS_TO_MICROSECONDS);
            interAction->timerTask = socperfQueue_.submit_h(updateLimitStatusFunc, taskAttr);
        } else if ((!weakInteractionStatus_ || boostResCnt != 0) && interAction->status == WEAK_INTERACTION_STATUS) {
            interAction->status = BOOST_STATUS;
            DoWeakInteraction(
                socPerfConfig_.configPerfActionsInfo_[DEFAULT_CONFIG_MODE][interAction->cmdId],
                EVENT_OFF, interAction->actionType);
            std::string trace_str("WeakInteraction");
            trace_str.append(",cmdId[").append(std::to_string(interAction->cmdId)).append("]");
            trace_str.append(",onOff[").append(std::to_string(EVENT_OFF)).append("]");
            StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF, trace_str.c_str());
            FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF);
        } else if ((!weakInteractionStatus_ || boostResCnt != 0) && interAction->status == BOOST_END_STATUS) {
            interAction->status = BOOST_STATUS;
            if (interAction->timerTask != nullptr) {
                socperfQueue_.cancel(interAction->timerTask);
                interAction->timerTask = nullptr;
            }
        }
    }
}

void SocPerfThreadWrap::DoWeakInteraction(std::shared_ptr<Actions> actions, int32_t onOff, int32_t actionType)
{
    std::shared_ptr<ResActionItem> header = nullptr;
    std::shared_ptr<ResActionItem> curItem = nullptr;
    if (actions == nullptr) {
        return;
    }
    for (auto iter = actions->actionList.begin(); iter != actions->actionList.end(); iter++) {
        std::shared_ptr<Action> action = *iter;
        for (int32_t i = 0; i < (int32_t)action->variable.size() - 1; i += RES_ID_AND_VALUE_PAIR) {
            if (!socPerfConfig_.IsValidResId(action->variable[i])) {
                continue;
            }
            auto resActionItem = std::make_shared<ResActionItem>(action->variable[i]);
            resActionItem->resAction = std::make_shared<ResAction>(action->variable[i + 1], 0,
                actionType, onOff, actions->id, MAX_INT_VALUE);
            resActionItem->resAction->interaction = false;
            if (curItem) {
                curItem->next = resActionItem;
            } else {
                header = resActionItem;
            }
            curItem = resActionItem;
        }
    }
    DoFreqActionPack(header);
}

void SocPerfThreadWrap::SendResStatus()
{
    std::vector<int32_t> qosId;
    std::vector<int64_t> value;
    std::vector<int64_t> endTime;
    std::vector<int32_t> qosIdToRssEx;
    std::vector<int64_t> valueToRssEx;
    std::vector<int64_t> endTimeToRssEx;
    for (auto iter = resStatusInfo_.begin(); iter != resStatusInfo_.end(); ++iter) {
        int32_t resId = iter->first;
        std::shared_ptr<ResStatus> resStatus = iter->second;
        if (socPerfConfig_.resourceNodeInfo_.find(resId) != socPerfConfig_.resourceNodeInfo_.end() &&
            (resStatus->previousValue != resStatus->currentValue ||
            resStatus->previousEndTime != resStatus->currentEndTime)) {
            if (socPerfConfig_.resourceNodeInfo_[resId]->persistMode == REPORT_TO_PERFSO) {
                qosId.push_back(resId);
                value.push_back(resStatus->currentValue);
                endTime.push_back(resStatus->currentEndTime);
            } else {
                qosIdToRssEx.push_back(resId);
                valueToRssEx.push_back(resStatus->currentValue);
                endTimeToRssEx.push_back(resStatus->currentEndTime);
            }
            resStatus->previousValue = resStatus->currentValue;
            resStatus->previousEndTime = resStatus->currentEndTime;
            if (socPerfConfig_.resourceNodeInfo_[resId]->trace) {
                CountTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF,
                    socPerfConfig_.resourceNodeInfo_[resId]->name.c_str(),
                    resStatus->currentValue == MAX_INT32_VALUE ? NODE_DEFAULT_VALUE : resStatus->currentValue);
            }
        }
    }
    ReportToPerfSo(qosId, value, endTime);
    ReportToRssExe(qosIdToRssEx, valueToRssEx, endTimeToRssEx);

    WeakInteraction();
}

void SocPerfThreadWrap::ReportToPerfSo(std::vector<int32_t>& qosId, std::vector<int64_t>& value,
    std::vector<int64_t>& endTime)
{
    if (!socPerfConfig_.reportFunc_) {
        return;
    }
    if (qosId.size() > 0) {
        socPerfConfig_.reportFunc_(qosId, value, endTime, "");
        std::string log("send data to perf so");
        for (unsigned long i = 0; i < qosId.size(); i++) {
            log.append(",[id:").append(std::to_string(qosId[i]));
            log.append(", value:").append(std::to_string(value[i])).append("]");
        }
        StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF, log.c_str());
        FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF);
    }
}

void SocPerfThreadWrap::ReportToRssExe(std::vector<int32_t>& qosId, std::vector<int64_t>& value,
    std::vector<int64_t>& endTime)
{
    if (qosId.size() > 0) {
        nlohmann::json payload;
        payload[QOSID_STRING] = qosId;
        payload[VALUE_STRING] = value;
        ResourceSchedule::ResSchedExeClient::GetInstance().SendRequestAsync(
            ResourceSchedule::ResExeType::EWS_TYPE_SOCPERF_EXECUTOR_ASYNC_EVENT, SOCPERF_EVENT_WIRTE_NODE, payload);
        std::string log("send data to rssexe so");
        for (unsigned long i = 0; i < qosId.size(); i++) {
            log.append(",[id:").append(std::to_string(qosId[i]));
            log.append(", value:").append(std::to_string(value[i])).append("]");
        }
        StartTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF, log.c_str());
        FinishTraceEx(HITRACE_LEVEL_INFO, HITRACE_TAG_SOCPERF);
    }
}

void SocPerfThreadWrap::DoFreqActionLevel(int32_t resId, std::shared_ptr<ResAction> resAction)
{
    int32_t realResId = resId - RES_ID_ADDITION;
    if (!socPerfConfig_.IsValidResId(realResId) || !resAction) {
        return;
    }
    int32_t level = (int32_t)resAction->value;
    if (!GetResValueByLevel(realResId, level, resAction->value)) {
        return;
    }
    UpdateResActionList(realResId, resAction, false);
}

void SocPerfThreadWrap::PostDelayTask(std::shared_ptr<ResActionItem> queueHead)
{
    if (queueHead == nullptr) {
        return;
    }
    std::unordered_map<int32_t, std::vector<std::shared_ptr<ResActionItem>>> resActionMap;
    std::shared_ptr<ResActionItem> head = queueHead;
    while (head) {
        if (!head->resAction || head->resAction->duration == 0) {
            head = head->next;
            continue;
        }
        resActionMap[head->resAction->duration].push_back(head);
        head = head->next;
    }
    for (auto item : resActionMap) {
        ffrt::task_attr taskAttr;
        taskAttr.delay(item.first * SCALES_OF_MILLISECONDS_TO_MICROSECONDS);
        std::function<void()>&& postDelayTaskFunc = [this, item]() {
            for (uint32_t i = 0; i < item.second.size(); i++) {
                UpdateResActionList(item.second[i]->resId, item.second[i]->resAction, true);
            }
            SendResStatus();
        };
        socperfQueue_.submit(postDelayTaskFunc, taskAttr);
    }
}

bool SocPerfThreadWrap::GetResValueByLevel(int32_t resId, int32_t level, int64_t& resValue)
{
    if (socPerfConfig_.resourceNodeInfo_.find(resId) == socPerfConfig_.resourceNodeInfo_.end()
        || socPerfConfig_.resourceNodeInfo_[resId]->available.empty()) {
        SOC_PERF_LOGE("resId[%{public}d] is not valid.", resId);
        return false;
    }
    if (level < 0) {
        return false;
    }

    std::set<int64_t> available;
    for (auto a : socPerfConfig_.resourceNodeInfo_[resId]->available) {
        available.insert(a);
    }
    int32_t len = (int32_t)available.size();
    auto iter = available.begin();
    if (level < len) {
        std::advance(iter, len - 1 - level);
    }
    resValue = *iter;
    return true;
}

void SocPerfThreadWrap::UpdateResActionList(int32_t resId, std::shared_ptr<ResAction> resAction, bool delayed)
{
    std::shared_ptr<ResStatus> resStatus = resStatusInfo_[resId];
    int32_t type = resAction->type;

    if (delayed) {
        UpdateResActionListByDelayedMsg(resId, type, resAction, resStatus);
    } else {
        UpdateResActionListByInstantMsg(resId, type, resAction, resStatus);
    }
}

void SocPerfThreadWrap::UpdateResActionListByDelayedMsg(int32_t resId, int32_t type,
    std::shared_ptr<ResAction> resAction, std::shared_ptr<ResStatus> resStatus)
{
    for (auto iter = resStatus->resActionList[type].begin();
        iter != resStatus->resActionList[type].end(); ++iter) {
        if (resAction == *iter) {
            resStatus->resActionList[type].erase(iter);
            UpdateCandidatesValue(resId, type);
            if (resAction->interaction) {
                boostResCnt--;
            }
            break;
        }
    }
}

void SocPerfThreadWrap::HandleResAction(int32_t resId, int32_t type,
    std::shared_ptr<ResAction> resAction, std::shared_ptr<ResStatus> resStatus)
{
    for (auto iter = resStatus->resActionList[type].begin();
         iter != resStatus->resActionList[type].end(); ++iter) {
        if (resAction->TotalSame(*iter)) {
            resStatus->resActionList[type].erase(iter);
            if (resAction->interaction) {
                boostResCnt--;
            }
            break;
        }
    }
    resStatus->resActionList[type].push_back(resAction);
    UpdateCandidatesValue(resId, type);
    if (resAction->interaction) {
        boostResCnt++;
    }
}

void SocPerfThreadWrap::UpdateResActionListByInstantMsg(int32_t resId, int32_t type,
    std::shared_ptr<ResAction> resAction, std::shared_ptr<ResStatus> resStatus)
{
    switch (resAction->onOff) {
        case EVENT_INVALID:
        case EVENT_ON: {
            HandleResAction(resId, type, resAction, resStatus);
            break;
        }
        case EVENT_OFF: {
            for (auto iter = resStatus->resActionList[type].begin();
                iter != resStatus->resActionList[type].end(); ++iter) {
                if (resAction->PartSame(*iter) && (*iter)->onOff == EVENT_ON) {
                    resStatus->resActionList[type].erase(iter);
                    UpdateCandidatesValue(resId, type);
                    boostResCnt = boostResCnt - (resAction->interaction ? 1 : 0);
                    break;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void SocPerfThreadWrap::UpdateCandidatesValue(int32_t resId, int32_t type)
{
    std::shared_ptr<ResStatus> resStatus = resStatusInfo_[resId];
    int64_t prevValue = resStatus->candidatesValue[type];
    int64_t prevEndTime = resStatus->candidatesEndTime[type];

    if (resStatus->resActionList[type].empty()) {
        resStatus->candidatesValue[type] = INVALID_VALUE;
        resStatus->candidatesEndTime[type] = MAX_INT_VALUE;
    } else {
        InnerArbitrateCandidatesValue(type, resStatus);
    }

    if (resStatus->candidatesValue[type] != prevValue || resStatus->candidatesEndTime[type] != prevEndTime) {
        ArbitrateCandidate(resId);
    }
}

void SocPerfThreadWrap::InnerArbitrateCandidatesValue(int32_t type, std::shared_ptr<ResStatus> resStatus)
{
    // perf first action type:  ACTION_TYPE_PERF\ACTION_TYPE_PERFLVL
    // power first action type: ACTION_TYPE_POWER\ACTION_TYPE_THERMAL
    bool isPerfFirst = (type == ACTION_TYPE_PERF || type == ACTION_TYPE_PERFLVL);

    int64_t res = isPerfFirst ? MIN_INT_VALUE : MAX_INT_VALUE;
    int64_t endTime = MIN_INT_VALUE;
    for (auto iter = resStatus->resActionList[type].begin();
        iter != resStatus->resActionList[type].end(); ++iter) {
        if (((*iter)->value > res && isPerfFirst)
            || ((*iter)->value < res && !isPerfFirst)) {
            res = (*iter)->value;
            endTime = (*iter)->endTime;
        } else if ((*iter)->value == res) {
            endTime = Max(endTime, (*iter)->endTime);
        }
    }
    resStatus->candidatesValue[type] = res;
    resStatus->candidatesEndTime[type] = endTime;
}

void SocPerfThreadWrap::ArbitrateCandidate(int32_t resId)
{
    std::shared_ptr<ResStatus> resStatus = resStatusInfo_[resId];
    // if perf, power and thermal don't have valid value, send default value
    if (ExistNoCandidate(resId, resStatus)) {
        return;
    }
    // Arbitrate in perf, power and thermal
    ProcessLimitCase(resId);
    // perf request thermal level is highest priority in this freq adjuster
    if (ArbitratePairResInPerfLvl(resId)) {
        return;
    }
    // adjust resource value if it has 'max' config
    ArbitratePairRes(resId, false);
}

void SocPerfThreadWrap::ProcessLimitCase(int32_t resId)
{
    std::shared_ptr<ResStatus> resStatus = resStatusInfo_[resId];
    int64_t candidatePerfValue = resStatus->candidatesValue[ACTION_TYPE_PERF];
    int64_t candidatePowerValue = resStatus->candidatesValue[ACTION_TYPE_POWER];
    int64_t candidateThermalValue = resStatus->candidatesValue[ACTION_TYPE_THERMAL];
    if (!powerLimitBoost_ && !thermalLimitBoost_) {
        if (candidatePerfValue != INVALID_VALUE) {
            resStatus->candidate = Max(candidatePerfValue, candidatePowerValue, candidateThermalValue);
        } else {
            resStatus->candidate = (candidatePowerValue == INVALID_VALUE) ? candidateThermalValue :
                ((candidateThermalValue == INVALID_VALUE) ? candidatePowerValue :
                Min(candidatePowerValue, candidateThermalValue));
        }
    } else if (!powerLimitBoost_ && thermalLimitBoost_) {
        resStatus->candidate = (candidateThermalValue != INVALID_VALUE) ? candidateThermalValue :
            Max(candidatePerfValue, candidatePowerValue);
    } else if (powerLimitBoost_ && !thermalLimitBoost_) {
        resStatus->candidate = (candidatePowerValue != INVALID_VALUE) ? candidatePowerValue :
            Max(candidatePerfValue, candidateThermalValue);
    } else {
        if (candidatePowerValue == INVALID_VALUE && candidateThermalValue == INVALID_VALUE) {
            resStatus->candidate = candidatePerfValue;
        } else {
            resStatus->candidate = (candidatePowerValue == INVALID_VALUE) ? candidateThermalValue :
                ((candidateThermalValue == INVALID_VALUE) ? candidatePowerValue :
                Min(candidatePowerValue, candidateThermalValue));
        }
    }
    resStatus->currentEndTime = Min(resStatus->candidatesEndTime[ACTION_TYPE_PERF],
        resStatus->candidatesEndTime[ACTION_TYPE_POWER], resStatus->candidatesEndTime[ACTION_TYPE_THERMAL]);
}

bool SocPerfThreadWrap::ArbitratePairResInPerfLvl(int32_t resId)
{
    std::shared_ptr<ResStatus> resStatus = resStatusInfo_[resId];
    int32_t pairResId = INVALID_VALUE;
    if (!socPerfConfig_.IsGovResId(resId)) {
        pairResId = std::static_pointer_cast<ResNode>(socPerfConfig_.resourceNodeInfo_[resId])->pair;
    }
    // if resource self and resource's pair both not have perflvl value
    if (resStatus->candidatesValue[ACTION_TYPE_PERFLVL] == INVALID_VALUE && (pairResId != INVALID_VALUE &&
        resStatusInfo_[pairResId]->candidatesValue[ACTION_TYPE_PERFLVL] == INVALID_VALUE)) {
        return false;
    }
    // if this resource has PerfRequestLvl value, the final arbitrate value change to PerfRequestLvl value
    if (resStatus->candidatesValue[ACTION_TYPE_PERFLVL] != INVALID_VALUE) {
        if (thermalLvl_ == 0 && resStatus->candidate != INVALID_VALUE) {
            resStatus->candidate = Min(resStatus->candidate, resStatus->candidatesValue[ACTION_TYPE_PERFLVL]);
        } else {
            resStatus->candidate = resStatus->candidatesValue[ACTION_TYPE_PERFLVL];
        }
    }
    // only limit max when PerfRequestLvl has max value
    bool limit = false;
    if (thermalLvl_ != 0 &&
        !socPerfConfig_.IsGovResId(resId) &&
        (socPerfConfig_.resourceNodeInfo_[resId]->isMaxValue ||
        (pairResId != INVALID_VALUE && socPerfConfig_.resourceNodeInfo_[pairResId]->isMaxValue))) {
        limit = true;
    }
    ArbitratePairRes(resId, limit);
    return true;
}

void SocPerfThreadWrap::ArbitratePairRes(int32_t resId, bool perfRequestLimit)
{
    bool limit = powerLimitBoost_ || thermalLimitBoost_ || perfRequestLimit;
    if (socPerfConfig_.IsGovResId(resId)) {
        UpdateCurrentValue(resId, resStatusInfo_[resId]->candidate);
        return;
    }
    int32_t pairResId = std::static_pointer_cast<ResNode>(socPerfConfig_.resourceNodeInfo_[resId])->pair;
    if (pairResId == INVALID_VALUE) {
        UpdateCurrentValue(resId, resStatusInfo_[resId]->candidate);
        return;
    }

    if (resStatusInfo_[pairResId]->candidate == NODE_DEFAULT_VALUE ||
        resStatusInfo_[resId]->candidate == NODE_DEFAULT_VALUE) {
        UpdatePairResValue(resId, resStatusInfo_[resId]->candidate, pairResId, resStatusInfo_[pairResId]->candidate);
        return;
    }

    if (socPerfConfig_.resourceNodeInfo_[resId]->isMaxValue) {
        if (resStatusInfo_[resId]->candidate < resStatusInfo_[pairResId]->candidate) {
            if (limit) {
                UpdatePairResValue(pairResId,
                    resStatusInfo_[resId]->candidate, resId, resStatusInfo_[resId]->candidate);
            } else {
                UpdatePairResValue(pairResId,
                    resStatusInfo_[pairResId]->candidate, resId, resStatusInfo_[pairResId]->candidate);
            }
        } else {
            UpdatePairResValue(pairResId,
                resStatusInfo_[pairResId]->candidate, resId, resStatusInfo_[resId]->candidate);
        }
    } else {
        if (resStatusInfo_[resId]->candidate > resStatusInfo_[pairResId]->candidate) {
            if (limit) {
                UpdatePairResValue(resId,
                    resStatusInfo_[pairResId]->candidate, pairResId, resStatusInfo_[pairResId]->candidate);
            } else {
                UpdatePairResValue(resId,
                    resStatusInfo_[resId]->candidate, pairResId, resStatusInfo_[resId]->candidate);
            }
        } else {
            UpdatePairResValue(resId,
                resStatusInfo_[resId]->candidate, pairResId, resStatusInfo_[pairResId]->candidate);
        }
    }
}

void SocPerfThreadWrap::UpdatePairResValue(int32_t minResId, int64_t minResValue, int32_t maxResId,
    int64_t maxResValue)
{
    UpdateCurrentValue(minResId, minResValue);
    UpdateCurrentValue(maxResId, maxResValue);
}

void SocPerfThreadWrap::UpdateCurrentValue(int32_t resId, int64_t currValue)
{
    resStatusInfo_[resId]->currentValue = currValue;
}

bool SocPerfThreadWrap::ExistNoCandidate(int32_t resId, std::shared_ptr<ResStatus> resStatus)
{
    int64_t perfCandidate = resStatus->candidatesValue[ACTION_TYPE_PERF];
    int64_t powerCandidate = resStatus->candidatesValue[ACTION_TYPE_POWER];
    int64_t thermalCandidate = resStatus->candidatesValue[ACTION_TYPE_THERMAL];
    int64_t perfLvlCandidate = resStatus->candidatesValue[ACTION_TYPE_PERFLVL];
    if (perfCandidate == INVALID_VALUE && powerCandidate == INVALID_VALUE && thermalCandidate == INVALID_VALUE
        && perfLvlCandidate == INVALID_VALUE) {
        resStatus->candidate = NODE_DEFAULT_VALUE;
        resStatus->currentEndTime = MAX_INT_VALUE;
        ArbitratePairRes(resId, false);
        return true;
    }
    return false;
}
} // namespace SOCPERF
} // namespace OHOS
