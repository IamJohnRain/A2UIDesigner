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

#ifndef A2UI_DATA_RESOLVED_VALUE_H
#define A2UI_DATA_RESOLVED_VALUE_H

#include <memory>
#include <string>

#include "../utils/JsonAdapter.h"

namespace NativeModule {

enum class ResolveSource { INVALID = 0, LITERAL, EXPRESSION, PATH, FUNCTION_CALL };

struct ResolvedValue {
    ResolveSource source = ResolveSource::INVALID;
    bool success = false;
    JsonValue value;
    std::shared_ptr<JsonAdapter> owner;
    std::string path;
    std::string functionName;
    std::string errorMessage;

    static ResolvedValue OkLiteral(const JsonValue& valueVal);
    static ResolvedValue OkExpression(const JsonValue& valueVal);
    static ResolvedValue OkPath(const JsonValue& valueVal, const std::string& pathVal);
    static ResolvedValue OkFunctionCall(const JsonValue& valueVal, const std::string& functionNameVal);
    static ResolvedValue FailExpression(const std::string& errorMessageVal);
    static ResolvedValue FailPath(const std::string& pathVal, const std::string& errorMessageVal);
    static ResolvedValue FailFunctionCall(const std::string& functionNameVal, const std::string& errorMessageVal);
    static ResolvedValue FailInvalid(const std::string& errorMessageVal);
};

} // namespace NativeModule

#endif // A2UI_DATA_RESOLVED_VALUE_H
