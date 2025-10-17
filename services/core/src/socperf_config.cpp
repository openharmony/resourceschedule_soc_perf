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
#include "socperf_config.h"

#include <algorithm>
#include <chrono>
#include <dlfcn.h>
 
#include "config_policy_utils.h"
#include "parameters.h"
#ifdef RES_SCHED_SA_INIT
#include "res_sa_init.h"
#endif

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
#ifdef RES_SCHED_SA_INIT
    std::lock_guard<std::mutex> xmlLock(ResourceSchedule::ResSchedSaInit::GetInstance().saInitXmlMutex_);
#endif
    std::string resourceConfigXml = system::GetParameter("ohos.boot.kernel", "").size() > 0 ?
        SOCPERF_BOOST_CONFIG_XML_EXT : SOCPERF_BOOST_CONFIG_XML;
    if (!LoadAllConfigXmlFile(SOCPERF_RESOURCE_CONFIG_XML)) {
        SOC_PERF_LOGE("Failed to load %{private}s", SOCPERF_RESOURCE_CONFIG_XML.c_str());
        return false;
    }

    if (!LoadAllConfigXmlFile(resourceConfigXml)) {
        SOC_PERF_LOGE("Failed to load %{private}s", resourceConfigXml.c_str());
        return false;
    }

    if (!LoadAllConfigXmlFile(CAMERA_AWARE_CONFIG_XML)) {
        SOC_PERF_LOGE("Failed to load %{private}s", CAMERA_AWARE_CONFIG_XML.c_str());
    }

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
        SOC_PERF_LOGE("load %{private}s file fail", configFile.c_str());
        return "";
    }
    return std::string(tmpPath);
}

std::vector<std::string> SocPerfConfig::GetAllRealConfigPath(const std::string& configFile)
{
    std::vector<std::string> configFilePaths;
    auto cfgFiles = GetCfgFiles(configFile.c_str());
    if (cfgFiles == nullptr) {
        return configFilePaths;
    }
    for (const auto& file : cfgFiles->paths) {
        if (file == nullptr) {
            continue;
        }
        configFilePaths.emplace_back(std::string(file));
    }
    FreeCfgFiles(cfgFiles);
    reverse(configFilePaths.begin(), configFilePaths.end());
    return configFilePaths;
}

bool SocPerfConfig::LoadAllConfigXmlFile(const std::string& configFile)
{
    std::vector<std::string> filePaths = GetAllRealConfigPath(configFile);
    for (auto realConfigFile : filePaths) {
        if (!(LoadConfigXmlFile(realConfigFile.c_str()))) {
            return false;
        }
    }
    SOC_PERF_LOGD("Success to Load %{private}s", configFile.c_str());
    return true;
}

bool SocPerfConfig::LoadConfigXmlFile(const std::string& realConfigFile)
{
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
    SOC_PERF_LOGD("Success to access %{private}s", realConfigFile.c_str());

    return true;
}

void SocPerfConfig::InitPerfFunc(const char* perfSoPath, const char* perfReportFunc, const char* perfScenarioFunc)
{
    if (perfSoPath == nullptr || (perfReportFunc == nullptr && perfScenarioFunc == nullptr)) {
        return;
    }

    if (reportFunc_ != nullptr && scenarioFunc_ != nullptr) {
        return;
    }

    g_handle = dlopen(perfSoPath, RTLD_NOW);
    if (g_handle == nullptr) {
        SOC_PERF_LOGE("perf so doesn't exist");
        return;
    }

    if (reportFunc_ == nullptr && perfReportFunc != nullptr) {
        reportFunc_ = reinterpret_cast<ReportDataFunc>(dlsym(g_handle, perfReportFunc));
    }

    if (scenarioFunc_ == nullptr && perfScenarioFunc != nullptr) {
        scenarioFunc_ = reinterpret_cast<PerfScenarioFunc>(dlsym(g_handle, perfScenarioFunc));
    }

    if (reportFunc_ == nullptr && scenarioFunc_ == nullptr) {
        SOC_PERF_LOGE("perf func doesn't exist");
        dlclose(g_handle);
        g_handle = nullptr;
    }
}

bool SocPerfConfig::ParseBoostXmlFile(const xmlNode* rootNode, const std::string& realConfigFile, xmlDoc* file)
{
    if (!LoadConfig(rootNode, realConfigFile)) {
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
        } else if (!xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("SceneResource"))) {
            LoadSceneResource(child, realConfigFile);
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

bool SocPerfConfig::CheckTrace(const char* trace)
{
    if (trace && IsNumber(trace) && atoi(trace) == PERF_OPEN_TRACE) {
        return true;
    }
    return false;
}

bool SocPerfConfig::TraversalFreqResource(xmlNode* grandson, const std::string& configFile)
{
    char* id = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("id")));
    char* name = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("name")));
    char* pair = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("pair")));
    char* mode = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("mode")));
    char* persistMode = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("switch")));
    char* trace = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("trace")));
    if (!CheckResourceTag(id, name, pair, mode, persistMode, configFile)) {
        xmlFree(id);
        xmlFree(name);
        xmlFree(pair);
        xmlFree(mode);
        xmlFree(persistMode);
        xmlFree(trace);
        return false;
    }
    auto it = resourceNodeInfo_.find(atoi(id));
    if (it != resourceNodeInfo_.end()) {
        xmlFree(id);
        xmlFree(name);
        xmlFree(pair);
        xmlFree(mode);
        xmlFree(persistMode);
        xmlFree(trace);
        return true;
    }
    xmlNode* greatGrandson = grandson->children;
    std::shared_ptr<ResNode> resNode = std::make_shared<ResNode>(atoi(id), name, mode ? atoi(mode) : 0,
        pair ? atoi(pair) : INVALID_VALUE, persistMode ? atoi(persistMode) : 0);
    resNode->trace = CheckTrace(trace);
    xmlFree(id);
    xmlFree(name);
    xmlFree(pair);
    xmlFree(mode);
    xmlFree(trace);
    if (!LoadFreqResourceContent(persistMode ? atoi(persistMode) : 0, greatGrandson, configFile, resNode)) {
        xmlFree(persistMode);
        return false;
    }
    xmlFree(persistMode);
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
        SOC_PERF_LOGE("Invalid resource node for %{private}s", configFile.c_str());
        xmlFree(node);
        return false;
    }
    xmlFree(node);

    g_resStrToIdInfo.insert(std::pair<std::string, int32_t>(resNode->name, resNode->id));
    resourceNodeInfo_.insert(std::pair<int32_t, std::shared_ptr<ResNode>>(resNode->id, resNode));

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
        char* persistMode = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("switch")));
        char* trace = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("trace")));
        if (!CheckGovResourceTag(id, name, persistMode, configFile)) {
            xmlFree(id);
            xmlFree(name);
            xmlFree(persistMode);
            xmlFree(trace);
            return false;
        }
        auto it = resourceNodeInfo_.find(atoi(id));
        if (it != resourceNodeInfo_.end()) {
            xmlFree(id);
            xmlFree(name);
            xmlFree(persistMode);
            xmlFree(trace);
            continue;
        }

        xmlNode* greatGrandson = grandson->children;
        std::shared_ptr<GovResNode> govResNode = std::make_shared<GovResNode>(atoi(id),
            name, persistMode ? atoi(persistMode) : 0);
        govResNode->trace = CheckTrace(trace);
        xmlFree(id);
        xmlFree(name);
        xmlFree(trace);
        g_resStrToIdInfo.insert(std::pair<std::string, int32_t>(govResNode->name, govResNode->id));
        resourceNodeInfo_.insert(std::pair<int32_t, std::shared_ptr<GovResNode>>(govResNode->id, govResNode));

        if (!TraversalGovResource(persistMode ? atoi(persistMode) : 0, greatGrandson, configFile, govResNode)) {
            xmlFree(persistMode);
            return false;
        }
        xmlFree(persistMode);
    }

    if (!CheckDefValid()) {
        return false;
    }

    return true;
}

void SocPerfConfig::LoadInfo(xmlNode* child, const std::string& configFile)
{
    xmlNode* grandson = child->children;
    if (!grandson || xmlStrcmp(grandson->name, reinterpret_cast<const xmlChar*>("inf"))) {
        return;
    }
    char* perfSoPath = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("path")));
    char* perfReportFunc =
        reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("func")));
    char* perfScenarioFunc =
        reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("scenarioFunc")));
    InitPerfFunc(perfSoPath, perfReportFunc, perfScenarioFunc);
    xmlFree(perfSoPath);
    xmlFree(perfReportFunc);
    xmlFree(perfScenarioFunc);
}

void SocPerfConfig::LoadInterAction(xmlNode* child, const std::string& configFile)
{
    xmlNode* grandson = child->children;
    for (; grandson; grandson = grandson->next) {
        if (xmlStrcmp(grandson->name, reinterpret_cast<const xmlChar*>("weak"))) {
            continue;
        }
        char* cmdId = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("id")));
        char* delayTime = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("delay")));
        char* actionType = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("type")));

        std::shared_ptr<InterAction> interAction = std::make_shared<InterAction>(
            atoi(cmdId), atoi(actionType), atoll(delayTime));
        interAction_.push_back(interAction);
        xmlFree(actionType);
        xmlFree(delayTime);
        xmlFree(cmdId);
    }
}

bool SocPerfConfig::LoadSceneResource(xmlNode* child, const std::string& configFile)
{
    xmlNode* grandson = child->children;
    for (; grandson; grandson = grandson->next) {
        if (xmlStrcmp(grandson->name, reinterpret_cast<const xmlChar*>("scene"))) {
            continue;
        }
        char* name = reinterpret_cast<char*>(xmlGetProp(grandson, reinterpret_cast<const xmlChar*>("name")));
        char* persistMode = reinterpret_cast<char*>(xmlGetProp(grandson,
            reinterpret_cast<const xmlChar*>("switch")));
        if (!CheckSceneResourceTag(name, persistMode, configFile)) {
            xmlFree(name);
            xmlFree(persistMode);
            return false;
        }
        auto it = sceneResourceInfo_.find(name);
        if (it != sceneResourceInfo_.end()) {
            xmlFree(name);
            xmlFree(persistMode);
            continue;
        }
        xmlNode* greatGrandson = grandson->children;
        std::shared_ptr<SceneResNode> sceneResNode =
            std::make_shared<SceneResNode>(name, persistMode ? atoi(persistMode) : 0);
        xmlFree(name);
        xmlFree(persistMode);

        sceneResourceInfo_.insert(std::pair<std::string, std::shared_ptr<SceneResNode>>(sceneResNode->name,
            sceneResNode));

        if (!TraversalSceneResource(greatGrandson, configFile, sceneResNode)) {
            return false;
        }
    }

    return true;
}

bool SocPerfConfig::TraversalSceneResource(xmlNode* greatGrandson, const std::string& configFile,
    std::shared_ptr<SceneResNode> sceneResNode)
{
    for (; greatGrandson; greatGrandson = greatGrandson->next) {
        if (!xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("item"))) {
            char* req = reinterpret_cast<char*>(
                xmlGetProp(greatGrandson, reinterpret_cast<const xmlChar*>("req")));
            char* item = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
            if (!item || (req && !IsNumber(req))) {
                SOC_PERF_LOGE("Invalid sceneernor resource item for %{private}s", configFile.c_str());
                xmlFree(item);
                xmlFree(req);
                return false;
            }

            std::shared_ptr<SceneItem> sceneItem =
                std::make_shared<SceneItem>(item, req ? atoi(req) : 1);
            sceneResNode->items.push_back(sceneItem);
            xmlFree(item);
            xmlFree(req);
        }
    }
    return true;
}

bool SocPerfConfig::CheckSceneResourceTag(const char* name, const char* persistMode,
    const std::string& configFile) const
{
    if (!name) {
        SOC_PERF_LOGE("Invalid sceneernor resource name for %{private}s", configFile.c_str());
        return false;
    }
    if (persistMode && (!IsNumber(persistMode) || !IsValidPersistMode(atoi(persistMode)))) {
        SOC_PERF_LOGE("Invalid sceneernor resource persistMode for %{private}s", configFile.c_str());
        return false;
    }
    return true;
}

bool SocPerfConfig::TraversalGovResource(int32_t persistMode, xmlNode* greatGrandson, const std::string& configFile,
    std::shared_ptr<GovResNode> govResNode)
{
    for (; greatGrandson; greatGrandson = greatGrandson->next) {
        if (!xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("default"))) {
            char* def = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
            if (!def || !IsNumber(def)) {
                SOC_PERF_LOGE("Invalid governor resource default for %{private}s", configFile.c_str());
                xmlFree(def);
                return false;
            }
            govResNode->def = atoll(def);
            xmlFree(def);
        } else if (persistMode != REPORT_TO_PERFSO &&
            !xmlStrcmp(greatGrandson->name, reinterpret_cast<const xmlChar*>("path"))) {
            char* path = reinterpret_cast<char*>(xmlNodeGetContent(greatGrandson));
            if (!path) {
                SOC_PERF_LOGE("Invalid governor resource path for %{private}s", configFile.c_str());
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
                SOC_PERF_LOGE("Invalid governor resource node for %{private}s", configFile.c_str());
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

bool SocPerfConfig::LoadConfig(const xmlNode* rootNode, const std::string& configFile)
{
    bool configTag = IsConfigTag(rootNode);
    xmlNode* configNode = rootNode->children;
    if (configTag) {
        if (!HandleConfigNode(configNode, configFile)) {
            return false;
        }
    } else {
        if (!LoadConfigInfo(rootNode, configFile, DEFAULT_CONFIG_MODE)) {
            return false;
        }
    }

    if (!CheckActionResIdAndValueValid(configFile)) {
        return false;
    }

    return true;
}

bool SocPerfConfig::HandleConfigNode(const xmlNode* configNode, const std::string& configFile)
{
    for (; configNode; configNode = configNode->next) { // Iterate all Config
        if (!xmlStrcmp(configNode->name, reinterpret_cast<const xmlChar*>("Config"))) {
            std::string configMode = GetConfigMode(configNode);
            if (configMode.empty()) {
                configMode = DEFAULT_CONFIG_MODE;
            }
            if (!LoadConfigInfo(configNode, configFile, configMode)) {
                return false;
            }
        }
    }
    return true;
}

std::string SocPerfConfig::GetConfigMode(const xmlNode* node)
{
    const xmlChar* configModeXml = xmlGetProp(node, reinterpret_cast<const xmlChar*>("mode"));
    if (configModeXml == nullptr) {
        return "";
    }
    char* configMode =  reinterpret_cast<char*>(const_cast<xmlChar*>(configModeXml));
    std::string configModeStr(configMode);
    xmlFree(configMode);
    return configModeStr;
}

bool SocPerfConfig::IsConfigTag(const xmlNode* rootNode)
{
    xmlNode* child = rootNode->children;
    for (; child; child = child->next) {
        if (!xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("Config"))) {
            return true;
        }
    }
    return false;
}

bool SocPerfConfig::LoadConfigInfo(const xmlNode* configNode, const std::string& configFile,
    const std::string& configMode)
{
    xmlNode* child = configNode->children;
    for (; child; child = child->next) { // Iterate all cmdID
        if (!xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("cmd"))) {
            if (!LoadCmdInfo(child, configFile, configMode)) {
                return false;
            }
        } else if (!xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("interaction"))) {
            LoadInterAction(child, configFile);
        }
    }
    return true;
}

bool SocPerfConfig::LoadCmdInfo(const xmlNode* child, const std::string& configFile, const std::string& configMode)
{
    char* id = reinterpret_cast<char*>(xmlGetProp(child, reinterpret_cast<const xmlChar*>("id")));
    char* name = reinterpret_cast<char*>(xmlGetProp(child, reinterpret_cast<const xmlChar*>("name")));
    if (!CheckCmdTag(id, name, configFile)) {
        xmlFree(id);
        xmlFree(name);
        return false;
    }

    auto config = configPerfActionsInfo_.find(configMode);
    std::unordered_map<int32_t, std::shared_ptr<Actions>> perfActionsInfo = {};
    if (config != configPerfActionsInfo_.end()) {
        perfActionsInfo = config->second;
    }

    auto it = perfActionsInfo.find(atoi(id));
    if (it != perfActionsInfo.end()) {
        xmlFree(id);
        xmlFree(name);
        return true;
    }
        
    xmlNode* grandson = child->children;
    std::shared_ptr<Actions> actions = std::make_shared<Actions>(atoi(id), name);
    xmlFree(id);
    xmlFree(name);

    char* interaction = reinterpret_cast<char*>(xmlGetProp(child, reinterpret_cast<const xmlChar*>("interaction")));
    if (interaction) {
        if (atoi(interaction) == 0) {
            actions->interaction = false;
        }
        xmlFree(interaction);
    }

    char* mode = reinterpret_cast<char*>(xmlGetProp(child, reinterpret_cast<const xmlChar*>("mode")));
    if (mode) {
        if (configMode == DEFAULT_CONFIG_MODE) {
            ParseModeCmd(mode, configFile, actions);
        }
        xmlFree(mode);
    }

    if (!TraversalBoostResource(grandson, configFile, actions)) {
        return false;
    }

    perfActionsInfo.insert(std::pair<int32_t, std::shared_ptr<Actions>>(actions->id, actions));
    configPerfActionsInfo_[configMode] = perfActionsInfo;
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
            SOC_PERF_LOGW("Invaild device mode pair for %{private}s", configFile.c_str());
            continue;
        }

        std::string modeDeviceStr = itemPair[0];
        std::string modeCmdIdStr = itemPair[RES_MODE_AND_ID_PAIR -1];
        if (modeDeviceStr.empty() || !IsNumber(modeCmdIdStr)) {
            SOC_PERF_LOGW("Invaild device mode name for %{private}s", configFile.c_str());
            continue;
        }

        int32_t cmdId = atoi(modeCmdIdStr.c_str());
        auto existMode = std::find_if(actions->modeMap.begin(), actions->modeMap.end(),
            [&modeDeviceStr](const std::shared_ptr<ModeMap>& modeItem) {
            return modeItem->mode == modeDeviceStr;
        });
        if (existMode != actions->modeMap.end()) {
            (*existMode)->cmdId = cmdId;
        } else {
            std::shared_ptr<ModeMap> newMode = std::make_shared<ModeMap>(modeDeviceStr, cmdId);
            actions->modeMap.push_back(newMode);
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
        SOC_PERF_LOGE("Invalid cmd duration for %{private}s", configFile.c_str());
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
        SOC_PERF_LOGE("Invalid cmd resource(%{public}s) for %{private}s", resStr, configFile.c_str());
        xmlFree(resValue);
        return false;
    }
    action->variable.push_back(g_resStrToIdInfo[resStr]);
    action->variable.push_back(atoll(resValue));
    xmlFree(resValue);
    return true;
}

int32_t SocPerfConfig::GetXmlIntProp(const xmlNode* xmlNode, const char* propName, int32_t def) const
{
    int ret = def;
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
        action->thermalLvl_ = GetXmlIntProp(grandson, "thermalLvl", INVALID_THERMAL_LVL);
        if (action->thermalLvl_ != INVALID_THERMAL_LVL) {
            if (minThermalLvl_ == INVALID_THERMAL_LVL || minThermalLvl_ > action->thermalLvl_) {
                minThermalLvl_ = action->thermalLvl_;
            }
        }
        action->thermalCmdId_ = GetXmlIntProp(grandson, "thermalCmdId", INVALID_THERMAL_CMD_ID);
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
        SOC_PERF_LOGE("Invalid resource id for %{private}s", configFile.c_str());
        return false;
    }
    if (!name) {
        SOC_PERF_LOGE("Invalid resource name for %{private}s", configFile.c_str());
        return false;
    }
    if (pair && (!IsNumber(pair) || !IsValidRangeResId(atoi(pair)))) {
        SOC_PERF_LOGE("Invalid resource pair for %{private}s", configFile.c_str());
        return false;
    }
    if (mode && !IsNumber(mode)) {
        SOC_PERF_LOGE("Invalid resource mode for %{private}s", configFile.c_str());
        return false;
    }
    return CheckResourcePersistMode(persistMode, configFile);
}

bool SocPerfConfig::CheckResourcePersistMode(const char* persistMode, const std::string& configFile) const
{
    if (persistMode && (!IsNumber(persistMode) || !IsValidPersistMode(atoi(persistMode)))) {
        SOC_PERF_LOGE("Invalid resource persistMode for %{private}s", configFile.c_str());
        return false;
    }
    return true;
}

bool SocPerfConfig::CheckResourceTag(int32_t persistMode, const char* def,
    const char* path, const std::string& configFile) const
{
    if (!def || !IsNumber(def)) {
        SOC_PERF_LOGE("Invalid resource default for %{private}s", configFile.c_str());
        return false;
    }
    if (persistMode != REPORT_TO_PERFSO && !path) {
        SOC_PERF_LOGE("Invalid resource path for %{private}s", configFile.c_str());
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
            resNode->available.insert(atoll(str.c_str()));
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
        SOC_PERF_LOGE("Invalid governor resource id for %{private}s", configFile.c_str());
        return false;
    }
    if (!name) {
        SOC_PERF_LOGE("Invalid governor resource name for %{private}s", configFile.c_str());
        return false;
    }
    if (persistMode && (!IsNumber(persistMode) || !IsValidPersistMode(atoi(persistMode)))) {
        SOC_PERF_LOGE("Invalid governor resource persistMode for %{private}s", configFile.c_str());
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
        SOC_PERF_LOGE("Invalid cmd id for %{private}s", configFile.c_str());
        return false;
    }
    if (!name) {
        SOC_PERF_LOGE("Invalid cmd name for %{private}s", configFile.c_str());
        return false;
    }
    return true;
}

bool SocPerfConfig::TraversalActions(std::shared_ptr<Action> action, int32_t actionId)
{
    for (int32_t i = 0; i < (int32_t)action->variable.size() - 1; i += RES_ID_AND_VALUE_PAIR) {
        int32_t resId = action->variable[i];
        int64_t resValue = action->variable[i + 1];
        if (resourceNodeInfo_.find(resId) != resourceNodeInfo_.end() && resourceNodeInfo_[resId] != nullptr) {
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
    std::unordered_map<std::string,
        std::unordered_map<int32_t, std::shared_ptr<Actions>>> configs = configPerfActionsInfo_;
    for (auto configsIter = configs.begin(); configsIter != configs.end(); ++configsIter) {
        std::unordered_map<int32_t, std::shared_ptr<Actions>> actionsInfo = configsIter->second;
        if (!CheckActionsValid(actionsInfo)) {
            return false;
        }
    }
    return true;
}

bool SocPerfConfig::CheckActionsValid(std::unordered_map<int32_t, std::shared_ptr<Actions>>& actionsInfo)
{
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
} // namespace SOCPERF
} // namespace OHOS
