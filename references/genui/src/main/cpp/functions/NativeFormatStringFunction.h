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

#ifndef A2UI_NATIVE_FORMAT_STRING_FUNCTION_H
#define A2UI_NATIVE_FORMAT_STRING_FUNCTION_H

#include <string>

#include "NativeFunctionBase.h"

namespace NativeModule {

class NativeFormatStringFunction : public NativeFunctionBase {
public:
    std::string GetName() const override;
    FunctionResult Execute(const JsonValue& resolvedArgs) override;
    FunctionResult ExecuteWithContext(const JsonValue& resolvedArgs, const DynamicResolveContext& context) override;

private:
    std::string ResolveTemplate(const std::string& templateStr, const DynamicResolveContext& context);
    int FindMatchingBrace(const std::string& s, int start);
    bool ParseFunctionCallArgs(
        const std::string& argsPart, JsonValue& argsObject, const DynamicResolveContext& context);
    bool ParseSingleArgValue(const std::string& argsPart, size_t valStart, const DynamicResolveContext& context,
        JsonValue& argValue, size_t& nextPos);
    std::string ResolveFunctionCall(
        const std::string& funcName, const std::string& argsPart, const DynamicResolveContext& context);
    std::string ResolveDataPathExpression(const std::string& expr, const DynamicResolveContext& context);
};

} // namespace NativeModule

#endif // A2UI_NATIVE_FORMAT_STRING_FUNCTION_H
