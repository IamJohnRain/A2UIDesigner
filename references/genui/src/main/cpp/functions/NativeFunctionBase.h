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

#ifndef A2UI_NATIVE_FUNCTION_BASE_H
#define A2UI_NATIVE_FUNCTION_BASE_H

#include <string>

#include "data/DynamicValueResolver.h"

#include "FunctionResult.h"

namespace NativeModule {

class NativeFunctionBase {
public:
    virtual ~NativeFunctionBase() = default;

    virtual std::string GetName() const = 0;
    virtual FunctionResult Execute(const JsonValue& resolvedArgs) = 0;
    virtual FunctionResult ExecuteWithContext(const JsonValue& resolvedArgs, const DynamicResolveContext& context);
    virtual bool ValidateReturnType(const std::string& returnType, const JsonValue& resultValue) const;
};

} // namespace NativeModule

#endif // A2UI_NATIVE_FUNCTION_BASE_H
