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

#include "ResolvedValue.h"

namespace NativeModule {

ResolvedValue ResolvedValue::OkLiteral(const JsonValue& value)
{
    ResolvedValue result;
    result.source = ResolveSource::LITERAL;
    result.success = true;
    result.value = value;
    return result;
}

ResolvedValue ResolvedValue::OkExpression(const JsonValue& value)
{
    ResolvedValue result;
    result.source = ResolveSource::EXPRESSION;
    result.success = true;
    result.value = value;
    return result;
}

ResolvedValue ResolvedValue::OkPath(const JsonValue& value, const std::string& path)
{
    ResolvedValue result;
    result.source = ResolveSource::PATH;
    result.success = true;
    result.value = value;
    result.path = path;
    return result;
}

ResolvedValue ResolvedValue::OkFunctionCall(const JsonValue& value, const std::string& functionName)
{
    ResolvedValue result;
    result.source = ResolveSource::FUNCTION_CALL;
    result.success = true;
    result.value = value;
    result.functionName = functionName;
    return result;
}

ResolvedValue ResolvedValue::FailExpression(const std::string& errorMessage)
{
    ResolvedValue result;
    result.source = ResolveSource::EXPRESSION;
    result.success = false;
    result.errorMessage = errorMessage;
    return result;
}

ResolvedValue ResolvedValue::FailPath(const std::string& path, const std::string& errorMessage)
{
    ResolvedValue result;
    result.source = ResolveSource::PATH;
    result.success = false;
    result.path = path;
    result.errorMessage = errorMessage;
    return result;
}

ResolvedValue ResolvedValue::FailFunctionCall(const std::string& functionName, const std::string& errorMessage)
{
    ResolvedValue result;
    result.source = ResolveSource::FUNCTION_CALL;
    result.success = false;
    result.functionName = functionName;
    result.errorMessage = errorMessage;
    return result;
}

ResolvedValue ResolvedValue::FailInvalid(const std::string& errorMessage)
{
    ResolvedValue result;
    result.source = ResolveSource::INVALID;
    result.success = false;
    result.errorMessage = errorMessage;
    return result;
}

} // namespace NativeModule
