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

#ifndef SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_CONFIG_H
#define SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_CONFIG_H

#include "libxml/tree.h"
#include "socperf_common.h"
#include <string>
#include <vector>

namespace OHOS {
namespace SOCPERF {
using ReportDataFunc = int (*)(const std::vector<int32_t>& resId, const std::vector<int64_t>& value,
    const std::vector<int64_t>& endTime, const std::string& msgStr);
class SocPerfConfig {
public:
    bool Init();
    bool IsGovResId(int32_t resId) const;
    bool IsValidResId(int32_t resId) const;
    int32_t GetResIdNumsPerType(int32_t resId) const;
    static SocPerfConfig& GetInstance();

public:
    bool wrapSwitch_[MAX_QUEUE_NUM] = { false };
    ReportDataFunc reportFunc_ = nullptr;
    std::unordered_map<int32_t, std::shared_ptr<ResourceNode>> resourceNodeInfo_;
    std::unordered_map<int32_t, std::shared_ptr<Actions>> perfActionsInfo_;

private:
    SocPerfConfig();
    ~SocPerfConfig();
    std::string GetRealConfigPath(const std::string& configFile);
    bool LoadConfigXmlFile(const std::string& configFile);
    void InitPerfFunc(const char* perfSoPath, const char* perfSoFunc);
    bool ParseBoostXmlFile(const xmlNode* rootNode, const std::string& realConfigFile, xmlDoc* file);
    bool ParseResourceXmlFile(const xmlNode* rootNode, const std::string& realConfigFile, xmlDoc* file);
    bool LoadResource(xmlNode* rootNode, const std::string& configFile);
    bool TraversalFreqResource(xmlNode* grandson, const std::string& configFile);
    bool LoadFreqResourceContent(int32_t persistMode, xmlNode* greatGrandson, const std::string& configFile,
        std::shared_ptr<ResNode> resNode);
    int32_t GetXmlIntProp(const xmlNode* xmlNode, const char* propName) const;
    bool LoadGovResource(xmlNode* rootNode, const std::string& configFile);
    bool TraversalGovResource(int32_t persistMode, xmlNode* greatGrandson, const std::string& configFile,
        std::shared_ptr<GovResNode> govResNode);
    void LoadInfo(xmlNode* child, const std::string& configFile);
    bool LoadCmd(const xmlNode* rootNode, const std::string& configFile);
    bool TraversalBoostResource(xmlNode* grandson, const std::string& configFile, std::shared_ptr<Actions> actions);
    bool ParseDuration(xmlNode* greatGrandson, const std::string& configFile, std::shared_ptr<Action> action) const;
    bool ParseResValue(xmlNode* greatGrandson, const std::string& configFile, std::shared_ptr<Action> action);
    bool CheckResourceTag(const char* id, const char* name, const char* pair, const char* mode,
        const char* persistMode, const std::string& configFile) const;
    bool CheckResourcePersistMode(const char* persistMode, const std::string& configFile) const;
    bool CheckResourceTag(int32_t persistMode, const char* def, const char* path, const std::string& configFile) const;
    bool LoadResourceAvailable(std::shared_ptr<ResNode> resNode, const char* node);
    bool CheckPairResIdValid() const;
    bool CheckDefValid() const;
    bool CheckGovResourceTag(const char* id, const char* name, const char* persistMode,
        const std::string& configFile) const;
    void ParseModeCmd(const char* mode, const std::string& configFile, std::shared_ptr<Actions> actions);
    bool LoadGovResourceAvailable(std::shared_ptr<GovResNode> govResNode, const char* level, const char* node);
    bool CheckCmdTag(const char* id, const char* name, const std::string& configFile) const;
    bool CheckActionResIdAndValueValid(const std::string& configFile);
    bool TraversalActions(std::shared_ptr<Action> action, int32_t actionId);
    void PrintCachedInfo() const;
};
} // namespace SOCPERF
} // namespace OHOS
#endif // SOC_PERF_SERVICES_CORE_INCLUDE_SOCPERF_H
