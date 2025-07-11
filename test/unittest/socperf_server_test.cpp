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

#define private public
#define protected public

#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include "socperf_config.h"
#include "isoc_perf.h"
#include "socperf_server.h"
#include "socperf.h"

using namespace testing::ext;
using namespace testing::mt;

namespace OHOS {
namespace SOCPERF {
class SocPerfServerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
private:
    std::shared_ptr<SocPerfServer> socPerfServer_ = DelayedSingleton<SocPerfServer>::GetInstance();
};

void SocPerfServerTest::SetUpTestCase(void)
{
}

void SocPerfServerTest::TearDownTestCase(void)
{
}

void SocPerfServerTest::SetUp(void)
{
}

void SocPerfServerTest::TearDown(void)
{
}

/*
 * @tc.name: SocPerfServerTest_Init_Config_001
 * @tc.desc: test init config
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_Init_Config_001, Function | MediumTest | Level0)
{
    socPerfServer_->OnStart();
    sleep(1);
    EXPECT_TRUE(socPerfServer_->socPerf.enabled_);
}

/*
 * @tc.name: SocPerfServerTest_SocPerfAPI_001
 * @tc.desc: test socperf api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocPerfAPI_001, Function | MediumTest | Level0)
{
    std::string msg = "testBoost";
    socPerfServer_->socPerf.PerfRequest(10010, msg);
    socPerfServer_->socPerf.PerfRequestEx(10000, true, msg);
    socPerfServer_->socPerf.PerfRequestEx(10000, false, msg);
    socPerfServer_->socPerf.PerfRequestEx(10028, true, msg);
    socPerfServer_->socPerf.PerfRequestEx(10028, false, msg);
    socPerfServer_->socPerf.LimitRequest(ActionType::ACTION_TYPE_POWER, {1001}, {999000}, msg);
    socPerfServer_->socPerf.LimitRequest(ActionType::ACTION_TYPE_THERMAL, {1001}, {999000}, msg);
    socPerfServer_->socPerf.LimitRequest(ActionType::ACTION_TYPE_POWER, {1001}, {1325000}, msg);
    socPerfServer_->socPerf.LimitRequest(ActionType::ACTION_TYPE_THERMAL, {1001}, {1325000}, msg);
    socPerfServer_->socPerf.PowerLimitBoost(true, msg);
    socPerfServer_->socPerf.ThermalLimitBoost(true, msg);
    EXPECT_EQ(msg, "testBoost");
    std::string id = "1000";
    std::string name = "lit_cpu_freq";
    std::string pair = "1001";
    std::string mode = "1";
    std::string persisMode = "1";
    std::string configFile = "";
    bool ret = socPerfServer_->socPerf.socPerfConfig_.CheckResourceTag(id.c_str(),
        name.c_str(), pair.c_str(), mode.c_str(),
        persisMode.c_str(), configFile.c_str());
    EXPECT_TRUE(ret);
    ret = socPerfServer_->socPerf.socPerfConfig_.CheckResourceTag(nullptr, name.c_str(), pair.c_str(), mode.c_str(),
        persisMode.c_str(), configFile.c_str());
    EXPECT_FALSE(ret);
}

/*
 * @tc.name: SocPerfServerTest_SocPerfServerAPI_000
 * @tc.desc: test socperf server api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocPerfServerAPI_000, Function | MediumTest | Level0)
{
    std::string msg = "testBoost";
    socPerfServer_->PerfRequest(10010, msg);
    socPerfServer_->PerfRequestEx(10000, true, msg);
    socPerfServer_->PerfRequestEx(10000, false, msg);
    socPerfServer_->LimitRequest(ActionType::ACTION_TYPE_POWER, {1001}, {1364000}, msg);
    socPerfServer_->LimitRequest(ActionType::ACTION_TYPE_POWER, {11001}, {2}, msg);
    socPerfServer_->LimitRequest(ActionType::ACTION_TYPE_MAX, {11001}, {2}, msg);
    socPerfServer_->PowerLimitBoost(true, msg);
    socPerfServer_->LimitRequest(ActionType::ACTION_TYPE_THERMAL, {1001}, {1364000}, msg);
    socPerfServer_->ThermalLimitBoost(true, msg);
    socPerfServer_->PowerLimitBoost(false, msg);
    socPerfServer_->ThermalLimitBoost(false, msg);
    bool allowDump = socPerfServer_->AllowDump();
    EXPECT_TRUE(allowDump);
    int32_t fd = -1;
    std::vector<std::u16string> args = {to_utf16("1"), to_utf16("2"), to_utf16("3"), to_utf16("-a"), to_utf16("-h")};
    socPerfServer_->Dump(fd, args);
    socPerfServer_->OnStop();
    EXPECT_EQ(msg, "testBoost");
}

/*
 * @tc.name: SocPerfSubTest_RequestCmdIdCount_001
 * @tc.desc: RequestCmdIdCount
 * @tc.type FUNC
 * @tc.require: issueI9H4NS
 */
HWTEST_F(SocPerfServerTest, SocPerfSubTest_RequestCmdIdCount_001, Function | MediumTest | Level0)
{
    int firstCheckColdStartNum = 0;
    int secondCheckColdStartNum = 0;
    map<int, int> myMap;
    char colon, comma;
    int key, value;

    std::string ret = socPerfServer_->socPerf.RequestCmdIdCount("");
    std::stringstream ssfirst(ret);
    while (ssfirst >> key >> colon >> value >> comma) {
        myMap[key] = value;
    }
    ssfirst >> key >> colon >> value;
    myMap[key] = value;
    firstCheckColdStartNum = myMap[10010];

    sleep(1);
    std::string msg = "testBoost";
    socPerfServer_->PerfRequest(10010, msg);

    ret = socPerfServer_->socPerf.RequestCmdIdCount("");

    socPerfServer_->RequestCmdIdCount(msg, msg);
    std::stringstream sssecond(ret);
    while (sssecond >> key >> colon >> value >> comma) {
        myMap[key] = value;
    }
    sssecond >> key >> colon >> value;
    myMap[key] = value;
    secondCheckColdStartNum = myMap[10010];

    EXPECT_TRUE(secondCheckColdStartNum == firstCheckColdStartNum + 1);
}

/*
 * @tc.name: SocPerfServerTest_SocPerfServerAPI_001
 * @tc.desc: test socperf server api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocPerfServerAPI_001, Function | MediumTest | Level0)
{
    std::string msg = "";
    socPerfServer_->SetRequestStatus(false, msg);
    socPerfServer_->socPerf.ClearAllAliveRequest();
    EXPECT_FALSE(socPerfServer_->socPerf.perfRequestEnable_);
    auto socPerfThreadWrap = std::make_shared<SocPerfThreadWrap>();
    socPerfThreadWrap->ClearAllAliveRequest();
    for (const std::pair<int32_t, std::shared_ptr<ResStatus>>& item : socPerfThreadWrap->resStatusInfo_) {
        if (item.second == nullptr) {
            continue;
        }
        std::list<std::shared_ptr<ResAction>>& resActionList = item.second->resActionList[ACTION_TYPE_PERF];
        EXPECT_TRUE(resActionList.empty());
    }
}

/*
 * @tc.name: SocPerfServerTest_SocPerfServerAPI_002
 * @tc.desc: test socperf server api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocPerfServerAPI_002, Function | MediumTest | Level0)
{
    std::string msg = "test";
    socPerfServer_->RequestDeviceMode(msg, true);
    auto iter = socPerfServer_->socPerf.recordDeviceMode_.find(msg);
    EXPECT_TRUE(iter == socPerfServer_->socPerf.recordDeviceMode_.end());

    socPerfServer_->RequestDeviceMode(msg, false);
    auto iter2 = socPerfServer_->socPerf.recordDeviceMode_.find(msg);
    EXPECT_TRUE(iter2 == socPerfServer_->socPerf.recordDeviceMode_.end());

    std::string msgEmpty = "";
    socPerfServer_->RequestDeviceMode("", true);
    auto iter3 = socPerfServer_->socPerf.recordDeviceMode_.find(msgEmpty);
    EXPECT_TRUE(iter3 == socPerfServer_->socPerf.recordDeviceMode_.end());

    std::string msgMax = "ABCDEFGHABCDEFGHABCDEFGHABCDEFGHABCDEFGHABCDEFGHABCDEFGHABCDEFGHZ";
    socPerfServer_->RequestDeviceMode(msgMax, true);
    auto iter4 = socPerfServer_->socPerf.recordDeviceMode_.find(msgMax);
    EXPECT_TRUE(iter4 == socPerfServer_->socPerf.recordDeviceMode_.end());

    std::string msgWeakInteractionStatus = "actionmode:weakaction";
    socPerfServer_->RequestDeviceMode(msgWeakInteractionStatus, true);
    auto iter5 = socPerfServer_->socPerf.recordDeviceMode_.find(msgWeakInteractionStatus);
    EXPECT_TRUE(iter5 == socPerfServer_->socPerf.recordDeviceMode_.end());

    std::string msgWeakInteractionError = "actionmode:error";
    socPerfServer_->RequestDeviceMode(msgWeakInteractionError, true);
    auto iter6 = socPerfServer_->socPerf.recordDeviceMode_.find(msgWeakInteractionError);
    EXPECT_TRUE(iter6 == socPerfServer_->socPerf.recordDeviceMode_.end());
}

/*
 * @tc.name: SocPerfServerTest_SocperfMatchCmd_001
 * @tc.desc: test socperf MatchDeviceModeCmd func
 * @tc.type FUNC
 * @tc.require: issueI9GCD8
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocperfMatchDeviceCmd_001, Function | MediumTest | Level0)
{
    std::string modeStr = "displayMain";
    int32_t cmdTest = 10000;
    std::unordered_map<int32_t, std::shared_ptr<Actions>> perfActionsInfo =
        socPerfServer_->socPerf.socPerfConfig_.configPerfActionsInfo_[DEFAULT_CONFIG_MODE];
    auto it_actions = perfActionsInfo.find(cmdTest);
    if (it_actions == perfActionsInfo.end()) {
        EXPECT_EQ(modeStr, "displayMain");
        return;
    }
    std::shared_ptr<Actions> actions = perfActionsInfo[cmdTest];
    if (actions->modeMap.empty()) {
        std::shared_ptr<ModeMap> newMode = std::make_shared<ModeMap>(modeStr, cmdTest);
        actions->modeMap.push_back(newMode);
    }

    // case : normal match
    int32_t ret = socPerfServer_->socPerf.MatchDeviceModeCmd(cmdTest, false);
    auto iter_match = std::find_if(actions->modeMap.begin(), actions->modeMap.end(),
        [&modeStr](const std::shared_ptr<ModeMap>& modeItem) {
        return modeItem->mode == modeStr;
    });
    if (iter_match != actions->modeMap.end()) {
        EXPECT_EQ(ret, (*iter_match)->cmdId);
    } else {
        EXPECT_EQ(ret, cmdTest);
    }

    // case : match cmdid is not exist branch
    int32_t cmdInvaild = 60000;
    auto iter_invaild = perfActionsInfo.find(cmdInvaild);
    if (iter_invaild != perfActionsInfo.end()) {
        EXPECT_EQ(cmdInvaild, 60000);
    } else {
        auto iter_mode = std::find_if(actions->modeMap.begin(), actions->modeMap.end(),
            [&modeStr](const std::shared_ptr<ModeMap>& modeItem) {
            return modeItem->mode == modeStr;
        });
        if (iter_mode == actions->modeMap.end()) {
            EXPECT_EQ(cmdInvaild, 60000);
        } else {
            (*iter_mode)->cmdId = cmdInvaild;
            int32_t retInvaild = socPerfServer_->socPerf.MatchDeviceModeCmd(cmdTest, false);
            EXPECT_EQ(retInvaild, cmdTest);
        }
    }

    // case : no match mode
    std::string modeInvaild = "test";
    socPerfServer_->RequestDeviceMode(modeStr, false);
    socPerfServer_->RequestDeviceMode(modeInvaild, true);
    int32_t retModeInvaild = socPerfServer_->socPerf.MatchDeviceModeCmd(cmdTest, false);
    EXPECT_EQ(retModeInvaild, cmdTest);
}

/*
 * @tc.name: SocPerfServerTest_SocperfMatchCmd_002
 * @tc.desc: test socperf MatchDeviceModeCmd func
 * @tc.type FUNC
 * @tc.require: issueI9GCD8
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocperfMatchCmd_002, Function | MediumTest | Level0)
{
    std::string modeStr = "displayMainTest";
    int32_t cmdTest = 10000;
    int32_t cmdMatch = 10001;
    std::unordered_map<int32_t, std::shared_ptr<Actions>> perfActionsInfo =
        socPerfServer_->socPerf.socPerfConfig_.configPerfActionsInfo_[DEFAULT_CONFIG_MODE];
    auto it_actions = perfActionsInfo.find(cmdTest);
    if (it_actions == perfActionsInfo.end()) {
        EXPECT_EQ(modeStr, "displayMainTest");
        return;
    }
    std::shared_ptr<Actions> actions = perfActionsInfo[cmdTest];
    actions->isLongTimePerf = false;
    std::shared_ptr<ModeMap> newMode = std::make_shared<ModeMap>(modeStr, cmdMatch);
    actions->modeMap.push_back(newMode);

    auto it_match = perfActionsInfo.find(cmdMatch);
    if (it_match == perfActionsInfo.end()) {
        EXPECT_EQ(modeStr, "displayMainTest");
        return;
    }

    // case : match cmdid is long time perf branch
    std::shared_ptr<Actions> actionsMatch = perfActionsInfo[cmdMatch];
    actionsMatch->isLongTimePerf = true;
    int32_t retInvaild = socPerfServer_->socPerf.MatchDeviceModeCmd(cmdTest, true);
    EXPECT_EQ(retInvaild, cmdTest);
}

/*
 * @tc.name: SocPerfServerTest_SocperfMatchCmd_003
 * @tc.desc: test socperf MatchDeviceModeCmd func
 * @tc.type FUNC
 * @tc.require: issueI9GCD8
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocperfMatchCmd_003, Function | MediumTest | Level0)
{
    std::string modeStr = "displayMainTest";
    int32_t cmdTest = 10002;
    std::unordered_map<int32_t, std::shared_ptr<Actions>> perfActionsInfo =
        socPerfServer_->socPerf.socPerfConfig_.configPerfActionsInfo_[DEFAULT_CONFIG_MODE];
    auto it_actions = perfActionsInfo.find(cmdTest);
    if (it_actions == perfActionsInfo.end()) {
        EXPECT_EQ(modeStr, "displayMainTest");
        return;
    }

    std::shared_ptr<Actions> actions = perfActionsInfo[cmdTest];
    std::shared_ptr<ModeMap> newMode = std::make_shared<ModeMap>(modeStr, cmdTest);
    actions->modeMap.push_back(newMode);
    socPerfServer_->socPerf.recordDeviceMode_.clear();

    // case : match device mode is empty branch
    int32_t retInvaild = socPerfServer_->socPerf.MatchDeviceModeCmd(cmdTest, true);
    EXPECT_EQ(retInvaild, cmdTest);
}

/*
 * @tc.name: SocPerfServerTest_SocperfParseModeCmd_001
 * @tc.desc: test socperf ParseModeCmd func
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocperfParseModeCmd_001, Function | MediumTest | Level0)
{
    const char modePairInvaild[] = "parseTest";
    const char modeNumberInvaild[] = "parseTest=abc";
    const char modeCmdInvaild[] = "=12345";
    const char modeSame[] = "parseTest=12345|parseTest=23456";
    std::string cfgFile = "bootest.xml";
    int32_t cmdTest = 10002;
    int32_t exceptSame = 23456;
    std::string deviceMode = "parseTest";

    std::unordered_map<int32_t, std::shared_ptr<Actions>> perfActionsInfo =
        socPerfServer_->socPerf.socPerfConfig_.configPerfActionsInfo_[DEFAULT_CONFIG_MODE];
    auto it_actions = perfActionsInfo.find(cmdTest);
    if (it_actions == perfActionsInfo.end()) {
        EXPECT_EQ(cmdTest, 10002);
        return;
    }

    std::shared_ptr<Actions> actions = perfActionsInfo[cmdTest];
    socPerfServer_->socPerf.socPerfConfig_.ParseModeCmd(modePairInvaild, cfgFile, actions);
    auto iter_mode = std::find_if(actions->modeMap.begin(), actions->modeMap.end(),
        [&deviceMode](const std::shared_ptr<ModeMap>& modeItem) {
        return modeItem->mode == deviceMode;
    });
    EXPECT_TRUE(iter_mode == actions->modeMap.end());

    socPerfServer_->socPerf.socPerfConfig_.ParseModeCmd(modeNumberInvaild, cfgFile, actions);
    iter_mode = std::find_if(actions->modeMap.begin(), actions->modeMap.end(),
        [&deviceMode](const std::shared_ptr<ModeMap>& modeItem) {
        return modeItem->mode == deviceMode;
    });
    EXPECT_TRUE(iter_mode == actions->modeMap.end());

    socPerfServer_->socPerf.socPerfConfig_.ParseModeCmd(modeCmdInvaild, cfgFile, actions);
    iter_mode = std::find_if(actions->modeMap.begin(), actions->modeMap.end(),
        [&deviceMode](const std::shared_ptr<ModeMap>& modeItem) {
        return modeItem->mode == deviceMode;
    });
    EXPECT_TRUE(iter_mode == actions->modeMap.end());

    int32_t size = actions->modeMap.size();
    socPerfServer_->socPerf.socPerfConfig_.ParseModeCmd(modeSame, cfgFile, actions);
    EXPECT_EQ(size + 1, actions->modeMap.size());
    auto iterSame = std::find_if(actions->modeMap.begin(), actions->modeMap.end(),
        [&deviceMode](const std::shared_ptr<ModeMap>& modeItem) {
        return modeItem->mode == deviceMode;
    });
    ASSERT_TRUE(iterSame != actions->modeMap.end());
    EXPECT_EQ(exceptSame, (*iterSame)->cmdId);

    int32_t sizeBefore = actions->modeMap.size();
    const char *modeNullInvaild = nullptr;
    socPerfServer_->socPerf.socPerfConfig_.ParseModeCmd(modeNullInvaild, cfgFile, actions);
    EXPECT_EQ(sizeBefore, actions->modeMap.size());
}

/*
 * @tc.name: SocPerfServerTest_SocperfThreadWrapp_001
 * @tc.desc: test log switch func
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SocperfThreadWrapp_001, Function | MediumTest | Level0)
{
    std::string msg = "";
    auto socPerfThreadWrap = std::make_shared<SocPerfThreadWrap>();
    socPerfThreadWrap->InitResourceNodeInfo();
    socPerfThreadWrap->socPerfConfig_.InitPerfFunc(nullptr, nullptr, nullptr);
    socPerfThreadWrap->socPerfConfig_.InitPerfFunc(nullptr, msg.c_str(), msg.c_str());
    socPerfThreadWrap->socPerfConfig_.InitPerfFunc(msg.c_str(), nullptr, nullptr);
    socPerfThreadWrap->socPerfConfig_.InitPerfFunc(msg.c_str(), msg.c_str(), msg.c_str());
    socPerfThreadWrap->DoFreqActionPack(nullptr);
    socPerfThreadWrap->UpdateLimitStatus(0, nullptr, 0);
    socPerfThreadWrap->DoFreqAction(0, nullptr);
    socPerfThreadWrap->DoFreqAction(1000, nullptr);
    EXPECT_NE(msg.c_str(), "-1");
    bool ret = false;
    int inValidResId = 9999;
    ret = socPerfThreadWrap->socPerfConfig_.IsValidResId(inValidResId);
    EXPECT_FALSE(ret);
    ret = socPerfThreadWrap->socPerfConfig_.IsGovResId(inValidResId);
    EXPECT_FALSE(ret);
    int32_t level = 10;
    int64_t value = 0;
    ret = socPerfThreadWrap->GetResValueByLevel(inValidResId, level, value);
    EXPECT_FALSE(ret);
}

class SocperfStubTest : public SocPerfStub {
public:
    SocperfStubTest() {}
    ErrCode PerfRequest(int32_t cmdId, const std::string& msg) override
    {
        return ERR_OK;
    }
    ErrCode PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg) override
    {
        return ERR_OK;
    }
    ErrCode PowerLimitBoost(bool onOffTag, const std::string& msg) override
    {
        return ERR_OK;
    }
    ErrCode ThermalLimitBoost(bool onOffTag, const std::string& msg) override
    {
        return ERR_OK;
    }
    ErrCode LimitRequest(int32_t clientId,
        const std::vector<int32_t>& tags, const std::vector<int64_t>& configs, const std::string& msg) override
    {
        return ERR_OK;
    }
    ErrCode SetRequestStatus(bool status, const std::string& msg) override
    {
        return ERR_OK;
    }
    ErrCode SetThermalLevel(int32_t level) override
    {
        return ERR_OK;
    }
    ErrCode RequestDeviceMode(const std::string& mode, bool status) override
    {
        return ERR_OK;
    }
    ErrCode RequestCmdIdCount(const std::string& msg, std::string& funcResult) override
    {
        return ERR_OK;
    }
};

/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_001
 * @tc.desc: test socperf stub api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_001, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteInt32(10000);
    data.WriteString("");
    MessageParcel reply;
    MessageOption option;
    uint32_t requestIpcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_PERF_REQUEST);
    int32_t ret = socPerfStub.OnRemoteRequest(requestIpcId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_002
 * @tc.desc: test socperf stub api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_002, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteInt32(10000);
    data.WriteBool(true);
    data.WriteString("");
    MessageParcel reply;
    MessageOption option;
    uint32_t requestExIpcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_PERF_REQUEST_EX);
    int32_t ret = socPerfStub.OnRemoteRequest(requestExIpcId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_003
 * @tc.desc: test socperf stub api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_003, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteInt32(1);
    std::vector<int32_t> tags = {1001};
    data.WriteInt32Vector(tags);
    std::vector<int64_t> configs = {1416000};
    data.WriteInt64Vector(configs);
    data.WriteString("");
    MessageParcel reply;
    MessageOption option;
    uint32_t powerLimitId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_LIMIT_REQUEST);
    int32_t ret = socPerfStub.OnRemoteRequest(powerLimitId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_004
 * @tc.desc: test socperf stub api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_004, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteBool(true);
    data.WriteString("");
    MessageParcel reply;
    MessageOption option;
    uint32_t powerLimitIpcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_POWER_LIMIT_BOOST);
    int32_t ret = socPerfStub.OnRemoteRequest(powerLimitIpcId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_005
 * @tc.desc: test socperf stub api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_005, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteBool(true);
    data.WriteString("");
    MessageParcel reply;
    MessageOption option;
    uint32_t thermalLimitIpcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_THERMAL_LIMIT_BOOST);
    int32_t ret = socPerfStub.OnRemoteRequest(thermalLimitIpcId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_006
 * @tc.desc: test socperf stub api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_006, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteBool(true);
    MessageParcel reply;
    MessageOption option;
    uint32_t ipcId = 0x000f;
    int32_t ret = socPerfStub.OnRemoteRequest(ipcId, data, reply, option);
    EXPECT_NE(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_007
 * @tc.desc: test socperf stub api
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_007, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteBool(false);
    data.WriteString("");
    MessageParcel reply;
    MessageOption option;
    uint32_t ipcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_SET_REQUEST_STATUS);
    int32_t ret = socPerfStub.OnRemoteRequest(ipcId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);

    MessageParcel dataPerf;
    dataPerf.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    dataPerf.WriteInt32(10000);
    dataPerf.WriteString("");
    MessageParcel replyPerf;
    MessageOption optionPerf;
    uint32_t requestPerfIpcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_PERF_REQUEST);
    ret = socPerfStub.OnRemoteRequest(requestPerfIpcId, dataPerf, replyPerf, optionPerf);
    EXPECT_EQ(ret, ERR_OK);

    MessageParcel dataLimit;
    dataLimit.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    dataLimit.WriteInt32(1);
    std::vector<int32_t> tags = {1001};
    dataLimit.WriteInt32Vector(tags);
    std::vector<int64_t> configs = {1416000};
    dataLimit.WriteInt64Vector(configs);
    dataLimit.WriteString("");
    MessageParcel replyLimit;
    MessageOption optionLimit;
    uint32_t powerLimitId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_LIMIT_REQUEST);
    ret = socPerfStub.OnRemoteRequest(powerLimitId, dataLimit, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}


/*
 * @tc.name: SocPerfStubTest_SocPerfServerAPI_008
 * @tc.desc: test socperf requet device mode stub api
 * @tc.type FUNC
 * @tc.require: issue#I95U8S
 */
HWTEST_F(SocPerfServerTest, SocPerfStubTest_SocPerfServerAPI_008, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteString("test");
    data.WriteBool(true);
    MessageParcel reply;
    MessageOption option;
    uint32_t ipcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_REQUEST_DEVICE_MODE);
    int32_t ret = socPerfStub.OnRemoteRequest(ipcId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfServerTest_SetThermalLevel_001
 * @tc.desc: perf request lvl server API
 * @tc.type FUNC
 * @tc.require: issue#I95U8S
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SetThermalLevel_Server_001, Function | MediumTest | Level0)
{
    SocperfStubTest socPerfStub;
    MessageParcel data;
    data.WriteInterfaceToken(SocPerfStub::GetDescriptor());
    data.WriteInt32(3);
    MessageParcel reply;
    MessageOption option;
    uint32_t ipcId = static_cast<uint32_t>(ISocPerfIpcCode::COMMAND_SET_THERMAL_LEVEL);
    int32_t ret = socPerfStub.OnRemoteRequest(ipcId, data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
}

/*
 * @tc.name: SocPerfServerTest_SetThermalLevel_Server_002
 * @tc.desc: perf request lvl server API
 * @tc.type FUNC
 * @tc.require: issue#I95U8S
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SetThermalLevel_Server_002, Function | MediumTest | Level0)
{
    socPerfServer_->SetThermalLevel(3);
    EXPECT_EQ(socPerfServer_->socPerf.thermalLvl_, 3);
}

/*
 * @tc.name: SocPerfServerTest_SetThermalLevel_Server_005
 * @tc.desc: perf request lvl server API
 * @tc.type FUNC
 * @tc.require: issue#I95U8S
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SetThermalLevel_Server_005, Function | MediumTest | Level0)
{
    int32_t litCpuMinFreq = 1000;
    int32_t litCpuMaxFreq = 1001;
    std::shared_ptr<SocPerfThreadWrap> socPerfThreadWrap = socPerfServer_->socPerf.socperfThreadWrap_;
    socPerfThreadWrap->resStatusInfo_[litCpuMinFreq]->candidatesValue[ACTION_TYPE_PERFLVL] = 1000;
    bool ret = socPerfThreadWrap->ArbitratePairResInPerfLvl(litCpuMinFreq);
    EXPECT_TRUE(ret);

    socPerfThreadWrap->resStatusInfo_[litCpuMinFreq]->candidatesValue[ACTION_TYPE_PERFLVL] = INVALID_VALUE;
    socPerfThreadWrap->resStatusInfo_[litCpuMaxFreq]->candidatesValue[ACTION_TYPE_PERFLVL] = 1000;
    ret = socPerfThreadWrap->ArbitratePairResInPerfLvl(litCpuMinFreq);
    EXPECT_TRUE(ret);

    socPerfThreadWrap->resStatusInfo_[litCpuMinFreq]->candidatesValue[ACTION_TYPE_PERFLVL] = INVALID_VALUE;
    socPerfThreadWrap->resStatusInfo_[litCpuMaxFreq]->candidatesValue[ACTION_TYPE_PERFLVL] = INVALID_VALUE;
    ret = socPerfThreadWrap->ArbitratePairResInPerfLvl(litCpuMinFreq);
    EXPECT_FALSE(ret);
}

/*
 * @tc.name: SocPerfServerTest_SetThermalLevel_Server_006
 * @tc.desc: perf request lvl server API
 * @tc.type FUNC
 * @tc.require: issue#I95U8S
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_SetThermalLevel_Server_006, Function | MediumTest | Level0)
{
    bool ret = socPerfServer_->socPerf.socPerfConfig_.Init();
    sleep(1);
    EXPECT_TRUE(ret);
}

/*
 * @tc.name: SocPerfServerTest_End_001
 * @tc.desc: perf end
 * @tc.type FUNC
 * @tc.require: issue#I95U8S
 */
HWTEST_F(SocPerfServerTest, SocPerfServerTest_End_001, Function | MediumTest | Level0)
{
    sleep(5);
    EXPECT_TRUE(socPerfServer_->socPerf.enabled_);
}
} // namespace SOCPERF
} // namespace OHOS