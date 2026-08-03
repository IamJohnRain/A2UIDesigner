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

#include "PathValidator.h"

namespace NativeModule {

namespace {

bool IsAllowedPathChar(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';
}

} // namespace

bool IsValidDataPath(const std::string& path)
{
    if (path.empty() || path[0] != '/') {
        return false;
    }

    if (path.size() == 1) {
        return true;
    }

    bool expectSegmentStart = true;
    bool hasSegmentChar = false;
    for (size_t index = 1; index < path.size(); ++index) {
        char ch = path[index];
        if (ch == '/') {
            if (expectSegmentStart || !hasSegmentChar) {
                return false;
            }
            expectSegmentStart = true;
            hasSegmentChar = false;
            continue;
        }

        if (!IsAllowedPathChar(ch)) {
            return false;
        }

        expectSegmentStart = false;
        hasSegmentChar = true;
    }

    return hasSegmentChar;
}

} // namespace NativeModule
