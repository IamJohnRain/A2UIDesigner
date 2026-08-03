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

#include "DynamicValueResolver.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>

#include "../RenderManager.h"
#include "../RenderSlot.h"
#include "../SurfaceErrorCodes.h"
#include "../SurfaceManager.h"
#include "../SurfaceSlot.h"
#include "../functions/FunctionBridge.h"
#include "../functions/FunctionCallInfo.h"
#include "../functions/NativeFunctionRegistry.h"
#include "../functions/RuntimeErrorDispatchBridge.h"
#ifdef ENABLE_EXPRESSION_ENGINE
#include "../expression/EvaluationContext.h"
#include "../expression/ExpressionEngine.h"
#endif
#include "utils/LogA2UI.h"

#include "BindingEngine.h"
#include "DataModel.h"
#include "PathValidator.h"

namespace NativeModule {

namespace {

constexpr int32_t MAX_DYNAMIC_RESOLVE_DEPTH = 16;

std::shared_ptr<DataModel> GetDataModel(const DynamicResolveContext& context);

void DispatchDynamicResolveError(const DynamicResolveContext& context, const std::string& message)
{
    if (context.renderId < 0) {
        return;
    }
    RuntimeErrorDispatchBridge::GetInstance().Dispatch(context.renderId, context.surfaceId, context.componentId,
        SURFACE_ERROR_DYNAMIC_VALUE_RESOLVE_FAILED, message, "DynamicValueResolver");
}

#ifdef ENABLE_EXPRESSION_ENGINE
bool HasExpressionContextError(const EvaluationContext& evalContext)
{
    return evalContext.lastError != ExpressionError::NONE && !evalContext.errorMessage.empty();
}

bool IsIllegalExpressionContextError(ExpressionError error)
{
    return error == ExpressionError::PARSE_UNEXPECTED_TOKEN;
}

bool IsSoftExpressionContextError(ExpressionError error)
{
    return error == ExpressionError::EVAL_PATH_NOT_FOUND || error == ExpressionError::EVAL_NO_GLOBAL_VARIABLE ||
           error == ExpressionError::EVAL_UNDEFINED_VARIABLE || IsIllegalExpressionContextError(error);
}

int32_t ResolveExpressionRuntimeErrorCode(ExpressionError error)
{
    if (error == ExpressionError::EVAL_NO_GLOBAL_VARIABLE) {
        return SURFACE_ERROR_GLOBAL_VARIABLE_NOT_FOUND;
    }
    if (IsIllegalExpressionContextError(error)) {
        return SURFACE_ERROR_ILLEGAL_EXPRESSION;
    }
    return SURFACE_ERROR_DYNAMIC_VALUE_RESOLVE_FAILED;
}

std::string ResolveExpressionRuntimeErrorMessage(const EvaluationContext& evalContext)
{
    if (IsIllegalExpressionContextError(evalContext.lastError) &&
        evalContext.errorMessage.rfind("illegal expression", 0) != 0) {
        return "illegal expression: " + evalContext.errorMessage;
    }
    return evalContext.errorMessage;
}

void DispatchExpressionResolveError(const DynamicResolveContext& context, const EvaluationContext& evalContext)
{
    if (context.renderId < 0 || !HasExpressionContextError(evalContext)) {
        return;
    }
    RuntimeErrorDispatchBridge::GetInstance().Dispatch(context.renderId, context.surfaceId, context.componentId,
        ResolveExpressionRuntimeErrorCode(evalContext.lastError), ResolveExpressionRuntimeErrorMessage(evalContext),
        "DynamicValueResolver");
}

void ApplyLocalVariables(EvaluationContext& evalContext, const DynamicResolveContext& context)
{
    if (context.localVariables.empty()) {
        return;
    }

    evalContext.PushScope();
    for (const auto& [name, value] : context.localVariables) {
        if (name.empty() || !value.IsValid()) {
            continue;
        }
        evalContext.SetLocalVariable(name, EvalResult::FromJson(value));
    }
}
#endif

bool CloneJsonValue(const JsonValue& input, JsonValue& output)
{
    if (!input.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsValid();
}

void AppendUniqueString(std::vector<std::string>& values, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) != values.end()) {
        return;
    }
    values.push_back(value);
}

using ArgPath = std::vector<std::string>;

constexpr const char ARG_ARRAY_ITEM_SEGMENT[] = "[]";

bool IsFunctionWithExpressionArgPolicy(const std::string& functionName)
{
    return functionName == "required" || functionName == "regex" || functionName == "length" ||
           functionName == "numeric" || functionName == "email" || functionName == "formatString" ||
           functionName == "formatNumber" || functionName == "formatCurrency" || functionName == "formatDate" ||
           functionName == "pluralize" || functionName == "and" || functionName == "or" || functionName == "not" ||
           functionName == "getCheckboxGroupValues" || functionName == "getRadioValue" ||
           functionName == "getSelectValue" || functionName == "getToggleValue" || functionName == "navigate" ||
           functionName == "setDataModel" || functionName == "setAttributes";
}

bool IsRootExpressionArgAllowed(const std::string& functionName, const std::string& argName)
{
    if (functionName == "required" || functionName == "regex" || functionName == "length" ||
        functionName == "numeric" || functionName == "email" || functionName == "formatString" ||
        functionName == "not") {
        return argName == "value";
    }
    if (functionName == "formatNumber") {
        return argName == "value" || argName == "decimals" || argName == "grouping";
    }
    if (functionName == "formatCurrency") {
        return argName == "value" || argName == "currency" || argName == "decimals" || argName == "grouping";
    }
    if (functionName == "formatDate") {
        return argName == "value" || argName == "format";
    }
    if (functionName == "pluralize") {
        return argName == "value" || argName == "zero" || argName == "one" || argName == "two" || argName == "few" ||
               argName == "many" || argName == "other";
    }
    if (functionName == "getCheckboxGroupValues" || functionName == "getRadioValue") {
        return argName == "group";
    }
    if (functionName == "getSelectValue" || functionName == "getToggleValue") {
        return argName == "componentId";
    }
    if (functionName == "navigate") {
        return argName == "componentId" || argName == "targetComponentId";
    }
    if (functionName == "setDataModel") {
        return argName == "value";
    }
    if (functionName == "setAttributes") {
        return argName == "componentId" || argName == "value";
    }
    return false;
}

bool IsExpressionAllowedArgPath(const std::string& functionName, const ArgPath& path)
{
    if (path.empty()) {
        return false;
    }
    if (functionName == "and" || functionName == "or") {
        return path.size() >= 2u && path[0] == "values" && path[1] == ARG_ARRAY_ITEM_SEGMENT;
    }
    return IsRootExpressionArgAllowed(functionName, path[0]);
}

bool MayContainExpressionAllowedArgPath(const std::string& functionName, const ArgPath& path)
{
    if (path.empty()) {
        return IsFunctionWithExpressionArgPolicy(functionName);
    }
    if (functionName == "and" || functionName == "or") {
        return (path.size() == 1u && path[0] == "values") || IsExpressionAllowedArgPath(functionName, path);
    }
    return IsExpressionAllowedArgPath(functionName, path);
}

ArgPath AppendObjectArgPath(const ArgPath& path, const std::string& key)
{
    ArgPath nextPath = path;
    nextPath.push_back(key);
    return nextPath;
}

ArgPath AppendArrayItemArgPath(const ArgPath& path)
{
    ArgPath nextPath = path;
    nextPath.push_back(ARG_ARRAY_ITEM_SEGMENT);
    return nextPath;
}

bool CreateStringValue(const std::string& input, JsonValue& output)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateString(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsString();
}

bool HasOnlyPathDescriptorKey(const JsonValue& value)
{
    if (!value.IsObject() || !value.Has("path")) {
        return false;
    }

    for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
        if (child.GetKey() != "path") {
            return false;
        }
    }
    return true;
}

std::string JsonValueToTemplateOutput(const JsonValue& value)
{
    if (!value.IsValid() || value.IsNull()) {
        return "";
    }
    if (value.IsString()) {
        return value.GetStringValue("");
    }
    if (value.IsNumber()) {
        std::ostringstream numberStream;
        numberStream << value.GetNumberValue(0.0);
        return numberStream.str();
    }
    if (value.IsBool()) {
        return value.GetBoolValue(false) ? "true" : "false";
    }
    return value.ToJsonLiteral();
}

std::optional<ResolvedValue> ResolveJsonPointerTemplateValue(
    const JsonValue& value, const DynamicResolveContext& context)
{
    if (!value.IsString()) {
        return std::nullopt;
    }

    const std::string raw = value.GetStringValue("");
    if (raw.find("${") == std::string::npos && raw.find("\\${") == std::string::npos) {
        return std::nullopt;
    }

    std::shared_ptr<DataModel> dataModel;
    std::string resolved;
    resolved.reserve(raw.size());
    bool touched = false;
    size_t index = 0;
    while (index < raw.size()) {
        if (index + 2 < raw.size() && raw[index] == '\\' && raw[index + 1] == '$' && raw[index + 2] == '{') {
            resolved.append("${");
            index += 3;
            touched = true;
            continue;
        }

        if (index + 1 < raw.size() && raw[index] == '$' && raw[index + 1] == '{') {
            size_t closePos = raw.find('}', index + 2);
            if (closePos == std::string::npos) {
                resolved.push_back(raw[index]);
                ++index;
                continue;
            }

            std::string expr = raw.substr(index + 2, closePos - index - 2);
            if (!expr.empty() && expr[0] == '/' && IsValidDataPath(expr)) {
                if (dataModel == nullptr) {
                    dataModel = GetDataModel(context);
                }
                if (dataModel != nullptr) {
                    std::optional<JsonValue> nodeOpt = dataModel->GetNode(expr);
                    if (nodeOpt.has_value()) {
                        resolved.append(JsonValueToTemplateOutput(nodeOpt.value()));
                    } else {
                        DispatchDynamicResolveError(context, "path not found: " + expr);
                    }
                }
                touched = true;
                index = closePos + 1;
                continue;
            }
        }

        resolved.push_back(raw[index]);
        ++index;
    }

    if (!touched) {
        return std::nullopt;
    }

    JsonValue resolvedValue;
    if (!CreateStringValue(resolved, resolvedValue)) {
        return ResolvedValue::FailInvalid("template string conversion failed");
    }
    return ResolvedValue::OkLiteral(resolvedValue);
}

#ifdef ENABLE_EXPRESSION_ENGINE
std::string TrimExpressionString(const std::string& value)
{
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(begin, end - begin);
}

bool IsExpressionCandidate(const std::string& value)
{
    return ExpressionEngine::IsExpression(TrimExpressionString(value));
}

std::string ExtractExpressionCandidate(const std::string& value)
{
    return ExpressionEngine::ExtractExpression(TrimExpressionString(value));
}

ResolvedValue ResolveExpressionValue(const JsonValue& value, const DynamicResolveContext& context)
{
    if (!value.IsString()) {
        return ResolvedValue::FailExpression("expression value must be a string");
    }

    std::string expression = TrimExpressionString(value.GetStringValue(""));
    if (!IsExpressionCandidate(expression)) {
        JsonValue literalValue;
        if (!CloneJsonValue(value, literalValue)) {
            return ResolvedValue::FailInvalid("literal clone failed");
        }
        return ResolvedValue::OkLiteral(literalValue);
    }

    EvaluationContext evalContext;
    evalContext.SetRenderId(context.renderId);
    evalContext.SetSurfaceId(context.surfaceId);
    evalContext.SetComponentId(context.componentId);
    ApplyLocalVariables(evalContext, context);

    if (context.dataModel != nullptr) {
        evalContext.SetDataModel(context.dataModel.get());
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(context.renderId);
    if (renderSlot != nullptr) {
        std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
        if (surfaceManager != nullptr) {
            evalContext.SetThemeContext(&surfaceManager->GetThemeContext());

            if (!context.surfaceId.empty() && context.dataModel == nullptr) {
                SurfaceSlot* surfaceSlot = surfaceManager->FindSurface(context.surfaceId);
                if (surfaceSlot != nullptr && surfaceSlot->GetBindingEngine() != nullptr) {
                    auto dataModel = surfaceSlot->GetBindingEngine()->GetOrCreateDataModel(context.surfaceId);
                    evalContext.SetDataModel(dataModel.get());
                }
            }
        }
    }

    JsonValue resolvedJson = ExpressionEngine::GetInstance().EvaluateAsJsonValue(expression, evalContext);
    bool hasContextError = HasExpressionContextError(evalContext);
    if (hasContextError && (IsSoftExpressionContextError(evalContext.lastError) || !resolvedJson.IsValid())) {
        DispatchExpressionResolveError(context, evalContext);
    }
    if (!resolvedJson.IsValid()) {
        std::string errorMessage =
            hasContextError ? ResolveExpressionRuntimeErrorMessage(evalContext) : "expression evaluation failed";
        if (!hasContextError) {
            DispatchDynamicResolveError(context, "expression evaluation failed");
        }
        return ResolvedValue::FailExpression(errorMessage);
    }

    ResolvedValue resolved = ResolvedValue::OkExpression(resolvedJson);
    if (hasContextError) {
        resolved.errorMessage = ResolveExpressionRuntimeErrorMessage(evalContext);
    }
    return resolved;
}
#endif

enum class RecursiveResolveMode { STRICT = 0, ALLOW_PARTIAL };

bool ShouldKeepPartialResolveFailure(RecursiveResolveMode mode)
{
    return mode == RecursiveResolveMode::ALLOW_PARTIAL;
}

bool ResolveJsonValueRecursivelyWithMode(const JsonValue& value, const DynamicResolveContext& context, int32_t depth,
    JsonValue& resolvedValue, RecursiveResolveMode mode)
{
    if (depth > MAX_DYNAMIC_RESOLVE_DEPTH) {
        LOG_A2UI(LOG_WARN, "ResolveJsonValueRecursively: max depth exceeded");
        return false;
    }
    if (!value.IsValid()) {
        std::unique_ptr<JsonAdapter> nullValue = JsonAdapter::CreateNull();
        if (nullValue == nullptr) {
            return false;
        }
        resolvedValue = nullValue->GetRoot();
        return false;
    }

    if (value.IsObject()) {
        bool hasCall = value.Has("call");
        bool hasPathBinding = HasOnlyPathDescriptorKey(value);
        if (hasCall || hasPathBinding) {
            ResolvedValue dynamicResolved = DynamicValueResolver::Resolve(value, context);
            if (!dynamicResolved.success || !dynamicResolved.value.IsValid()) {
                return false;
            }
            JsonValue clonedValue;
            if (!CloneJsonValue(dynamicResolved.value, clonedValue)) {
                return false;
            }
            resolvedValue = clonedValue;
            return true;
        }

        std::unique_ptr<JsonAdapter> objectAdapter = JsonAdapter::CreateObject();
        if (objectAdapter == nullptr) {
            return false;
        }
        JsonValue objectValue = objectAdapter->GetRoot();
        bool hadMember = false;
        bool keptMember = false;
        bool skippedMember = false;
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            std::string key = child.GetKey();
            if (key.empty()) {
                continue;
            }
            hadMember = true;
            JsonValue childValue;
            if (!ResolveJsonValueRecursivelyWithMode(child, context, depth + 1, childValue, mode)) {
                if (!ShouldKeepPartialResolveFailure(mode)) {
                    return false;
                }
                skippedMember = true;
                continue;
            }
            if (!objectValue.Put(key.c_str(), childValue)) {
                return false;
            }
            keptMember = true;
        }
        resolvedValue = objectValue;
        if (ShouldKeepPartialResolveFailure(mode) && hadMember && skippedMember && !keptMember) {
            return false;
        }
        return true;
    }

    if (value.IsArray()) {
        std::unique_ptr<JsonAdapter> arrayAdapter = JsonAdapter::CreateArray();
        if (arrayAdapter == nullptr) {
            return false;
        }
        JsonValue arrayRoot = arrayAdapter->GetRoot();

        for (int index = 0; index < value.GetArraySize(); ++index) {
            JsonValue itemValue;
            if (!ResolveJsonValueRecursivelyWithMode(value.GetArrayItem(index), context, depth + 1, itemValue, mode)) {
                return false;
            }
            if (!arrayRoot.Append(itemValue)) {
                return false;
            }
        }
        resolvedValue = arrayRoot;
        return true;
    }

#ifdef ENABLE_EXPRESSION_ENGINE
    if (context.allowExpression && value.IsString()) {
        ResolvedValue expressionResolved = ResolveExpressionValue(value, context);
        if (expressionResolved.source == ResolveSource::EXPRESSION) {
            if (!expressionResolved.success || !expressionResolved.value.IsValid()) {
                return false;
            }
            JsonValue clonedValue;
            if (!CloneJsonValue(expressionResolved.value, clonedValue)) {
                return false;
            }
            resolvedValue = clonedValue;
            return true;
        }
    }
#endif

    if (context.allowExpression && value.IsString()) {
        std::optional<ResolvedValue> templateResolved = ResolveJsonPointerTemplateValue(value, context);
        if (templateResolved.has_value()) {
            if (!templateResolved->success || !templateResolved->value.IsValid()) {
                return false;
            }
            JsonValue clonedValue;
            if (!CloneJsonValue(templateResolved->value, clonedValue)) {
                return false;
            }
            resolvedValue = clonedValue;
            return true;
        }
    }

    JsonValue literalValue;
    if (!CloneJsonValue(value, literalValue)) {
        return false;
    }
    resolvedValue = literalValue;
    return true;
}

bool ResolveJsonValueRecursively(
    const JsonValue& value, const DynamicResolveContext& context, int32_t depth, JsonValue& resolvedValue)
{
    return ResolveJsonValueRecursivelyWithMode(value, context, depth, resolvedValue, RecursiveResolveMode::STRICT);
}

bool ResolveFunctionArgsWithPolicy(const JsonValue& value, const std::string& functionName,
    const DynamicResolveContext& context, const ArgPath& path, int32_t depth, JsonValue& resolvedValue)
{
    if (depth > MAX_DYNAMIC_RESOLVE_DEPTH) {
        LOG_A2UI(LOG_WARN, "ResolveFunctionArgsWithPolicy: max depth exceeded");
        return false;
    }
    if (!value.IsValid()) {
        std::unique_ptr<JsonAdapter> nullValue = JsonAdapter::CreateNull();
        if (nullValue == nullptr) {
            return false;
        }
        resolvedValue = nullValue->GetRoot();
        return false;
    }

    if (IsExpressionAllowedArgPath(functionName, path)) {
        DynamicResolveContext argsContext = context;
        argsContext.allowExpression = true;
        return ResolveJsonValueRecursively(value, argsContext, depth, resolvedValue);
    }

    if (!MayContainExpressionAllowedArgPath(functionName, path)) {
        return CloneJsonValue(value, resolvedValue);
    }

    if (value.IsObject()) {
        if (value.Has("call") || HasOnlyPathDescriptorKey(value)) {
            return CloneJsonValue(value, resolvedValue);
        }

        std::unique_ptr<JsonAdapter> objectAdapter = JsonAdapter::CreateObject();
        if (objectAdapter == nullptr) {
            return false;
        }
        JsonValue objectValue = objectAdapter->GetRoot();
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            std::string key = child.GetKey();
            if (key.empty()) {
                continue;
            }
            JsonValue childValue;
            if (!ResolveFunctionArgsWithPolicy(
                    child, functionName, context, AppendObjectArgPath(path, key), depth + 1, childValue)) {
                return false;
            }
            if (!objectValue.Put(key.c_str(), childValue)) {
                return false;
            }
        }
        resolvedValue = objectValue;
        return true;
    }

    if (value.IsArray()) {
        std::unique_ptr<JsonAdapter> arrayAdapter = JsonAdapter::CreateArray();
        if (arrayAdapter == nullptr) {
            return false;
        }
        JsonValue arrayRoot = arrayAdapter->GetRoot();
        ArgPath itemPath = AppendArrayItemArgPath(path);
        int itemCount = value.GetArraySize();
        for (int index = 0; index < itemCount; ++index) {
            JsonValue itemValue;
            if (!ResolveFunctionArgsWithPolicy(
                    value.GetArrayItem(index), functionName, context, itemPath, depth + 1, itemValue)) {
                return false;
            }
            if (!arrayRoot.Append(itemValue)) {
                return false;
            }
        }
        resolvedValue = arrayRoot;
        return true;
    }

    return CloneJsonValue(value, resolvedValue);
}

ResolvedValue ResolvePathValue(const JsonValue& value, const DynamicResolveContext& context)
{
    JsonValue pathValue = value.GetItem("path");
    if (!pathValue.IsString()) {
        return ResolvedValue::FailInvalid("path descriptor must contain string field 'path'");
    }

    std::string path = pathValue.GetStringValue("");
    if (!IsValidDataPath(path)) {
        return ResolvedValue::FailInvalid("path descriptor contains invalid path");
    }

    std::shared_ptr<DataModel> dataModel = GetDataModel(context);
    if (dataModel == nullptr) {
        return ResolvedValue::FailPath(path, "data model unavailable");
    }

    std::optional<JsonValue> valueOpt = dataModel->GetNode(path);
    if (!valueOpt.has_value()) {
        DispatchDynamicResolveError(context, "path not found: " + path);
        return ResolvedValue::FailPath(path, "path not found");
    }

    JsonValue resolvedJson;
    if (!CloneJsonValue(valueOpt.value(), resolvedJson)) {
        return ResolvedValue::FailPath(path, "path value clone failed");
    }
    if (!resolvedJson.IsValid()) {
        return ResolvedValue::FailPath(path, "path value conversion failed");
    }
    return ResolvedValue::OkPath(resolvedJson, path);
}

ResolvedValue ResolveFunctionCallValue(const JsonValue& value, const DynamicResolveContext& context)
{
    JsonValue callValue = value.GetItem("call");
    if (!callValue.IsString()) {
        return ResolvedValue::FailInvalid("functionCall descriptor must contain string field 'call'");
    }

    std::string functionName = callValue.GetStringValue("");
    if (functionName.empty()) {
        return ResolvedValue::FailFunctionCall(functionName, "function name is empty");
    }

    JsonValue resolvedArgs;
    JsonValue argsValue = value.GetItem("args");
    if (argsValue.IsValid()) {
        if (!ResolveFunctionArgsWithPolicy(argsValue, functionName, context, ArgPath(), 0, resolvedArgs)) {
            return ResolvedValue::FailFunctionCall(functionName, "failed to resolve function args");
        }
    }
    std::string returnType = value.GetString("returnType", "void");

    if (NativeFunctionRegistry::GetInstance().HasFunction(functionName)) {
        std::shared_ptr<FunctionCallInfo> normalizedCall =
            std::make_shared<FunctionCallInfo>(functionName, resolvedArgs, returnType);
        JsonValue normalizedArgs;
        std::string normalizedReturnType = returnType;
        bool normalized = FunctionBridge::GetInstance().NormalizeFunctionCall(context.renderId, context.surfaceId,
            context.componentId, normalizedCall, normalizedArgs, normalizedReturnType);
        if (!normalized) {
            return ResolvedValue::FailFunctionCall(functionName, "builtin function schema normalize failed");
        }
        ResolvedValue nativeResult = NativeFunctionRegistry::GetInstance().Execute(
            functionName, normalizedArgs, context, normalizedReturnType == "void" ? "" : normalizedReturnType);
        if (!nativeResult.success) {
            return nativeResult;
        }
        return ResolvedValue::OkFunctionCall(nativeResult.value, functionName);
    }

    std::shared_ptr<FunctionCallInfo> functionCall =
        std::make_shared<FunctionCallInfo>(functionName, resolvedArgs, returnType);
    JsonValue functionResult;
    bool invokeSuccess = FunctionBridge::GetInstance().InvokeForValue(
        context.renderId, context.surfaceId, context.componentId, functionCall, functionResult);
    if (!invokeSuccess) {
        return ResolvedValue::FailFunctionCall(functionName, "local function invoke failed");
    }

    if (!functionResult.IsValid()) {
        return ResolvedValue::FailFunctionCall(functionName, "local function return json conversion failed");
    }
    return ResolvedValue::OkFunctionCall(functionResult, functionName);
}

std::shared_ptr<DataModel> GetDataModel(const DynamicResolveContext& context)
{
    if (context.dataModel != nullptr) {
        return context.dataModel;
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(context.renderId);
    if (renderSlot == nullptr) {
        return nullptr;
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return nullptr;
    }

    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface(context.surfaceId);
    if (surfaceSlot == nullptr || surfaceSlot->GetBindingEngine() == nullptr) {
        return nullptr;
    }

    return surfaceSlot->GetBindingEngine()->GetOrCreateDataModel(context.surfaceId);
}

void ExtractDataPathsRecursive(const JsonValue& value, std::vector<std::string>& paths)
{
    if (!value.IsValid()) {
        return;
    }
    if (value.IsObject()) {
        if (HasOnlyPathDescriptorKey(value)) {
            JsonValue pathValue = value.GetItem("path");
            if (pathValue.IsString()) {
                std::string path = pathValue.GetStringValue("");
                if (!path.empty() && std::find(paths.begin(), paths.end(), path) == paths.end()) {
                    paths.push_back(path);
                }
            }
            return;
        }
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            ExtractDataPathsRecursive(child, paths);
        }
        return;
    }
    if (value.IsArray()) {
        int itemCount = value.GetArraySize();
        for (int index = 0; index < itemCount; ++index) {
            ExtractDataPathsRecursive(value.GetArrayItem(index), paths);
        }
        return;
    }
    if (value.IsString()) {
        std::string str = value.GetStringValue("");
        size_t pos = 0;
        while ((pos = str.find("${", pos)) != std::string::npos) {
            if (pos > 0 && str[pos - 1] == '\\') {
                pos += 2;
                continue;
            }
            size_t closePos = str.find('}', pos + 2);
            if (closePos == std::string::npos) {
                break;
            }
            std::string expr = str.substr(pos + 2, closePos - pos - 2);
            if (!expr.empty() && expr[0] == '/' && IsValidDataPath(expr) &&
                std::find(paths.begin(), paths.end(), expr) == paths.end()) {
                paths.push_back(expr);
            }
            pos = closePos + 1;
        }
    }
}

#ifdef ENABLE_EXPRESSION_ENGINE
void AppendExpressionDependency(DynamicValueDependencies& dependencies, const Dependency& dependency)
{
    if (dependency.variableName == "__dataModel") {
        if (IsValidDataPath(dependency.path)) {
            AppendUniqueString(dependencies.dataPaths, dependency.path);
        }
        return;
    }
    AppendUniqueString(dependencies.globalVariables, dependency.variableName);
}

void ExtractExpressionDataDependencies(const JsonValue& value, DynamicValueDependencies& dependencies)
{
    if (!value.IsString()) {
        return;
    }

    std::string expression = ExtractExpressionCandidate(value.GetStringValue(""));
    if (expression.empty()) {
        return;
    }

    auto parseResult = ExpressionEngine::GetInstance().Parse(expression);
    if (!parseResult.success || parseResult.ast == nullptr) {
        return;
    }

    DependencyCollector collector;
    std::vector<Dependency> expressionDependencies = collector.Collect(parseResult.ast);
    for (const auto& dependency : expressionDependencies) {
        AppendExpressionDependency(dependencies, dependency);
    }
}

void ExtractExpressionDependenciesFromDescriptor(const JsonValue& value, DynamicValueDependencies& dependencies);

void ExtractExpressionDependenciesFromArgs(const JsonValue& value, const std::string& functionName, const ArgPath& path,
    DynamicValueDependencies& dependencies, int32_t depth)
{
    if (depth > MAX_DYNAMIC_RESOLVE_DEPTH || !value.IsValid()) {
        return;
    }

    bool expressionAllowed = IsExpressionAllowedArgPath(functionName, path);
    if (expressionAllowed && value.IsString()) {
        ExtractExpressionDataDependencies(value, dependencies);
        return;
    }

    if (!expressionAllowed && !MayContainExpressionAllowedArgPath(functionName, path)) {
        return;
    }

    if (value.IsObject()) {
        if (value.Has("call")) {
            if (expressionAllowed) {
                ExtractExpressionDependenciesFromDescriptor(value, dependencies);
            }
            return;
        }
        if (HasOnlyPathDescriptorKey(value)) {
            return;
        }

        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            std::string key = child.GetKey();
            if (key.empty()) {
                continue;
            }
            ExtractExpressionDependenciesFromArgs(
                child, functionName, AppendObjectArgPath(path, key), dependencies, depth + 1);
        }
        return;
    }

    if (value.IsArray()) {
        ArgPath itemPath = AppendArrayItemArgPath(path);
        int itemCount = value.GetArraySize();
        for (int index = 0; index < itemCount; ++index) {
            ExtractExpressionDependenciesFromArgs(
                value.GetArrayItem(index), functionName, itemPath, dependencies, depth + 1);
        }
    }
}

void ExtractExpressionDependenciesFromDescriptor(const JsonValue& value, DynamicValueDependencies& dependencies)
{
    if (!value.IsObject()) {
        return;
    }

    JsonValue callValue = value.GetItem("call");
    if (!callValue.IsString()) {
        return;
    }

    std::string functionName = callValue.GetStringValue("");
    if (functionName.empty()) {
        return;
    }

    JsonValue argsValue = value.GetItem("args");
    if (argsValue.IsValid()) {
        ExtractExpressionDependenciesFromArgs(argsValue, functionName, ArgPath(), dependencies, 0);
    }
}

void ExtractExpressionDependenciesRecursive(
    const JsonValue& value, DynamicValueDependencies& dependencies, int32_t depth)
{
    if (depth > MAX_DYNAMIC_RESOLVE_DEPTH || !value.IsValid()) {
        return;
    }

    if (value.IsString()) {
        ExtractExpressionDataDependencies(value, dependencies);
        return;
    }

    if (value.IsObject()) {
        if (value.Has("call")) {
            ExtractExpressionDependenciesFromDescriptor(value, dependencies);
            return;
        }
        if (HasOnlyPathDescriptorKey(value)) {
            return;
        }
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            ExtractExpressionDependenciesRecursive(child, dependencies, depth + 1);
        }
        return;
    }

    if (value.IsArray()) {
        int itemCount = value.GetArraySize();
        for (int index = 0; index < itemCount; ++index) {
            ExtractExpressionDependenciesRecursive(value.GetArrayItem(index), dependencies, depth + 1);
        }
    }
}
#endif

void ExtractFunctionCallExpressionDependencies(const JsonValue& value, DynamicValueDependencies& dependencies)
{
    if (!value.IsValid()) {
        return;
    }
#ifdef ENABLE_EXPRESSION_ENGINE
    ExtractExpressionDependenciesRecursive(value, dependencies, 0);
#endif
}

} // namespace

std::shared_ptr<FunctionCallInfo> DynamicValueResolver::ResolveFunctionCallDescriptor(
    const JsonValue& value, const DynamicResolveContext& context)
{
    if (!value.IsObject()) {
        return nullptr;
    }

    JsonValue callValue = value.GetItem("call");
    if (!callValue.IsString()) {
        return nullptr;
    }

    std::string functionName = callValue.GetStringValue("");
    if (functionName.empty()) {
        return nullptr;
    }

    JsonValue resolvedArgs;
    JsonValue argsValue = value.GetItem("args");
    if (argsValue.IsValid()) {
        if (!ResolveFunctionArgsWithPolicy(argsValue, functionName, context, ArgPath(), 0, resolvedArgs)) {
            LOG_A2UI(LOG_WARN, "ResolveFunctionCallDescriptor: args resolve failed, function=%{public}s",
                functionName.c_str());
            return nullptr;
        }
    }

    std::string returnType = value.GetString("returnType", "void");
    return std::make_shared<FunctionCallInfo>(functionName, resolvedArgs, returnType);
}

ResolvedValue DynamicValueResolver::Resolve(const JsonValue& value, const DynamicResolveContext& context)
{
    if (!value.IsValid()) {
        return ResolvedValue::FailInvalid("value is invalid");
    }

    // literal value (string, number, bool, null)
    if (!value.IsObject()) {
#ifdef ENABLE_EXPRESSION_ENGINE
        if (context.allowExpression && value.IsString()) {
            ResolvedValue expressionResolved = ResolveExpressionValue(value, context);
            if (expressionResolved.source == ResolveSource::EXPRESSION) {
                return expressionResolved;
            }
        }
#endif
        if (context.allowExpression && value.IsString()) {
            std::optional<ResolvedValue> templateResolved = ResolveJsonPointerTemplateValue(value, context);
            if (templateResolved.has_value()) {
                return templateResolved.value();
            }
        }
        JsonValue literalValue;
        if (!CloneJsonValue(value, literalValue)) {
            return ResolvedValue::FailInvalid("literal clone failed");
        }
        return ResolvedValue::OkLiteral(literalValue);
    }

    // dynamic value (object with "path" or "call")
    bool hasCall = value.Has("call");
    bool hasPath = value.Has("path");
    if (hasCall) {
        return ResolveFunctionCallValue(value, context);
    }
    if (hasPath) {
        return ResolvePathValue(value, context);
    }

    // object literal
    JsonValue literalValue;
    if (!CloneJsonValue(value, literalValue)) {
        return ResolvedValue::FailInvalid("object literal clone failed");
    }
    return ResolvedValue::OkLiteral(literalValue);
}

ResolvedValue DynamicValueResolver::ResolveRecursively(const JsonValue& value, const DynamicResolveContext& context)
{
    if (!value.IsValid()) {
        return ResolvedValue::FailInvalid("value is invalid");
    }

    JsonValue resolvedValue;
    if (!ResolveJsonValueRecursively(value, context, 0, resolvedValue) || !resolvedValue.IsValid()) {
        return ResolvedValue::FailInvalid("recursive value resolve failed");
    }
    return ResolvedValue::OkLiteral(resolvedValue);
}

ResolvedValue DynamicValueResolver::ResolveRecursivelyAllowPartial(
    const JsonValue& value, const DynamicResolveContext& context)
{
    if (!value.IsValid()) {
        return ResolvedValue::FailInvalid("value is invalid");
    }

    JsonValue resolvedValue;
    if (!ResolveJsonValueRecursivelyWithMode(value, context, 0, resolvedValue, RecursiveResolveMode::ALLOW_PARTIAL) ||
        !resolvedValue.IsValid()) {
        return ResolvedValue::FailInvalid("recursive value resolve failed");
    }
    return ResolvedValue::OkLiteral(resolvedValue);
}

std::vector<std::string> DynamicValueResolver::ExtractDataPaths(const JsonValue& value)
{
    return ExtractDependencies(value).dataPaths;
}

DynamicValueDependencies DynamicValueResolver::ExtractDependencies(const JsonValue& value)
{
    DynamicValueDependencies dependencies;
    if (!value.IsValid()) {
        return dependencies;
    }
    ExtractDataPathsRecursive(value, dependencies.dataPaths);
    ExtractFunctionCallExpressionDependencies(value, dependencies);
    return dependencies;
}

} // namespace NativeModule
