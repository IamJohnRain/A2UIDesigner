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

#ifndef HILOG_LOG_H
#define HILOG_LOG_H

#include <cstdio>
#include <iostream>

typedef enum { LOG_DEBUG = 3, LOG_INFO = 4, LOG_WARN = 5, LOG_ERROR = 6, LOG_FATAL = 7 } LogLevel;

typedef enum { LOG_APP = 0 } LogType;

static inline int OH_LOG_Print(LogType type, int level, unsigned int domain, const char* tag, const char* fmt, ...)
{
    (void)type;
    (void)level;
    (void)domain;
    (void)tag;
    (void)fmt;
    return 0;
}

#endif
