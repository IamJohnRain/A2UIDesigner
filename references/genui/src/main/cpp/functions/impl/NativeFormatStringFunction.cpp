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

#include "NativeFormatStringFunction.h"

#include <cstdlib>
#include <sstream>

#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "../RenderManager.h"
#include "../RenderSlot.h"
#include "../SurfaceManager.h"
#include "../SurfaceSlot.h"
#include "NativeFunctionRegistry.h"

namespace NativeModule {

namespace {

bool TryParseNumber(const std::string& text, double& valueOut)
{
    if (text.empty()) {
        return false;
    }
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0') {
        return false;
    }
    valueOut = parsed;
    return true;
}

JsonValue ParseLooseJsonToken(const std::string& token)
{
    if (token.empty()) {
        std::unique_ptr<JsonAdapter> emptyString = JsonAdapter::CreateString("");
        return emptyString != nullptr ? emptyString->GetRoot() : JsonValue();
    }
    if (token == "true" || token == "false") {
        std::unique_ptr<JsonAdapter> boolValue = JsonAdapter::CreateBool(token == "true");
        return boolValue != nullptr ? boolValue->GetRoot() : JsonValue();
    }
    if (token == "null") {
        std::unique_ptr<JsonAdapter> nullValue = JsonAdapter::CreateNull();
        return nullValue != nullptr ? nullValue->GetRoot() : JsonValue();
    }
    double numberValue = 0.0;
    if (TryParseNumber(token, numberValue)) {
        std::unique_ptr<JsonAdapter> number = JsonAdapter::CreateNumber(numberValue);
        return number != nullptr ? number->GetRoot() : JsonValue();
    }
    std::unique_ptr<JsonAdapter> stringValue = JsonAdapter::CreateString(token);
    return stringValue != nullptr ? stringValue->GetRoot() : JsonValue();
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

} // namespace

static std::shared_ptr<DataModel> GetDataModel(const DynamicResolveContext& context)
{
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

std::string NativeFormatStringFunction::GetName() const
{
    return "formatString";
}

FunctionResult NativeFormatStringFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(std::string(""));
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsString()) {
        return FunctionResult(std::string(""));
    }

    return FunctionResult(valueArg.GetStringValue(""));
}

FunctionResult NativeFormatStringFunction::ExecuteWithContext(
    const JsonValue& resolvedArgs, const DynamicResolveContext& context)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(std::string(""));
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsString()) {
        return FunctionResult(std::string(""));
    }

    std::string templateStr = valueArg.GetStringValue("");
    std::string resolved = ResolveTemplate(templateStr, context);
    return FunctionResult(std::move(resolved));
}

int NativeFormatStringFunction::FindMatchingBrace(const std::string& s, int start)
{
    int depth = 0;
    for (int i = start; i < static_cast<int>(s.size()); ++i) {
        if (s[i] == '{') {
            ++depth;
        } else if (s[i] == '}') {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return -1;
}

std::string NativeFormatStringFunction::ResolveTemplate(
    const std::string& templateStr, const DynamicResolveContext& context)
{
    std::ostringstream result;
    size_t i = 0;

    while (i < templateStr.size()) {
        if (i + 1 < templateStr.size() && templateStr[i] == '\\' && templateStr[i + 1] == '$' &&
            i + 2 < templateStr.size() && templateStr[i + 2] == '{') {
            result << "${";
            i += 3;
            continue;
        }

        if (templateStr[i] != '$' || i + 1 >= templateStr.size() || templateStr[i + 1] != '{') {
            result << templateStr[i];
            ++i;
            continue;
        }

        int closePos = FindMatchingBrace(templateStr, static_cast<int>(i) + 1);
        if (closePos < 0) {
            result << templateStr[i];
            ++i;
            continue;
        }

        std::string expr = templateStr.substr(i + 2, closePos - i - 2);
        std::string resolved;
        size_t parenPos = expr.find('(');
        if (parenPos != std::string::npos && expr.back() == ')') {
            std::string funcName = expr.substr(0, parenPos);
            std::string argsPart = expr.substr(parenPos + 1, expr.size() - parenPos - 2);
            resolved = ResolveFunctionCall(funcName, argsPart, context);
        } else {
            resolved = ResolveDataPathExpression(expr, context);
        }

        result << resolved;
        i = static_cast<size_t>(closePos) + 1;
    }

    return result.str();
}

bool NativeFormatStringFunction::ParseSingleArgValue(const std::string& argsPart, size_t valStart,
    const DynamicResolveContext& context, JsonValue& argValue, size_t& nextPos)
{
    if (argsPart[valStart] == '\'') {
        size_t endQuote = argsPart.find('\'', valStart + 1);
        if (endQuote == std::string::npos) {
            return false;
        }
        std::unique_ptr<JsonAdapter> stringValue =
            JsonAdapter::CreateString(argsPart.substr(valStart + 1, endQuote - valStart - 1));
        if (stringValue != nullptr) {
            argValue = stringValue->GetRoot();
        }
        nextPos = endQuote + 1;
        return true;
    }
    if (argsPart[valStart] == '$' && valStart + 1 < argsPart.size() && argsPart[valStart + 1] == '{') {
        int innerClose = FindMatchingBrace(argsPart, static_cast<int>(valStart) + 1);
        if (innerClose < 0) {
            return false;
        }
        std::string innerExpr = argsPart.substr(valStart + 2, innerClose - valStart - 2);
        std::string innerWrapper = "${";
        innerWrapper += innerExpr;
        innerWrapper += "}";
        std::string innerResolved = ResolveTemplate(innerWrapper, context);
        argValue = ParseLooseJsonToken(innerResolved);
        if (!argValue.IsValid()) {
            return false;
        }
        nextPos = static_cast<size_t>(innerClose) + 1;
        return true;
    }
    size_t valEnd = argsPart.find(',', valStart);
    if (valEnd == std::string::npos) {
        valEnd = argsPart.size();
    }
    std::string rawToken = argsPart.substr(valStart, valEnd - valStart);
    while (!rawToken.empty() && rawToken.back() == ' ') {
        rawToken.pop_back();
    }
    argValue = ParseLooseJsonToken(rawToken);
    nextPos = valEnd;
    return true;
}

bool NativeFormatStringFunction::ParseFunctionCallArgs(
    const std::string& argsPart, JsonValue& argsObject, const DynamicResolveContext& context)
{
    size_t pos = 0;
    while (pos < argsPart.size()) {
        while (pos < argsPart.size() && argsPart[pos] == ' ') {
            ++pos;
        }
        if (pos >= argsPart.size()) {
            break;
        }

        size_t colonPos = argsPart.find(':', pos);
        if (colonPos == std::string::npos) {
            return false;
        }

        std::string argName = argsPart.substr(pos, colonPos - pos);
        while (!argName.empty() && argName.back() == ' ') {
            argName.pop_back();
        }
        if (argName.empty()) {
            return false;
        }

        size_t valStart = colonPos + 1;
        while (valStart < argsPart.size() && argsPart[valStart] == ' ') {
            ++valStart;
        }
        if (valStart >= argsPart.size()) {
            return false;
        }

        JsonValue argValue;
        size_t nextPos = pos;
        if (!ParseSingleArgValue(argsPart, valStart, context, argValue, nextPos)) {
            return false;
        }
        pos = nextPos;

        if (pos < argsPart.size() && argsPart[pos] == ',') {
            ++pos;
        }
        if (!argValue.IsValid()) {
            return false;
        }
        if (!argsObject.Put(argName.c_str(), argValue)) {
            return false;
        }
    }
    return true;
}

std::string NativeFormatStringFunction::ResolveFunctionCall(
    const std::string& funcName, const std::string& argsPart, const DynamicResolveContext& context)
{
    if (!NativeFunctionRegistry::GetInstance().HasFunction(funcName)) {
        return "";
    }

    std::unique_ptr<JsonAdapter> argsAdapter = JsonAdapter::CreateObject();
    JsonValue argsObject = argsAdapter != nullptr ? argsAdapter->GetRoot() : JsonValue();
    if (!ParseFunctionCallArgs(argsPart, argsObject, context) || !argsObject.IsValid()) {
        return "";
    }

    ResolvedValue funcResult = NativeFunctionRegistry::GetInstance().Execute(funcName, argsObject, context);
    if (!funcResult.success) {
        return "";
    }
    return JsonValueToTemplateOutput(funcResult.value);
}

std::string NativeFormatStringFunction::ResolveDataPathExpression(
    const std::string& expr, const DynamicResolveContext& context)
{
    std::shared_ptr<DataModel> dataModel = GetDataModel(context);
    if (dataModel == nullptr) {
        return "";
    }

    std::string path = expr;
    if (path.empty() || path[0] != '/') {
        path = "/" + path;
    }
    std::optional<JsonValue> nodeOpt = dataModel->GetNode(path);
    if (!nodeOpt.has_value()) {
        DynamicValueResolver::ReportMissingPath(context, path);
        return "";
    }
    return JsonValueToTemplateOutput(nodeOpt.value());
}

} // namespace NativeModule
