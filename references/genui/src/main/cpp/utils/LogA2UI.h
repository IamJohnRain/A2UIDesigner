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

#ifndef A2UI_LOG_A2UI_H
#define A2UI_LOG_A2UI_H

#include <hilog/log.h>

#include <cstdint>

#ifndef A2UI_COMMIT_SHORT_ID
#define A2UI_COMMIT_SHORT_ID "unknown"
#endif

namespace A2UILog {

constexpr uint32_t A2UI_DOMAIN = 0xFF00;
constexpr const char* A2UI_TAG = "A2UI@" A2UI_COMMIT_SHORT_ID;

} // namespace A2UILog

#define LOG_A2UI_PRINT(level, fmt, ...) \
    OH_LOG_Print(LOG_APP, level, A2UILog::A2UI_DOMAIN, A2UILog::A2UI_TAG, fmt, ##__VA_ARGS__)

#if defined(NDEBUG)
#define LOG_A2UI_LOG_DEBUG(fmt, ...) \
    do {                             \
    } while (0)
#define LOG_A2UI_LOG_INFO(fmt, ...) LOG_A2UI_PRINT(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_A2UI_LOG_WARN(fmt, ...) LOG_A2UI_PRINT(LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_A2UI_LOG_ERROR(fmt, ...) LOG_A2UI_PRINT(LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_A2UI_LOG_FATAL(fmt, ...) LOG_A2UI_PRINT(LOG_FATAL, fmt, ##__VA_ARGS__)
#define LOG_A2UI_DISPATCH(level, fmt, ...) LOG_A2UI_##level(fmt, ##__VA_ARGS__)
#define LOG_A2UI(level, fmt, ...) LOG_A2UI_DISPATCH(level, fmt, ##__VA_ARGS__)
#else
#define LOG_A2UI(level, fmt, ...) LOG_A2UI_PRINT(level, fmt, ##__VA_ARGS__)
#endif

#endif // A2UI_LOG_A2UI_H
