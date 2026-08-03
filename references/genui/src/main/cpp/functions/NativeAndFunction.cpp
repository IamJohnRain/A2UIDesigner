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

#include "NativeAndFunction.h"

#include <cstdint>

namespace NativeModule {

std::string NativeAndFunction::GetName() const
{
    return "and";
}

FunctionResult NativeAndFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(false);
    }

    JsonValue valuesArg = resolvedArgs.GetItem("values");
    if (!valuesArg.IsArray()) {
        return FunctionResult(false);
    }

    int32_t size = valuesArg.GetArraySize();
    if (size < 2) {
        return FunctionResult(false);
    }

    for (int32_t i = 0; i < size; ++i) {
        JsonValue item = valuesArg.GetArrayItem(i);
        if (!item.IsBool() || !item.GetBoolValue(false)) {
            return FunctionResult(false);
        }
    }
    return FunctionResult(true);
}

} // namespace NativeModule
