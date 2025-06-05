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
#include "socperf_hitrace_chain.h"

using namespace testing::ext;
using namespace testing::mt;

namespace OHOS {
namespace SOCPERF {
class SocPerfHitraceChainTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SocPerfHitraceChainTest::SetUpTestCase(void)
{
}

void SocPerfHitraceChainTest::TearDownTestCase(void)
{
}

void SocPerfHitraceChainTest::SetUp(void)
{
}

void SocPerfHitraceChainTest::TearDown(void)
{
}

/*
 * @tc.name: SocPerfHitraceChainTest : SocPerfHitraceChainTest__001
 * @tc.desc: SocPerfHitraceChainTest__001
 * @tc.type FUNC
 * @tc.require: issueI78T3V
 */
HWTEST_F(SocPerfHitraceChainTest, SocPerfHitraceChainTest__001, Function | MediumTest | Level0)
{
    SocPerfHiTraceChain traceChain(__func__);
    EXPECT_TRUE(traceChain.isBegin_);
    EXPECT_TRUE(HiTraceChainIsValid(&traceChain.traceId_));
}

} // namespace SOCPERF
} // namespace OHOS