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

#include "NativeFunctionBase.h"

namespace NativeModule {

FunctionResult NativeFunctionBase::ExecuteWithContext(
    const JsonValue& resolvedArgs, const DynamicResolveContext& context)
{
    (void)context;
    return Execute(resolvedArgs);
}

bool NativeFunctionBase::ValidateReturnType(const std::string& returnType, const JsonValue& resultValue) const
{
    if (returnType.empty()) {
        return true;
    }
    if (returnType == "void") {
        return resultValue.IsNull();
    }
    if (returnType == "string") {
        return resultValue.IsString();
    }
    if (returnType == "number") {
        return resultValue.IsNumber();
    }
    if (returnType == "boolean") {
        return resultValue.IsBool();
    }
    if (returnType == "null") {
        return resultValue.IsNull();
    }
    if (returnType == "array") {
        return resultValue.IsArray();
    }
    if (returnType == "object") {
        return resultValue.IsObject();
    }
    return false;
}

} // namespace NativeModule
