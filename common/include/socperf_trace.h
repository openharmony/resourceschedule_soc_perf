/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef SOC_PERF_COMMON_INCLUDE_SOCPERF_TRACE_H
#define SOC_PERF_COMMON_INCLUDE_SOCPERF_TRACE_H

#include "hisysevent.h"
#include "hitrace_meter.h"

namespace OHOS {
namespace SOCPERF {
#define HITRACE_TAG_SOCPERF (HITRACE_TAG_OHOS | HITRACE_TAG_APP)

#define SOCPERF_TRACE_BEGIN(info) StartTrace(HITRACE_TAG_SOCPERF, info)
#define SOCPERF_TRACE_END() FinishTrace(HITRACE_TAG_SOCPERF)
#define SOCPERF_TRACE_COUNTTRACE(name, value) CountTrace(HITRACE_TAG_SOCPERF, name, value)
} // namespace SOCPERF
} // namespace OHOS

#endif // SOC_PERF_COMMON_INCLUDE_SOCPERF_TRACE_H
