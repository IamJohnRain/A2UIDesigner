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

#include "utils/LocalVariableNameUtils.h"

#include <cctype>

namespace NativeModule {

namespace {

bool IsIdentifierStart(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool IsIdentifierChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

} // namespace

bool IsValidLocalVariableName(const std::string& name)
{
    if (name.empty() || !IsIdentifierStart(name[0])) {
        return false;
    }
    if (name.size() >= 2 && name[0] == '_' && name[1] == '_') {
        return false;
    }
    for (size_t index = 1; index < name.size(); ++index) {
        if (!IsIdentifierChar(name[index])) {
            return false;
        }
    }
    return true;
}

} // namespace NativeModule
