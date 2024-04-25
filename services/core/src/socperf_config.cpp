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
#include <chrono>
#include <dlfcn.h>
#include "socperf_config.h"
#include "config_policy_utils.h"
#include "hisysevent.h"
#include "hitrace_meter.h"
#include "parameters.h"

namespace OHOS {
namespace SOCPERF {
namespace {
    std::unordered_map<std::string, int32_t> g_resStrToIdInfo;
    void* g_handle;
    const std::string SPLIT_OR = "|";
    const std::string SPLIT_EQUAL = "=";
    const std::string SPLIT_SPACE = " ";
}

SocPerfConfig& SocPerfConfig::GetInstance()
{
    static SocPerfConfig socPerfConfig_;
    return socPerfConfig_;
}

SocPerfConfig::SocPerfConfig() {}

SocPerfConfig::~SocPerfConfig()
{
    if (g_handle != nullptr) {
        dlclose(g_handle);
        g_handle = nullptr;
    }
}

bool SocPerfConfig::Init()
{
    std::string resourceConfigXml = system::GetParameter("ohos.boot.kernel", "").size() > 0 ?
        SOCPERF_BOOST_CONFIG_XML_EXT : SOCPERF_BOOST_CONFIG_XML;
    if (!LoadConfigXmlFile(SOCPERF_RESOURCE_CONFIG_XML)) {
        SOC_PERF_LOGE("Failed to load %{public}s", SOCPERF_RESOURCE_CONFIG_XML.c_str());
        return false;
    }

    if (!LoadConfigXmlFile(resourceConfigXml)) {
        SOC_PERF_LOGE("Failed to load %{public}s", resourceConfigXml.c_str());
        return false;
    }

    PrintCachedInfo();
    g_resStrToIdInfo.clear();
    g_resStrToIdInfo = std::unordered_map<std::string, int32_t>();
    SOC_PERF_LOGD("SocPerf Init SUCCESS!");
    return true;
}

bool SocPerfConfig::IsGovResId(int32_t resId) const
{
    auto item = resourceNodeInfo_.find(resId);
    if (item != resourceNodeInfo_.end() && item->second->isGov) {
        return true;
    }
    return false;
}

bool SocPerfConfig::IsValidResId(int32_t resId) const
{
    if (resourceNodeInfo_.find(resId) == resourceNodeInfo_.end()) {
        return false;
    }
    return true;
}

std::string SocPerfConfig::GetRealConfigPath(const std::string& configFile)
{
    char buf[PATH_MAX + 1];
    char* configFilePath = GetOneCfgFile(configFile.c_str(), buf, PATH_MAX + 1);
    char tmpPath[PATH_MAX + 1] = {0};
    if (!configFilePath || strlen(configFilePath) == 0 || strlen(configFilePath) > PATH_MAX ||
        !realpath(configFilePath, tmpPath)) {
        SOC_PERF_LOGE("load %{public}s file fail", configFile.c_str());
        return "";
    }
    return std::string(tmpPath);
}

bool SocPerfConfig::LoadConfigXmlFile(const std::string& configFile)
{
    std::string realConfigFile = GetRealConfigPath(configFile);
    if (realConfigFile.size() == 0) {
        return false;
    }
    xmlKeepBlanksDefault(0);
    xmlDoc* file = xmlReadFile(realConfigFile.c_str(), nullptr, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    if (!file) {
        SOC_PERF_LOGE("Failed to open xml file");
        return false;
    }
    xmlNode* rootNode = xmlDocGetRootElement(file);
    if (!rootNode) {
        SOC_PERF_LOGE("Failed to get xml file's RootNode");
        xmlFreeDoc(file);
        return false;
    }
    if (!xmlStrcmp(rootNode->name, reinterpret_cast<const xmlChar*>("Configs"))) {
        if (realConfigFile.find(SOCPERF_RESOURCE_CONFIG_XML) != std::string::npos) {
            bool ret = ParseResourceXmlFile(rootNode, realConfigFile, file);
            if (!ret) {
                xmlFreeDoc(file);
                return false;
            }
        } else {
            bool ret = ParseBoostXmlFile(rootNode, realConfigFile, file);
            if (!ret) {
                xmlFreeDoc(file);
                return false;
            }
        }
    } else {
        SOC_PERF_LOGE("Wrong format for xml file");
        xmlFreeDoc(file);
        return false;
    }
    xmlFreeDoc(file);
    SOC_PERF_LOGD("Success to Load %{public}s", configFile.c_str());
    return true;
}

void SocPerfConfig::InitPerfFunc(const char* perfSoPath, const char* perfSoFunc)
{
    if (perfSoPath == nullptr || perfSoFunc == nullptr) {
        return;
    }

    g_handle = dlopen(perfSoPath, RTLD_NOW);
    if (g_handle == nullptr) {
        SOC_PERF_LOGE("perf so doesn't exist");
        return;
    }

    reportFunc_ = reinterpret_cast<ReportDataFunc>(dlsym(g_handle, perfSoFunc));
    if (reportFunc_ == nullptr) {
        SOC_PERF_LOGE("perf func doesn't exist");
        dlclose(g_handle);
    }
}

bool SocPerfConfig::ParseBoostXmlFile(const xmlNode* rootNode, const std::string& realConfigFile, xmlDoc* file)
{
    if (!LoadCmd(rootNode, realConfigFile)) {
        return false;
    }
    return true;
}

bool SocPerfConfig::ParseResourceXmlFile(const xmlNode* rootNode, const std::string& realConfigFile, xmlDoc* file)
{
    xmlNode* child = rootNode->children;
    for (; child; child = child->next) {
        if (!xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("Resource"))) {
            if (!LoadResource(child, realConfigFile)) {
                return false;
            }
        } else if (!xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("GovResource"))) {
            if (!LoadGovResource(child, realConfigFile)) {
                return false;
            }
        } else if (!xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("Info"))) {
            LoadInfo(child, realConfigFile);
        }
    }
    return true;
}

bool SocPerfConfig::LoadResource(xmlNode* child, const std::string& configFile)
{
    xmlNode* grandson = child->children;
    for (; grandson; grandson = grandson->next) {
        if (!xmlStrcmp(grandson->name, reinterpret_cast<const xmlChar*>("res"))) {
            if (!TraversalFreqResource(grandson, configFile)) {
                return false;
            }
        }
    }

    if (!CheckPairResIdValid() || !CheckDefValid()) {
        return false;
    }

    return true;
}

bool SocPerfConfig::TraversalFreqResource(xmlNode* grandson, const std::string& configFile)
{
    char* id = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("id")));
    char* name = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("name")));
    char* pair = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("pair")));
    char* mode = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("mode")));
    char* persistMode = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("switch")));
    if (!CheckResourceTag(id, name, pair, mode, persistMode, configFile)) {
        xmlFree(id);
        xmlFree(name);
        xmlFree(pair);
        xmlFree(mode);
        xmlFree(persistMode);
        return false;
    }
    xmlNode* greatGrandson = grandson->children;
    std::shared_ptr<ResNode> resNode = std::make_shared<ResNode>(atoi(id), name, mode ? atoi(mode) : 0,
        pair ? atoi(pair) : INVALID_VALUE, persistMode ? atoi(persistMode) : 0);
    xmlFree(id);
    xmlFree(name);
    xmlFree(pair);
    xmlFree(mode);
    xmlFree(persistMode);
    if (!LoadFreqResourceContent(persistMode ? atoi(persistMode) : 0, greatGrandson, configFile, resNode)) {
        return false;
    }
    return true;
}

bool SocPerfConfig::LoadFreqResourceContent(int32_t persistMode, xmlNode* greatGrandson, const std::string& configFile,
    std::shared_ptr<ResNode> resNode)
{
    char *def = nullptr;
    char *path  = nullptr;
    char *node  = nullptr;
    for (; greatGrandson; greatGrandson = greatGrandson->next) {
        if (!xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("default"))) {
            xmlFree(def);
            def = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
        } else if (persistMode != REPORT_TO_PERFSO &&
            !xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("path"))) {
            xmlFree(path);
            path = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
        } else if (!xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("node"))) {
            xmlFree(node);
            node = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
        }
    }
    if (!CheckResourceTag(persistMode, def, path, configFile)) {
        xmlFree(def);
        xmlFree(path);
        xmlFree(node);
        return false;
    }
    resNode->def = atoll(def);
    if (persistMode != REPORT_TO_PERFSO) {
        resNode->path = path;
    }
    xmlFree(def);
    xmlFree(path);
    if (node && !LoadResourceAvailable(resNode, node)) {
        SOC_PERF_LOGE("Invalid resource node for %{public}s", configFile.c_str());
        xmlFree(node);
        return false;
    }
    xmlFree(node);

    g_resStrToIdInfo.insert(std::pair<std::string, int32_t>(resNode->name, resNode->id));
    resourceNodeInfo_.insert(std::pair<int32_t, std::shared_ptr<ResNode>>(resNode->id, resNode));
    wrapSwitch_[resNode->id / GetResIdNumsPerType(resNode->id)] = true;
    return true;
}

bool SocPerfConfig::LoadGovResource(xmlNode* child, const std::string& configFile)
{
    xmlNode* grandson = child->children;
    for (; grandson; grandson = grandson->next) {
        if (xmlStrcmp(grandson->name, reinterpret_cast<const xmlChar*>("res"))) {
            continue;
        }
        char* id = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("id")));
        char* name = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("name")));
        char* persistMode = reinterpret_cast<char*>(xmlGetProp(grandson,
            reinterpret_cast<const xmlChar*>("switch")));
        if (!CheckGovResourceTag(id, name, persistMode, configFile)) {
            xmlFree(id);
            xmlFree(name);
            xmlFree(persistMode);
            return false;
        }
        xmlNode* greatGrandson = grandson->children;
        std::shared_ptr<GovResNode> govResNode = std::make_shared<GovResNode>(atoi(id),
            name, persistMode ? atoi(persistMode) : 0);
        xmlFree(id);
        xmlFree(name);
        xmlFree(persistMode);
        g_resStrToIdInfo.insert(std::pair<std::string, int32_t>(govResNode->name, govResNode->id));
        resourceNodeInfo_.insert(std::pair<int32_t, std::shared_ptr<GovResNode>>(govResNode->id, govResNode));
        wrapSwitch_[govResNode->id / GetResIdNumsPerType(govResNode->id)] = true;
        if (!TraversalGovResource(persistMode ? atoi(persistMode) : 0, greatGrandson, configFile, govResNode)) {
            return false;
        }
    }

    if (!CheckDefValid()) {
        return false;
    }

    return true;
}

int32_t SocPerfConfig::GetResIdNumsPerType(int32_t resId) const
{
    auto item = resourceNodeInfo_.find(resId);
    if (item != resourceNodeInfo_.end() && item->second->persistMode == REPORT_TO_PERFSO) {
        return RES_ID_NUMS_PER_TYPE_EXT;
    }
    return RES_ID_NUMS_PER_TYPE;
}

void SocPerfConfig::LoadInfo(xmlNode* child, const std::string& configFile)
{
    xmlNode* grandson = child->children;
    if (!grandson || xmlStrcmp(grandson->name, reinterpret_cast<const xmlChar*>("inf"))) {
        return;
    }
    char* perfSoPath = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("path")));
    char* perfSoFunc = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("func")));
    InitPerfFunc(perfSoPath, perfSoFunc);
    xmlFree(perfSoPath);
    xmlFree(perfSoFunc);
}

bool SocPerfConfig::TraversalGovResource(int32_t persistMode, xmlNode* greatGrandson, const std::string& configFile,
    std::shared_ptr<GovResNode> govResNode)
{
    for (; greatGrandson; greatGrandson = greatGrandson->next) {
        if (!xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("default"))) {
            char* def = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
            if (!def || !IsNumber(def)) {
                SOC_PERF_LOGE("Invalid governor resource default for %{public}s", configFile.c_str());
                xmlFree(def);
                return false;
            }
            govResNode->def = atoll(def);
            xmlFree(def);
        } else if (persistMode != REPORT_TO_PERFSO &&
            !xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("path"))) {
            char* path = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
            if (!path) {
                SOC_PERF_LOGE("Invalid governor resource path for %{public}s", configFile.c_str());
                return false;
            }
            govResNode->paths.push_back(path);
            xmlFree(path);
        } else if (persistMode != REPORT_TO_PERFSO &&
            !xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("node"))) {
            char* level = reinterpret_cast<char*>(
                xmlGetProp(greatGrandson, reinterpret_cast<const xmlChar*>("level")));
            char* node = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
            if (!level || !IsNumber(level) || !node
                || !LoadGovResourceAvailable(govResNode, level, node)) {
                SOC_PERF_LOGE("Invalid governor resource node for %{public}s", configFile.c_str());
                xmlFree(level);
                xmlFree(node);
                return false;
            }
            xmlFree(level);
            xmlFree(node);
        }
    }
    return true;
}

bool SocPerfConfig::LoadCmd(const xmlNode* rootNode, const std::string& configFile)
{
    xmlNode* child = rootNode->children;
    for (; child; child = child->next) { // Iterate all cmdID
        if (xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("cmd"))) {
            continue;
        }
        char* id = reinterpret_cast<char*>(xmlGetProp(child, reinterpret_cast<const xmlChar*>("id")));
        char* name = reinterpret_cast<char*>(xmlGetProp(child, reinterpret_cast<const xmlChar*>("name")));
        if (!CheckCmdTag(id, name, configFile)) {
            xmlFree(id);
            xmlFree(name);
            return false;
        }
        xmlNode* grandson = child->children;
        std::shared_ptr<Actions> actions = std::make_shared<Actions>(atoi(id), name);
        xmlFree(id);
        xmlFree(name);

        char* mode = reinterpret_cast<char*>(xmlGetProp(child, reinterpret_cast<const xmlChar*>("mode")));
        if (mode) {
            ParseModeCmd(mode, configFile, actions);
            xmlFree(mode);
        }

        if (!TraversalBoostResource(grandson, configFile, actions)) {
            return false;
        }
        perfActionsInfo_.insert(std::pair<int32_t, std::shared_ptr<Actions>>(actions->id, actions));
    }

    if (!CheckActionResIdAndValueValid(configFile)) {
        return false;
    }

    return true;
}

void SocPerfConfig::ParseModeCmd(const char* mode, const std::string& configFile, std::shared_ptr<Actions> actions)
{
    if (!mode) {
        return;
    }

    std::string modeStr = mode;
    std::vector<std::string> modeListResult = Split(modeStr, SPLIT_OR);
    for (auto pairStr : modeListResult) {
        std::vector<std::string> itemPair = Split(pairStr, SPLIT_EQUAL);
        if (itemPair.size() != RES_MODE_AND_ID_PAIR) {
            SOC_PERF_LOGW("Invaild device mode pair for %{public}s", configFile.c_str());
            continue;
        }

        std::string modeDeviceStr = itemPair[0];
        std::string modeCmdIdStr = itemPair[RES_MODE_AND_ID_PAIR -1];
        if (modeDeviceStr.empty() || !IsNumber(modeCmdIdStr)) {
            SOC_PERF_LOGW("Invaild device mode name for %{public}s", configFile.c_str());
            continue;
        }

        int32_t cmdId = atoi(modeCmdIdStr.c_str());
        auto iter = actions->modeMap.find(modeDeviceStr);
        if (iter != actions->modeMap.end()) {
            iter->second = cmdId;
        } else {
            actions->modeMap.insert(std::pair<std::string, int32_t>(modeDeviceStr, cmdId));
        }
    }
}

bool SocPerfConfig::ParseDuration(xmlNode *greatGrandson,
    const std::string& configFile, std::shared_ptr<Action> action) const
{
    if (xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("duration"))) {
        return true;
    }
    char* duration = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
    if (!duration || !IsNumber(duration)) {
        SOC_PERF_LOGE("Invalid cmd duration for %{public}s", configFile.c_str());
        xmlFree(duration);
        return false;
    }
    action->duration = atoi(duration);
    xmlFree(duration);
    return true;
}

bool SocPerfConfig::ParseResValue(xmlNode* greatGrandson, const std::string& configFile, std::shared_ptr<Action> action)
{
    if (!xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("duration"))) {
        return true;
    }
    char* resStr = reinterpret_cast<char*>(const_cast<xmlChar*>(greatGrandson->name));
    char* resValue = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
    if (!resStr || g_resStrToIdInfo.find(resStr) == g_resStrToIdInfo.end()
        || !resValue || !IsNumber(resValue)) {
        SOC_PERF_LOGE("Invalid cmd resource(%{public}s) for %{public}s", resStr, configFile.c_str());
        xmlFree(resValue);
        return false;
    }
    action->variable.push_back(g_resStrToIdInfo[resStr]);
    action->variable.push_back(atoll(resValue));
    xmlFree(resValue);
    return true;
}

int32_t SocPerfConfig::GetXmlIntProp(const xmlNode* xmlNode, const char* propName) const
{
    int ret = -1;
    char* propValue = reinterpret_cast<char*>(xmlGetProp(xmlNode, reinterpret_cast<const xmlChar*>(propName)));
    if (propValue != nullptr && IsNumber(propValue)) {
        ret = atoi(propValue);
    }
    if (propValue != nullptr) {
        xmlFree(propValue);
    }
    return ret;
}

bool SocPerfConfig::TraversalBoostResource(xmlNode* grandson,
    const std::string& configFile, std::shared_ptr<Actions> actions)
{
    for (; grandson; grandson = grandson->next) { // Iterate all Action
        std::shared_ptr<Action> action = std::make_shared<Action>();
        action->thermalLvl_ = GetXmlIntProp(grandson, "thermalLvl");
        action->thermalCmdId_ = GetXmlIntProp(grandson, "thermalCmdId");
        xmlNode* greatGrandson = grandson->children;
        for (; greatGrandson; greatGrandson = greatGrandson->next) { // Iterate duration and all res
            bool ret = ParseDuration(greatGrandson, configFile, action);
            if (!ret) {
                return false;
            }
            if (action->duration == 0) {
                actions->isLongTimePerf = true;
            }
            ret = ParseResValue(greatGrandson, configFile, action);
            if (!ret) {
                return false;
            }
        }
        actions->actionList.push_back(action);
    }
    return true;
}

bool SocPerfConfig::CheckResourceTag(const char* id, const char* name, const char* pair, const char* mode,
    const char* persistMode, const std::string& configFile) const
{
    if (!id || !IsNumber(id) || !IsValidRangeResId(atoi(id))) {
        SOC_PERF_LOGE("Invalid resource id for %{public}s", configFile.c_str());
        return false;
    }
    if (!name) {
        SOC_PERF_LOGE("Invalid resource name for %{public}s", configFile.c_str());
        return false;
    }
    if (pair && (!IsNumber(pair) || !IsValidRangeResId(atoi(pair)))) {
        SOC_PERF_LOGE("Invalid resource pair for %{public}s", configFile.c_str());
        return false;
    }
    if (mode && !IsNumber(mode)) {
        SOC_PERF_LOGE("Invalid resource mode for %{public}s", configFile.c_str());
        return false;
    }
    return CheckResourcePersistMode(persistMode, configFile);
}

bool SocPerfConfig::CheckResourcePersistMode(const char* persistMode, const std::string& configFile) const
{
    if (persistMode && (!IsNumber(persistMode) || !IsValidPersistMode(atoi(persistMode)))) {
        SOC_PERF_LOGE("Invalid resource persistMode for %{public}s", configFile.c_str());
        return false;
    }
    return true;
}

bool SocPerfConfig::CheckResourceTag(int32_t persistMode, const char* def,
    const char* path, const std::string& configFile) const
{
    if (!def || !IsNumber(def)) {
        SOC_PERF_LOGE("Invalid resource default for %{public}s", configFile.c_str());
        return false;
    }
    if (persistMode != REPORT_TO_PERFSO && !path) {
        SOC_PERF_LOGE("Invalid resource path for %{public}s", configFile.c_str());
        return false;
    }
    return true;
}

bool SocPerfConfig::LoadResourceAvailable(std::shared_ptr<ResNode> resNode, const char* node)
{
    std::string nodeStr = node;
    std::vector<std::string> result = Split(nodeStr, SPLIT_SPACE);
    for (auto str : result) {
        if (IsNumber(str)) {
            resNode->available.insert(stoll(str));
        } else {
            return false;
        }
    }
    return true;
}

bool SocPerfConfig::CheckPairResIdValid() const
{
    for (auto iter = resourceNodeInfo_.begin(); iter != resourceNodeInfo_.end(); ++iter) {
        if (iter->second->isGov) {
            continue;
        }
        int32_t resId = iter->first;
        std::shared_ptr<ResNode> resNode = std::static_pointer_cast<ResNode>(iter->second);
        int32_t pairResId = resNode->pair;
        if (pairResId != INVALID_VALUE && resourceNodeInfo_.find(pairResId) == resourceNodeInfo_.end()) {
            SOC_PERF_LOGE("resId[%{public}d]'s pairResId[%{public}d] is not valid", resId, pairResId);
            return false;
        }
    }
    return true;
}

bool SocPerfConfig::CheckDefValid() const
{
    for (auto iter = resourceNodeInfo_.begin(); iter != resourceNodeInfo_.end(); ++iter) {
        int32_t resId = iter->first;
        std::shared_ptr<ResourceNode> resourceNode = iter->second;
        int64_t def = resourceNode->def;
        if (!resourceNode->available.empty() && resourceNode->available.find(def) == resourceNode->available.end()) {
            SOC_PERF_LOGE("resId[%{public}d]'s def[%{public}lld] is not valid", resId, (long long)def);
            return false;
        }
    }
    return true;
}

bool SocPerfConfig::CheckGovResourceTag(const char* id, const char* name,
    const char* persistMode, const std::string& configFile) const
{
    if (!id || !IsNumber(id) || !IsValidRangeResId(atoi(id))) {
        SOC_PERF_LOGE("Invalid governor resource id for %{public}s", configFile.c_str());
        return false;
    }
    if (!name) {
        SOC_PERF_LOGE("Invalid governor resource name for %{public}s", configFile.c_str());
        return false;
    }
    if (persistMode && (!IsNumber(persistMode) || !IsValidPersistMode(atoi(persistMode)))) {
        SOC_PERF_LOGE("Invalid governor resource persistMode for %{public}s", configFile.c_str());
        return false;
    }
    return true;
}

bool SocPerfConfig::LoadGovResourceAvailable(std::shared_ptr<GovResNode> govResNode,
    const char* level, const char* node)
{
    govResNode->available.insert(atoll(level));
    std::string nodeStr = node;
    std::vector<std::string> result = Split(nodeStr, SPLIT_OR);
    if (result.size() != govResNode->paths.size()) {
        SOC_PERF_LOGE("Invalid governor resource node matches paths");
        return false;
    }
    govResNode->levelToStr.insert(std::pair<int32_t, std::vector<std::string>>(atoll(level), result));
    return true;
}

bool SocPerfConfig::CheckCmdTag(const char* id, const char* name, const std::string& configFile) const
{
    if (!id || !IsNumber(id)) {
        SOC_PERF_LOGE("Invalid cmd id for %{public}s", configFile.c_str());
        return false;
    }
    if (!name) {
        SOC_PERF_LOGE("Invalid cmd name for %{public}s", configFile.c_str());
        return false;
    }
    return true;
}

bool SocPerfConfig::TraversalActions(std::shared_ptr<Action> action, int32_t actionId)
{
    for (int32_t i = 0; i < (int32_t)action->variable.size() - 1; i += RES_ID_AND_VALUE_PAIR) {
        int32_t resId = action->variable[i];
        int64_t resValue = action->variable[i + 1];
        if (resourceNodeInfo_.find(resId) != resourceNodeInfo_.end()) {
            if (resourceNodeInfo_[resId]->persistMode != REPORT_TO_PERFSO &&
                !resourceNodeInfo_[resId]->available.empty() &&
                resourceNodeInfo_[resId]->available.find(resValue) == resourceNodeInfo_[resId]->available.end()) {
                SOC_PERF_LOGE("action[%{public}d]'s resValue[%{public}lld] is not valid",
                    actionId, (long long)resValue);
                return false;
            }
        } else {
            SOC_PERF_LOGE("action[%{public}d]'s resId[%{public}d] is not valid", actionId, resId);
            return false;
        }
    }
    return true;
}

bool SocPerfConfig::CheckActionResIdAndValueValid(const std::string& configFile)
{
    std::unordered_map<int32_t, std::shared_ptr<Actions>> actionsInfo = perfActionsInfo_;
    for (auto actionsIter = actionsInfo.begin(); actionsIter != actionsInfo.end(); ++actionsIter) {
        int32_t actionId = actionsIter->first;
        std::shared_ptr<Actions> actions = actionsIter->second;
        for (auto actionIter = actions->actionList.begin(); actionIter != actions->actionList.end(); ++actionIter) {
            bool ret = TraversalActions(*actionIter, actionId);
            if (!ret) {
                return false;
            }
        }
    }
    return true;
}

void SocPerfConfig::PrintCachedInfo() const
{
    SOC_PERF_LOGD("------------------------------------");
    SOC_PERF_LOGD("resourceNodeInfo_(%{public}d)", (int32_t)resourceNodeInfo_.size());
    for (auto iter = resourceNodeInfo_.begin(); iter != resourceNodeInfo_.end(); ++iter) {
        iter->second->PrintString();
    }
    SOC_PERF_LOGD("------------------------------------");
    SOC_PERF_LOGD("perfActionsInfo_(%{public}d)", (int32_t)perfActionsInfo_.size());
    for (auto iter = perfActionsInfo_.begin(); iter != perfActionsInfo_.end(); ++iter) {
        std::shared_ptr<Actions> actions = iter->second;
        actions->PrintString();
    }
    SOC_PERF_LOGD("------------------------------------");
}
} // namespace SOCPERF
} // namespace OHOS
