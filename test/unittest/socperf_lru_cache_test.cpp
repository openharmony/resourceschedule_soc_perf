/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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


#ifndef LRU_CACHE_TEST
#define LRU_CACHE_TEST

#define private public

#include "gtest/gtest.h"
#include "socperf_lru_cache.h"

namespace OHOS {
namespace SOCPERF {
class SocPerfLRUCacheTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
private:
    SocPerfLRUCache<int, std::string>* cache;
};

void SocPerfLRUCacheTest::SetUpTestCase() {}

void SocPerfLRUCacheTest::TearDownTestCase() {}

void SocPerfLRUCacheTest::SetUp()
{
    static const int maxSize = 2;
    cache = new ResschedLRUCache<int, std::string>(maxSize);
}

void SocPerfLRUCacheTest::TearDown()
{
    delete cache;
}

/**
 * @tc.name: SocPerfLRUCacheTest BasicPutGet
 * @tc.desc: test get
 * @tc.type: FUNC
 * @tc.require: issueICMIEN
 */
HWTEST_F(SocPerfLRUCacheTest, BasicPutGet, Function | MediumTest | Level0)
{
    cache->put(1, "one");
    std::string val = "";
    EXPECT_TRUE(cache->get(1, val));
    EXPECT_EQ(val, "one");
}

/**
 * @tc.name: SocPerfLRUCacheTest CacheEviction
 * @tc.desc: test CacheEviction
 * @tc.type: FUNC
 * @tc.require: issueICMIEN
 */
HWTEST_F(SocPerfLRUCacheTest, CacheEviction, Function | MediumTest | Level0)
{
    cache->put(1, "one");
    cache->put(2, "two");
    cache->put(3, "three");
    std::string val = "";
    EXPECT_FALSE(cache->get(1, val));
    EXPECT_TRUE(cache->get(2, val));
    EXPECT_TRUE(cache->get(3, val));
}

/**
 * @tc.name: SocPerfLRUCacheTest UpdateExistingKey
 * @tc.desc: test UpdateExistingKey
 * @tc.type: FUNC
 * @tc.require: issueICMIEN
 */
HWTEST_F(SocPerfLRUCacheTest, UpdateExistingKey, Function | MediumTest | Level0)
{
    cache->put(1, "one");
    cache->put(1, "new_one");
    std::string val = "";
    EXPECT_TRUE(cache->get(1, val));
    EXPECT_EQ(val, "new_one");
}

/**
 * @tc.name: SocPerfLRUCacheTest GetUpdatesLRU
 * @tc.desc: test GetUpdatesLRU
 * @tc.type: FUNC
 * @tc.require: issueICMIEN
 */
HWTEST_F(SocPerfLRUCacheTest, GetUpdatesLRU, Function | MediumTest | Level0)
{
    cache->put(1, "one");
    cache->put(2, "two");
    std::string val = "";
    cache->get(1, val);
    cache->put(3, "three");
    EXPECT_TRUE(cache->get(1, val));
    EXPECT_FALSE(cache->get(2, val));
    EXPECT_TRUE(cache->get(3, val));
}

} // namespace SOCPERF
} // namespace OHOS

#endif // LRU_CACHE_TEST
