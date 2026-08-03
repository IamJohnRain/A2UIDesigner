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

#include "NativeFunctionRegistry.h"

#include "utils/LogA2UI.h"

#include "NativeAndFunction.h"
#include "NativeEmailFunction.h"
#include "NativeFormatCurrencyFunction.h"
#include "NativeFormatDateFunction.h"
#include "NativeFormatNumberFunction.h"
#include "NativeFormatStringFunction.h"
#include "NativeGetCheckboxGroupValuesFunction.h"
#include "NativeGetRadioValueFunction.h"
#include "NativeGetSelectValueFunction.h"
#include "NativeGetToggleValueFunction.h"
#include "NativeLengthFunction.h"
#include "NativeNotFunction.h"
#include "NativeNumericFunction.h"
#include "NativeOrFunction.h"
#include "NativePluralizeFunction.h"
#include "NativeRegexFunction.h"
#include "NativeRequiredFunction.h"
#include "extended/NativeNavigateFunction.h"

namespace NativeModule {

namespace {

std::shared_ptr<JsonAdapter> CreateOwnedJsonAdapter(const FunctionResult& result)
{
    switch (result.GetType()) {
        case FunctionResultType::NULL_VALUE:
        case FunctionResultType::BOOL:
        case FunctionResultType::INT:
        case FunctionResultType::DOUBLE:
        case FunctionResultType::STRING:
        case FunctionResultType::JSON_VALUE:
            break;
        default:
            return nullptr;
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(result.ToJsonLiteral());
    if (adapter == nullptr) {
        return nullptr;
    }
    return std::shared_ptr<JsonAdapter>(std::move(adapter));
}

} // namespace

NativeFunctionRegistry::NativeFunctionRegistry()
{
    Register("required", std::make_shared<NativeRequiredFunction>());
    Register("regex", std::make_shared<NativeRegexFunction>());
    Register("length", std::make_shared<NativeLengthFunction>());
    Register("numeric", std::make_shared<NativeNumericFunction>());
    Register("email", std::make_shared<NativeEmailFunction>());
    Register("formatString", std::make_shared<NativeFormatStringFunction>());
    Register("formatNumber", std::make_shared<NativeFormatNumberFunction>());
    Register("formatCurrency", std::make_shared<NativeFormatCurrencyFunction>());
    Register("formatDate", std::make_shared<NativeFormatDateFunction>());
    Register("pluralize", std::make_shared<NativePluralizeFunction>());
    Register("and", std::make_shared<NativeAndFunction>());
    Register("or", std::make_shared<NativeOrFunction>());
    Register("not", std::make_shared<NativeNotFunction>());
    Register("getToggleValue", std::make_shared<NativeGetToggleValueFunction>());
    Register("getRadioValue", std::make_shared<NativeGetRadioValueFunction>());
    Register("getSelectValue", std::make_shared<NativeGetSelectValueFunction>());
    Register("getCheckboxGroupValues", std::make_shared<NativeGetCheckboxGroupValuesFunction>());
    Register("navigate", std::make_shared<NativeNavigateFunction>());
}

NativeFunctionRegistry& NativeFunctionRegistry::GetInstance()
{
    static NativeFunctionRegistry instance;
    return instance;
}

void NativeFunctionRegistry::Register(const std::string& name, std::shared_ptr<NativeFunctionBase> handler)
{
    if (name.empty() || handler == nullptr) {
        LOG_A2UI(LOG_ERROR, "NativeFunctionRegistry::Register: invalid name or handler");
        return;
    }
    handlers_[name] = std::move(handler);
    LOG_A2UI(LOG_INFO, "NativeFunctionRegistry::Register: %{public}s", name.c_str());
}

bool NativeFunctionRegistry::HasFunction(const std::string& name) const
{
    return handlers_.find(name) != handlers_.end();
}

ResolvedValue NativeFunctionRegistry::Execute(const std::string& name, const JsonValue& resolvedArgs,
    const DynamicResolveContext& context, const std::string& expectedReturnType)
{
    auto it = handlers_.find(name);
    if (it == handlers_.end()) {
        LOG_A2UI(LOG_ERROR, "NativeFunctionRegistry::Execute: function not found, name=%{public}s", name.c_str());
        return ResolvedValue::FailFunctionCall(name, "function not found");
    }

    FunctionResult execResult;
    try {
        execResult = it->second->ExecuteWithContext(resolvedArgs, context);
    } catch (const std::exception& error) {
        LOG_A2UI(LOG_ERROR, "NativeFunctionRegistry::Execute: exception, name=%{public}s, error=%{public}s",
            name.c_str(), error.what());
        return ResolvedValue::FailFunctionCall(name, error.what());
    } catch (...) {
        LOG_A2UI(LOG_ERROR, "NativeFunctionRegistry::Execute: unknown exception, name=%{public}s", name.c_str());
        return ResolvedValue::FailFunctionCall(name, "unknown native function exception");
    }
    std::shared_ptr<JsonAdapter> resultOwner = CreateOwnedJsonAdapter(execResult);
    JsonValue resultJson = resultOwner != nullptr ? resultOwner->GetRoot() : JsonValue();
    if (!resultJson.IsValid()) {
        return ResolvedValue::FailFunctionCall(name, "function return json conversion failed");
    }

    if (!expectedReturnType.empty() && !it->second->ValidateReturnType(expectedReturnType, resultJson)) {
        LOG_A2UI(LOG_ERROR,
            "NativeFunctionRegistry::Execute: returnType mismatch, function=%{public}s, returnType=%{public}s",
            name.c_str(), expectedReturnType.c_str());
        return ResolvedValue::FailFunctionCall(name, "builtin returnType mismatch");
    }
    ResolvedValue resolvedValue = ResolvedValue::OkFunctionCall(resultJson, name);
    resolvedValue.owner = resultOwner;
    return resolvedValue;
}

} // namespace NativeModule
