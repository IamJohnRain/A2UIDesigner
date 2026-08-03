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

#include "NativeRequiredFunction.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

std::string NativeRequiredFunction::GetName() const
{
    return "required";
}

FunctionResult NativeRequiredFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        LOG_A2UI(LOG_WARN, "NativeRequiredFunction::Execute: args is empty");
        return FunctionResult(false);
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsValid()) {
        return FunctionResult(false);
    }

    bool present = IsPresent(valueArg);
    return FunctionResult(present);
}

bool NativeRequiredFunction::IsPresent(const JsonValue& value)
{
    if (!value.IsValid() || value.IsNull()) {
        return false;
    }
    if (value.IsString()) {
        return !value.GetStringValue("").empty();
    }
    if (value.IsArray()) {
        return value.GetArraySize() > 0;
    }
    if (value.IsObject()) {
        return value.GetChild().IsValid();
    }
    return true;
}

} // namespace NativeModule
