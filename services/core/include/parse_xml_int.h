/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SOC_PERF_SERVICES_CORE_INCLUDE_PARSE_XML_INT_H
#define SOC_PERF_SERVICES_CORE_INCLUDE_PARSE_XML_INT_H

#include <charconv>
#include <cstdint>
#include <string>
#include <system_error>

namespace OHOS {
namespace SOCPERF {
inline bool ParseXmlInt32(const char *text, int32_t &out)
{
    if (text == nullptr || *text == '\0') {
        return false;
    }
    int32_t value = 0;
    const char *last = text + std::char_traits<char>::length(text);
    auto result = std::from_chars(text, last, value);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }
    out = value;
    return true;
}

inline bool ParseXmlInt64(const char *text, int64_t &out)
{
    if (text == nullptr || *text == '\0') {
        return false;
    }
    int64_t value = 0;
    const char *last = text + std::char_traits<char>::length(text);
    auto result = std::from_chars(text, last, value);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }
    out = value;
    return true;
}
} // namespace SOCPERF
} // namespace OHOS
#endif
