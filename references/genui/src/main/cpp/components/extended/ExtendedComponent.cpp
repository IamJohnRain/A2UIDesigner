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

#include "ExtendedComponent.h"

#include <cmath>
#include <memory>
#include <set>
#include <vector>

#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/ExpressionEngine.h"
#endif
#include <cctype>

#include "components/actions/EventHandlerChainExecutor.h"
#include "components/actions/NativeActionRegistry.h"
#include "composition/ChildListParser.h"
#include "data/DynamicValueResolver.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/EventContextResolver.h"
#include "functions/FunctionBridge.h"
#include "functions/NativeFunctionRegistry.h"
#include "functions/WarningDispatchBridge.h"
#include "styles/StyleApplyUtils.h"
#include "styles/StyleParser.h"
#include "styles/StyleResolver.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"
#include "ArkUIOHApiAdapter.h"
#include "ExtendedComponentStyleValidation.h"
#include "ExtendedDescriptorNormalizer.h"
#include "ExtendedStyleResolver.h"
#include "RenderManager.h"
#include "SchemaErrorCodes.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

bool SupportsExtendedChildren(const std::string& componentType)
{
    return componentType == "Column" || componentType == "Row" || componentType == "List" || componentType == "Stack" ||
           componentType == "Grid" || componentType == "NavContainer";
}

JsonValue CloneJsonValue(const JsonValue& value)
{
    if (!value.IsValid()) {
        return JsonValue();
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(value);
    if (adapter == nullptr) {
        return JsonValue();
    }
    return adapter->GetRoot();
}

JsonValue CreateEmptyObjectValue()
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

bool JsonObjectHasEntries(const JsonValue& value)
{
    if (!value.IsObject()) {
        return false;
    }

    for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
        if (!child.GetKey().empty()) {
            return true;
        }
    }
    return false;
}

std::string BuildExtendedSchemaWarningPath(const std::string& componentId, const std::string& propertyPath)
{
    if (componentId.empty()) {
        return propertyPath;
    }
    if (propertyPath.empty()) {
        return componentId;
    }
    return componentId + "." + propertyPath;
}

bool IsValidFontWeightNumber(double value)
{
    constexpr int32_t minFontWeight = 100;
    constexpr int32_t maxFontWeight = 900;
    constexpr int32_t fontWeightStep = 100;
    constexpr double epsilon = 0.0001;

    if (!std::isfinite(value)) {
        return false;
    }
    int32_t normalized = static_cast<int32_t>(std::lround(value));
    return std::fabs(value - static_cast<double>(normalized)) <= epsilon && normalized >= minFontWeight &&
           normalized <= maxFontWeight && normalized % fontWeightStep == 0;
}

bool IsValidFontWeightValue(const JsonValue& value, const std::set<std::string>& allowedStringValues)
{
    if (value.IsNumber()) {
        return IsValidFontWeightNumber(value.GetNumberValue(0.0));
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    for (char& ch : token) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (allowedStringValues.find(token) != allowedStringValues.end()) {
        return true;
    }

    float parsedNumber = 0.0F;
    return StyleApplyUtils::ParseNumber(value, parsedNumber) && IsValidFontWeightNumber(parsedNumber);
}

bool IsSupportedTextInputCancelButtonStyle(const JsonValue& value)
{
    if (!value.IsString()) {
        return false;
    }
    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    for (char& ch : token) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return token == "constant" || token == "invisible" || token == "input";
}

bool TryParseTextInputCancelButtonFontSize(const JsonValue& value, float& fontSize)
{
    StyleDimension dimension;
    if (!StyleApplyUtils::ParseDimension(value, dimension)) {
        return false;
    }
    if (dimension.unit != StyleDimensionUnit::VP && dimension.unit != StyleDimensionUnit::FP) {
        return false;
    }
    fontSize = dimension.value;
    return std::isfinite(fontSize);
}

} // namespace

ExtendedComponent::ExtendedComponent(ArkUI_NodeHandle nativeView, bool ownsNativeView, bool isCompositeType)
    : A2UIComponent(nativeView, ownsNativeView, isCompositeType)
{
    if (nativeView_ != nullptr) {
        ArkUIOHApiAdapter::SetCrossLanguageOption(nativeView_, true);
    }
}

bool ExtendedComponent::InitFromDescriptor(const JsonValue& descriptor, const RenderContext& context)
{
    LOG_A2UI(LOG_DEBUG,
        "ExtendedComponent::InitFromDescriptor - componentId=%{public}s, component=%{public}s, "
        "descriptorValid=%{public}s, renderId=%{public}d, surfaceId=%{public}s",
        descriptor.GetString("id", "").c_str(), descriptor.GetString("component", "").c_str(),
        descriptor.IsValid() ? "true" : "false", context.renderId, context.surfaceId.c_str());
    return ApplyExtendedDescriptor(descriptor, context);
}

bool ExtendedComponent::UpdateFromDescriptor(const JsonValue& descriptor, const RenderContext& context)
{
    LOG_A2UI(LOG_DEBUG,
        "ExtendedComponent::UpdateFromDescriptor - componentId=%{public}s, component=%{public}s, "
        "descriptorValid=%{public}s, renderId=%{public}d, surfaceId=%{public}s",
        descriptor.GetString("id", "").c_str(), descriptor.GetString("component", "").c_str(),
        descriptor.IsValid() ? "true" : "false", context.renderId, context.surfaceId.c_str());
    return ApplyExtendedDescriptor(descriptor, context);
}

bool ExtendedComponent::CreateArkUINode()
{
    return nativeView_ != nullptr;
}

void ExtendedComponent::ApplyComponentSpecificAttributes(const JsonValue& normalizedDescriptor)
{
    static_cast<void>(normalizedDescriptor);
}

void ExtendedComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) {}

void ExtendedComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    static_cast<void>(styles);
}

void ExtendedComponent::ValidateComponentSpecificDynamicStylesDfx(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
{
    if (!styles.IsObject() || dynamicStyleKeys.empty()) {
        return;
    }

    auto hasDynamicStyle = [&dynamicStyleKeys, &styles](const char* styleName) {
        return styleName != nullptr && dynamicStyleKeys.find(styleName) != dynamicStyleKeys.end() &&
               styles.Has(styleName);
    };
    auto reportTypeMismatch = [this](const std::string& path, const std::string& expected) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + path + " expects " + expected + ", fallback/reset has been applied", path);
    };
    auto reportInvalidValue = [this](const std::string& path, const std::string& reason) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + path + " " + reason + ", fallback/reset has been applied", path);
    };
    auto reportUndefinedField = [this](const std::string& path) {
        ReportExtendedSchemaWarning(
            SCHEMA_ERROR_CODE_UNDEFINED_FIELD, "Property " + path + " is undefined and has been ignored", path);
    };
    auto validateNumber = [&styles, &hasDynamicStyle, &reportTypeMismatch, &reportInvalidValue](
                              const char* styleName, bool requirePositive) {
        if (!hasDynamicStyle(styleName)) {
            return;
        }
        JsonValue value = styles.GetItem(styleName);
        float parsed = 0.0F;
        bool parsedNumber = StyleApplyUtils::ParseNumber(value, parsed);
        std::string path = "styles." + std::string(styleName);
        if (!value.IsNumber()) {
            reportTypeMismatch(path, "number");
            return;
        }
        if (!parsedNumber || !std::isfinite(parsed) || (requirePositive && parsed <= 0.0F) ||
            (!requirePositive && parsed < 0.0F)) {
            reportInvalidValue(path, "is out of range");
        }
    };
    auto validateNumberType = [&styles, &hasDynamicStyle, &reportTypeMismatch](const char* styleName) {
        if (!hasDynamicStyle(styleName)) {
            return;
        }
        if (!styles.GetItem(styleName).IsNumber()) {
            reportTypeMismatch("styles." + std::string(styleName), "number");
        }
    };
    auto validateNumberRange = [&styles, &hasDynamicStyle, &reportTypeMismatch, &reportInvalidValue](
                                   const char* styleName, double minValue, bool hasMaxValue, double maxValue) {
        if (!hasDynamicStyle(styleName)) {
            return;
        }
        JsonValue value = styles.GetItem(styleName);
        std::string path = "styles." + std::string(styleName);
        if (!value.IsNumber()) {
            reportTypeMismatch(path, "number");
            return;
        }
        double number = value.GetNumberValue(0.0);
        if (!std::isfinite(number) || number < minValue || (hasMaxValue && number > maxValue)) {
            reportInvalidValue(path, "is out of range");
        }
    };
    auto validateStringEnum = [&styles, &hasDynamicStyle, &reportTypeMismatch, &reportInvalidValue](
                                  const char* styleName, const std::set<std::string>& allowedValues) {
        if (!hasDynamicStyle(styleName)) {
            return;
        }
        JsonValue value = styles.GetItem(styleName);
        std::string path = "styles." + std::string(styleName);
        if (!value.IsString()) {
            reportTypeMismatch(path, "string enum");
            return;
        }
        std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
        if (allowedValues.find(token) == allowedValues.end()) {
            reportInvalidValue(path, "is out of enum range");
        }
    };
    auto validateBool = [&styles, &hasDynamicStyle, &reportTypeMismatch](const char* styleName) {
        if (hasDynamicStyle(styleName) && !styles.GetItem(styleName).IsBool()) {
            reportTypeMismatch("styles." + std::string(styleName), "boolean");
        }
    };
    auto validateStringColorValue = [&reportTypeMismatch, &reportInvalidValue](
                                        const JsonValue& value, const std::string& path) {
        uint32_t color = 0;
        if (!value.IsString()) {
            reportTypeMismatch(path, "string color");
            return;
        }
        if (!ExtendedStyleResolver::ParseColor(value, color)) {
            reportInvalidValue(path, "has invalid color value");
        }
    };
    auto validateStringColor = [&styles, &hasDynamicStyle, &validateStringColorValue](const char* styleName) {
        if (hasDynamicStyle(styleName)) {
            validateStringColorValue(styles.GetItem(styleName), "styles." + std::string(styleName));
        }
    };
    auto validateFontWeight = [&styles, &hasDynamicStyle, &reportTypeMismatch, &reportInvalidValue](
                                  const char* styleName, const std::set<std::string>& allowedStringValues) {
        if (!hasDynamicStyle(styleName)) {
            return;
        }
        JsonValue value = styles.GetItem(styleName);
        std::string path = "styles." + std::string(styleName);
        if (!value.IsString() && !value.IsNumber()) {
            reportTypeMismatch(path, "string or number");
            return;
        }
        if (!IsValidFontWeightValue(value, allowedStringValues)) {
            reportInvalidValue(path, "is out of enum range");
        }
    };
    auto validateCancelButton = [&styles, &hasDynamicStyle, &reportTypeMismatch, &reportInvalidValue,
                                    &reportUndefinedField, &validateStringColorValue](const char* styleName) {
        if (!hasDynamicStyle(styleName)) {
            return;
        }
        JsonValue cancelButtonValue = styles.GetItem(styleName);
        std::string path = "styles." + std::string(styleName);
        if (!cancelButtonValue.IsObject()) {
            reportTypeMismatch(path, "object");
            return;
        }
        for (JsonValue child = cancelButtonValue.GetChild(); child.IsValid(); child = child.GetNext()) {
            std::string key = child.GetKey();
            if (!key.empty() && key != "style" && key != "fontSize" && key != "fontColor") {
                reportUndefinedField(path + "." + key);
            }
        }
        if (cancelButtonValue.Has("style")) {
            JsonValue styleValue = cancelButtonValue.GetItem("style");
            std::string stylePath = path + ".style";
            if (!styleValue.IsString()) {
                reportTypeMismatch(stylePath, "string enum");
            } else if (!IsSupportedTextInputCancelButtonStyle(styleValue)) {
                reportInvalidValue(stylePath, "is out of enum range");
            }
        }
        if (cancelButtonValue.Has("fontSize")) {
            JsonValue fontSizeValue = cancelButtonValue.GetItem("fontSize");
            float fontSize = 0.0F;
            std::string fontSizePath = path + ".fontSize";
            if (fontSizeValue.IsNumber()) {
                double number = fontSizeValue.GetNumberValue(0.0);
                if (!std::isfinite(number) || number <= 0.0) {
                    reportInvalidValue(fontSizePath, "is out of range");
                }
            } else if (!fontSizeValue.IsString()) {
                reportTypeMismatch(fontSizePath, "number or dimension");
            } else if (!TryParseTextInputCancelButtonFontSize(fontSizeValue, fontSize)) {
                reportInvalidValue(fontSizePath, "is out of range");
            } else if (fontSize <= 0.0F) {
                reportInvalidValue(fontSizePath, "is out of range");
            }
        }
        if (cancelButtonValue.Has("fontColor")) {
            validateStringColorValue(cancelButtonValue.GetItem("fontColor"), path + ".fontColor");
        }
    };
    auto validateMark = [&styles, &hasDynamicStyle, &reportTypeMismatch, &reportInvalidValue, &reportUndefinedField,
                            &validateStringColorValue](const char* styleName) {
        if (!hasDynamicStyle(styleName)) {
            return;
        }
        JsonValue markValue = styles.GetItem(styleName);
        std::string path = "styles." + std::string(styleName);
        if (!markValue.IsObject()) {
            reportTypeMismatch(path, "object");
            return;
        }
        for (JsonValue child = markValue.GetChild(); child.IsValid(); child = child.GetNext()) {
            std::string key = child.GetKey();
            if (!key.empty() && key != "strokeColor" && key != "size" && key != "strokeWidth") {
                reportUndefinedField(path + "." + key);
            }
        }
        if (markValue.Has("strokeColor")) {
            validateStringColorValue(markValue.GetItem("strokeColor"), path + ".strokeColor");
        }
        auto validateMarkNumber = [&markValue, &path, &reportTypeMismatch, &reportInvalidValue](const char* key) {
            if (key == nullptr || !markValue.Has(key)) {
                return;
            }
            JsonValue value = markValue.GetItem(key);
            std::string itemPath = path + "." + key;
            if (!value.IsNumber()) {
                reportTypeMismatch(itemPath, "number");
                return;
            }
            double number = value.GetNumberValue(0.0);
            if (!std::isfinite(number) || number <= 0.0) {
                reportInvalidValue(itemPath, "is out of range");
            }
        };
        validateMarkNumber("size");
        validateMarkNumber("strokeWidth");
    };
    std::string componentType = GetType();
    if (componentType == "Button") {
        validateNumber("fontSize", true);
        validateNumber("minFontSize", true);
        validateNumber("maxFontSize", true);
        validateNumberRange("minFontScale", 0.0, true, 1.0);
        validateNumberRange("maxFontScale", 1.0, false, 0.0);
        validateStringEnum("fontScaleMode", { "custom", "followSystem" });
        validateStringColor("fontColor");
        validateFontWeight("fontWeight", { "normal", "regular", "medium", "bold", "bolder" });
        return;
    }

    if (componentType == "TextInput") {
        validateStringColor("fontColor");
        validateStringColor("placeholderColor");
        validateStringColor("caretColor");
        validateStringColor("selectedBackgroundColor");
        validateNumber("fontSize", true);
        validateNumber("minFontSize", true);
        validateNumber("maxFontSize", true);
        validateNumber("maxLines", true);
        validateNumberRange("minFontScale", 0.0, true, 1.0);
        validateNumberRange("maxFontScale", 1.0, false, 0.0);
        validateStringEnum("fontScaleMode", { "custom", "followSystem" });
        validateStringEnum("textAlign",
            { "start", "center", "end", "justify", "left", "leftToRight", "ltr", "right", "rightToLeft", "rtl" });
        validateStringEnum("wordBreak", { "normal", "breakAll", "breakWord", "hyphenation" });
        validateBool("showUnderline");
        validateCancelButton("cancelButton");
        validateFontWeight("fontWeight", { "lighter", "normal", "regular", "medium", "bold", "bolder" });
        if (hasDynamicStyle("underlineColor")) {
            JsonValue value = styles.GetItem("underlineColor");
            if (value.IsObject()) {
                const char* colorKeys[] = { "typing", "normal", "error", "disable" };
                for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
                    std::string key = child.GetKey();
                    if (!key.empty() && key != "typing" && key != "normal" && key != "error" && key != "disable") {
                        reportUndefinedField("styles.underlineColor." + key);
                    }
                }
                for (const char* key : colorKeys) {
                    if (key != nullptr && value.Has(key)) {
                        validateStringColorValue(value.GetItem(key), "styles.underlineColor." + std::string(key));
                    }
                }
            } else {
                validateStringColorValue(value, "styles.underlineColor");
            }
        }
        return;
    }

    if (componentType == "Toggle") {
        validateStringColor("selectedColor");
        validateStringColor("unSelectedColor");
        validateStringColor("switchPointColor");
        return;
    }

    if (componentType == "Radio") {
        validateStringColor("checkedBackgroundColor");
        validateStringColor("unCheckedBorderColor");
        validateStringColor("indicatorColor");
        return;
    }

    if (componentType == "Checkbox") {
        validateStringColor("selectedColor");
        validateStringColor("unselectedColor");
        validateStringEnum("shape", { "circle", "rounded_square" });
        validateMark("mark");
        return;
    }

    if (componentType == "CheckboxGroup") {
        validateStringColor("selectedColor");
        validateStringColor("unSelectedColor");
        validateStringEnum("checkboxShape", { "circle", "rounded_square" });
        validateMark("mark");
    }
}

void ExtendedComponent::RegisterExtendedListeners()
{
    RegisterClickHandler();
    RegisterAppearHandler();
    RegisterComponentSpecificListeners();
}

void ExtendedComponent::RegisterClickHandler()
{
    if (HasEventHandler("onClick")) {
        RegisterOnClickWithContext([this](const JsonValue& context) { DispatchEvent("onClick", context); });
    } else {
        RegisterOnClickWithContext(nullptr);
    }
}

void ExtendedComponent::RegisterAppearHandler()
{
    if (HasEventHandler("onAppear")) {
        RegisterNodeEventHandler(A2UINodeEventType::ON_APPEAR, [this]() { DispatchEvent("onAppear"); });
    } else {
        RegisterNodeEventHandler(A2UINodeEventType::ON_APPEAR, nullptr);
    }
}

void ExtendedComponent::RegisterComponentSpecificListeners() {}

bool ExtendedComponent::IsExpressionSupported() const
{
    return true;
}

bool ExtendedComponent::IsExpressionCandidate(const JsonValue& value) const
{
#ifdef ENABLE_EXPRESSION_ENGINE
    if (!value.IsString()) {
        return false;
    }
    std::string text = value.GetStringValue("");
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    std::string trimmed = text.substr(begin, end - begin);
    return ExpressionEngine::IsExpression(trimmed);
#else
    return false;
#endif
}

bool ExtendedComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    if (propertyName == "styles") {
        return true;
    }
    if (EventHandlerParser::KNOWN_EVENT_NAMES.count(propertyName) > 0) {
        return true;
    }
    return A2UIComponent::IsKnownAdditionalDescriptorKey(propertyName);
}

std::shared_ptr<DataModel> ExtendedComponent::GetDynamicResolveDataModel() const
{
    return renderContext_.dataModel;
}

void ExtendedComponent::ApplyDeclaredPropertyOrFallback(const JsonValue& descriptor, const std::string& propertyName)
{
    if (propertyName.empty()) {
        return;
    }
    if (descriptor.IsObject() && descriptor.Has(propertyName.c_str())) {
        SetPropertyFromDescriptor(propertyName, descriptor);
        return;
    }

    RemoveBindingsForProperty(propertyName);
    ApplyRuntimeProperty(propertyName, JsonValue(), false);
}

bool ExtendedComponent::HasEventHandler(const std::string& eventName) const
{
    if (eventName.empty()) {
        return false;
    }
    return eventHandlers_.find(eventName) != eventHandlers_.end();
}

void ExtendedComponent::DispatchEvent(const std::string& eventName, const JsonValue& extraContext) const
{
    DispatchEventToHandlers({ eventHandlers_, eventName, GetSurfaceId(), GetComponentId(), GetRenderId(), extraContext,
        &GetLocalVariables() });
}

void ExtendedComponent::DispatchActionInfo(
    const std::string& actionName, const std::shared_ptr<ActionInfo>& actionInfo, const JsonValue& extraContext) const
{
    if (actionInfo == nullptr || !actionInfo->IsValid()) {
        LOG_A2UI(LOG_DEBUG,
            "ExtendedComponent::DispatchActionInfo - action not found, componentId=%{public}s, action=%{public}s",
            GetComponentId().c_str(), actionName.c_str());
        return;
    }

    switch (actionInfo->GetType()) {
        case ActionType::FUNCTION_CALL: {
            std::shared_ptr<FunctionCallInfo> functionCall = actionInfo->GetFunctionCall();
            JsonValue functionCallDescriptor = actionInfo->GetFunctionCallDescriptor();
            bool resolvedDynamically = false;
            SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(GetRenderId(), GetSurfaceId());
            std::shared_ptr<DataModel> dataModel =
                surface != nullptr ? surface->GetOrCreateDataModel() : GetRenderContext().dataModel;
            if (functionCallDescriptor.IsValid()) {
                DynamicResolveContext context = { .renderId = GetRenderId(),
                    .surfaceId = GetSurfaceId(),
                    .componentId = GetComponentId(),
                    .dataModel = dataModel,
                    .allowExpression = true,
                    .localVariables = GetLocalVariables() };
                std::shared_ptr<FunctionCallInfo> resolvedFunctionCall =
                    DynamicValueResolver::ResolveFunctionCallDescriptor(functionCallDescriptor, context);
                if (resolvedFunctionCall != nullptr) {
                    functionCall = resolvedFunctionCall;
                    resolvedDynamically = true;
                }
            }
            if (functionCall == nullptr) {
                LOG_A2UI(LOG_WARN, "ExtendedComponent::DispatchActionInfo: functionCall is null, action=%{public}s",
                    actionName.c_str());
                return;
            }
            bool invokeResult = false;
            if (NativeActionRegistry::GetInstance().HasAction(functionCall->GetFunctionName())) {
                EventHandlerChainExecutor::ExecutionContext nativeActionContext;
                nativeActionContext.renderId = GetRenderId();
                nativeActionContext.surfaceId = GetSurfaceId();
                nativeActionContext.componentId = GetComponentId();
                nativeActionContext.dataModel = dataModel;
                nativeActionContext.eventContext = extraContext;
                nativeActionContext.externalEventContext = extraContext;
                nativeActionContext.hasExternalEventContext = extraContext.IsValid();
                nativeActionContext.localVariables = GetLocalVariables();
                NativeActionRegistry::GetInstance().Execute(
                    functionCall->GetFunctionName(), functionCall->GetArgs(), nativeActionContext);
                invokeResult = true;
            } else if (NativeFunctionRegistry::GetInstance().HasFunction(functionCall->GetFunctionName())) {
                DynamicResolveContext nativeContext = {
                    .renderId = GetRenderId(), .surfaceId = GetSurfaceId(), .componentId = GetComponentId()
                };
                JsonValue normalizedArgs;
                std::string normalizedReturnType = functionCall->GetReturnType();
                auto normalizedCall = std::make_shared<FunctionCallInfo>(
                    functionCall->GetFunctionName(), functionCall->GetArgs(), functionCall->GetReturnType());
                bool normalized = FunctionBridge::GetInstance().NormalizeFunctionCall(GetRenderId(), GetSurfaceId(),
                    GetComponentId(), normalizedCall, normalizedArgs, normalizedReturnType);
                if (normalized) {
                    auto nativeResult = NativeFunctionRegistry::GetInstance().Execute(functionCall->GetFunctionName(),
                        normalizedArgs, nativeContext, normalizedReturnType == "void" ? "" : normalizedReturnType);
                    invokeResult = nativeResult.success;
                } else {
                    LOG_A2UI(LOG_WARN,
                        "ExtendedComponent::DispatchActionInfo: native function normalize failed, "
                        "function=%{public}s",
                        functionCall->GetFunctionName().c_str());
                }
            } else {
                invokeResult =
                    FunctionBridge::GetInstance().Invoke(GetRenderId(), GetSurfaceId(), GetComponentId(), functionCall);
            }
            LOG_A2UI(LOG_DEBUG,
                "ExtendedComponent::DispatchActionInfo - invoke function, componentId=%{public}s, action=%{public}s, "
                "call=%{public}s, returnType=%{public}s, resolvedDynamically=%{public}s, result=%{public}s",
                GetComponentId().c_str(), actionName.c_str(), functionCall->GetFunctionName().c_str(),
                functionCall->GetReturnType().c_str(), resolvedDynamically ? "true" : "false",
                invokeResult ? "true" : "false");
            return;
        }
        case ActionType::EVENT: {
            EventResolveContext context = {
                .renderId = GetRenderId(), .surfaceId = GetSurfaceId(), .componentId = GetComponentId()
            };
            JsonValue resolvedContext = EventContextResolver::Resolve(actionInfo->GetEventContextDescriptor(), context);
            JsonValue dispatchContext = MergeEventContext(resolvedContext, extraContext);
            bool dispatchResult = ActionDispatchBridge::GetInstance().Dispatch(
                GetRenderId(), GetSurfaceId(), GetComponentId(), actionInfo->GetEventName(), dispatchContext);
            LOG_A2UI(LOG_DEBUG,
                "ExtendedComponent::DispatchActionInfo - dispatch event, componentId=%{public}s, action=%{public}s, "
                "eventName=%{public}s, result=%{public}s",
                GetComponentId().c_str(), actionName.c_str(), actionInfo->GetEventName().c_str(),
                dispatchResult ? "true" : "false");
            return;
        }
        default:
            LOG_A2UI(LOG_WARN,
                "ExtendedComponent::DispatchActionInfo - unsupported action type, componentId=%{public}s, "
                "action=%{public}s",
                GetComponentId().c_str(), actionName.c_str());
            return;
    }
}

const EventHandlerMap& ExtendedComponent::GetEventHandlers() const
{
    return eventHandlers_;
}

const RenderContext& ExtendedComponent::GetRenderContext() const
{
    return renderContext_;
}

void ExtendedComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    if (StyleResolver::IsStyleBindingProperty(property)) {
        std::string styleName = StyleResolver::ExtractStyleNameFromBindingProperty(property);
        LOG_A2UI(LOG_DEBUG,
            "ExtendedComponent::OnDataUpdate - route to style update, componentId=%{public}s, "
            "bindingProperty=%{public}s, "
            "styleName=%{public}s, valueType=%{public}s",
            GetComponentId().c_str(), property.c_str(), styleName.c_str(), value.GetTypeName());
        for (const auto& binding : dataBindings_) {
            if (binding.propertyName_ != property) {
                continue;
            }
            DynamicResolveContext context = { .renderId = GetRenderId(),
                .surfaceId = GetSurfaceId(),
                .componentId = GetComponentId(),
                .dataModel = GetRenderContext().dataModel,
                .allowExpression = true,
                .localVariables = GetLocalVariables() };
            if (binding.type_ == BindingType::EXPRESSION) {
#ifdef ENABLE_EXPRESSION_ENGINE
                std::unique_ptr<JsonAdapter> expressionAdapter =
                    JsonAdapter::CreateString("{{ " + binding.expression_ + " }}");
                if (expressionAdapter != nullptr) {
                    ResolvedValue resolved = DynamicValueResolver::Resolve(expressionAdapter->GetRoot(), context);
                    if (resolved.success && resolved.value.IsValid()) {
                        ApplySingleResolvedStyle(styleName, resolved.value);
                    }
                }
#endif
                return;
            }
            if (binding.type_ == BindingType::FUNCTION_CALL) {
                ResolvedValue resolved =
                    DynamicValueResolver::ResolveRecursivelyAllowPartial(binding.functionCallDescriptor_, context);
                if (resolved.success && resolved.value.IsValid()) {
                    ApplySingleResolvedStyle(styleName, resolved.value);
                } else {
                    LOG_A2UI(LOG_WARN,
                        "ExtendedComponent::OnDataUpdate - style descriptor resolve failed, componentId=%{public}s, "
                        "bindingProperty=%{public}s, reason=%{public}s",
                        GetComponentId().c_str(), property.c_str(), resolved.errorMessage.c_str());
                }
                return;
            }
        }
        ApplySingleResolvedStyle(styleName, value);
        return;
    }
    A2UIComponent::OnDataUpdate(property, value);
}

void ExtendedComponent::OnConfigChange(const ThemeContext& context)
{
    if (!hasCachedShadow_ || nodeApplier_ == nullptr) {
        return;
    }

    // Re-apply the cached shadow so its color follows the new theme context.
    std::vector<DescriptorValidationIssue> unusedIssues;
    auto theme = GetCommonTheme();
    if (theme == nullptr) {
        return;
    }
    ExtendedStyleResolver::ApplyShadow(cachedShadowValue_, *nodeApplier_, unusedIssues, theme);
}

void ExtendedComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListDescriptor();
    if (!SupportsExtendedChildren(GetType()) || !descriptor.IsObject() || !descriptor.Has("children")) {
        return;
    }
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

bool ExtendedComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    childIds.clear();
    if (!SupportsExtendedChildren(GetType())) {
        return false;
    }
    bool result = ExpandTemplateChildrenEager(childList, surfaceSlot, childIds);
    if (result) {
        surfaceSlot.OnTemplateExpansionResolved(GetComponentId());
    } else {
        surfaceSlot.OnTemplateExpansionDeferred(GetComponentId());
    }
    return result;
}

ArkUINodeApiAdapter* ExtendedComponent::GetNodeApplier() const
{
    return nodeApplier_.get();
}

bool ExtendedComponent::IsApplyingStyleDeltaUpdate() const
{
    return isApplyingStyleDeltaUpdate_;
}

void ExtendedComponent::ReportExtendedSchemaWarning(
    const std::string& code, const std::string& message, const std::string& propertyPath) const
{
    if (GetRenderId() < 0) {
        return;
    }
    std::string itemName = GetType().empty() ? "component" : GetType();
    WarningDispatchBridge::GetInstance().Dispatch(GetRenderId(), GetSurfaceId(), GetComponentId(), code, message,
        BuildExtendedSchemaWarningPath(GetComponentId(), propertyPath), "component", itemName);
}

bool ExtendedComponent::ApplyExtendedDescriptor(const JsonValue& descriptor, const RenderContext& context)
{
    renderContext_ = context;
    SetRenderId(context.renderId);
    SetSurfaceId(context.surfaceId);
    if (!CreateArkUINode()) {
        LOG_A2UI(LOG_WARN, "ExtendedComponent::ApplyExtendedDescriptor: create ArkUI node failed");
        return false;
    }

    NormalizedExtendedDescriptor normalized = ExtendedDescriptorNormalizer::Normalize(descriptor);
    if (!normalized.IsValid()) {
        LOG_A2UI(LOG_WARN, "ExtendedComponent::ApplyExtendedDescriptor: normalized descriptor is invalid");
        return false;
    }

    // DFX: dispatch validation issues collected during normalization
    for (const auto& issue : normalized.validationIssues) {
        ReportSchemaWarning(issue.code, issue.message, issue.path);
    }

    if (nodeApplier_ == nullptr) {
        nodeApplier_ = std::make_shared<ArkUINodeApiAdapter>([this]() { return GetNativeView(); },
            [this]() { return GetComponentId(); },
            [this](float top, float right, float bottom, float left) { SetMargin(top, right, bottom, left); },
            [this]() { ResetCommonMargin(); },
            [this](const std::function<void()>& onClick) { RegisterOnClick(onClick); });
        LOG_A2UI(LOG_DEBUG,
            "ExtendedComponent::ApplyExtendedDescriptor - created node applier, componentId=%{public}s, "
            "applier=%{public}p",
            normalized.descriptor.GetString("id", "").c_str(), nodeApplier_.get());
    }

    ApplyDescriptor(normalized.descriptor);
    ApplyComponentSpecificAttributes(normalized.descriptor);
    ApplyResolvedStyles(normalized.styles);
    ParseAndRegisterEventHandlers(normalized.descriptor);
    return true;
}

void ExtendedComponent::ApplyResolvedStyles(const JsonValue& styles)
{
    if (nodeApplier_ == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedComponent::ApplyResolvedStyles - node applier is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    // DFX: validate styles is an object before parsing
    if (styles.IsValid() && !styles.IsObject()) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles expects object value, got type '" + std::string(styles.GetTypeName()) + "'", "styles");
    }
    ValidateComponentSpecificStylesSchema(styles);

    StyleParseResult parseResult = StyleParser::Parse(styles);

    // DFX: report unknown style keys (parsed as StylePropertyName::UNKNOWN by StyleParser)
    for (const auto& property : parseResult.properties) {
        if (property.name == StylePropertyName::UNKNOWN && !property.rawName.empty() &&
            !IsExtendedComponentSpecificStyleKey(GetType(), property.rawName)) {
            ReportSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
                "Property styles." + property.rawName + " is undefined in extended style schema and has been ignored",
                "styles." + property.rawName);
        }
    }

    StyleResolveResult resolveResult =
        StyleResolver::Resolve(parseResult, renderContext_, GetComponentId(), appliedStyleKeys_, GetLocalVariables());

    // DFX: report style parse/resolve errors (resolveResult.errors includes parseResult.errors)
    for (const auto& error : resolveResult.errors) {
        std::string path = error.property.empty() ? "styles" : "styles." + error.property;
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "Style error at " + path + ": " + error.message, path);
    }

    for (const auto& bindingProperty : resolveResult.clearBindingProperties) {
        RemoveBindingsForProperty(bindingProperty);
    }
    for (const auto& resetProperty : resolveResult.resetProperties) {
        ExtendedStyleResolver::Reset(resetProperty, *nodeApplier_, renderContext_.apiVersion);
    }
    for (const auto& binding : resolveResult.bindings) {
        if (binding.bindingProperty.empty()) {
            continue;
        }
        if (binding.kind == StyleBindingKind::PATH && !binding.dataPath.empty()) {
            AddBinding(binding.bindingProperty, binding.dataPath);
            continue;
        }
        if (binding.kind == StyleBindingKind::EXPRESSION && !binding.expression.empty() &&
            !binding.globalVarDeps.empty()) {
            DataBinding expressionBinding(binding.bindingProperty, binding.expression, binding.globalVarDeps);
            expressionBinding.dataPath_ = binding.dataPath;
            dataBindings_.push_back(std::move(expressionBinding));
            continue;
        }
        if (binding.kind == StyleBindingKind::FUNCTION_CALL) {
            DataBinding functionBinding(
                binding.bindingProperty, binding.dataPath, BindingType::FUNCTION_CALL, binding.functionCallDescriptor);
            functionBinding.globalVarDeps_ = binding.globalVarDeps;
            dataBindings_.push_back(std::move(functionBinding));
        }
    }

    if (resolveResult.resolvedStyles.IsObject()) {
        // Cache shadow value for theme change re-application
        JsonValue shadowValue = resolveResult.resolvedStyles.GetItem("shadow");
        if (shadowValue.IsValid()) {
            cachedShadowValue_ = shadowValue;
            hasCachedShadow_ = true;
        }

        ValidateComponentSpecificDynamicStylesDfx(
            resolveResult.resolvedStyles, resolveResult.dynamicallyResolvedStyleKeys);
        if (!resolveResult.dynamicallyResolvedStyleKeys.empty()) {
            MarkDescriptorDynamicBindingsResolved();
        }
        isApplyingStyleDeltaUpdate_ = false;
        ConstraintDispatchContext dispatchCtx = { .renderId = GetRenderId(),
            .componentId = GetComponentId(),
            .nodeUniqueId = GetNativeNodeUniqueId(),
            .componentType = GetType(),
            .apiVersion = renderContext_.apiVersion,
            .commonTheme = GetCommonTheme() };
        std::vector<DescriptorValidationIssue> styleResolverIssues;
        ExtendedStyleResolver::ResolveAndApply(
            resolveResult.resolvedStyles, *nodeApplier_, dispatchCtx, styleResolverIssues);
        for (const auto& issue : styleResolverIssues) {
            ReportSchemaWarning(issue.code, issue.message, issue.path);
        }
        ApplyComponentSpecificStyles(resolveResult.resolvedStyles, *nodeApplier_);
        isApplyingStyleDeltaUpdate_ = false;
    } else {
        LOG_A2UI(LOG_DEBUG,
            "ExtendedComponent::ApplyResolvedStyles - no resolved style object to apply, componentId=%{public}s",
            GetComponentId().c_str());
    }

    appliedStyleKeys_ = resolveResult.currentStyleKeys;
}

void ExtendedComponent::ApplySingleResolvedStyle(const std::string& styleName, const JsonValue& styleValue)
{
    if (nodeApplier_ == nullptr || styleName.empty()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedComponent::ApplySingleResolvedStyle - skipped, componentId=%{public}s, "
            "nodeApplierNull=%{public}s, "
            "styleName=%{public}s",
            GetComponentId().c_str(), nodeApplier_ == nullptr ? "true" : "false", styleName.c_str());
        return;
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "ExtendedComponent::ApplySingleResolvedStyle - create adapter failed, componentId=%{public}s, "
            "styleName=%{public}s",
            GetComponentId().c_str(), styleName.c_str());
        return;
    }

    JsonValue root = adapter->GetRoot();
    if (!root.Put(styleName.c_str(), styleValue)) {
        LOG_A2UI(LOG_WARN,
            "ExtendedComponent::ApplySingleResolvedStyle - put style failed, componentId=%{public}s, "
            "styleName=%{public}s",
            GetComponentId().c_str(), styleName.c_str());
        return;
    }

    isApplyingStyleDeltaUpdate_ = true;
    ValidateComponentSpecificDynamicStylesDfx(root, { styleName });
    ConstraintDispatchContext dispatchCtx = { .renderId = GetRenderId(),
        .componentId = GetComponentId(),
        .nodeUniqueId = GetNativeNodeUniqueId(),
        .componentType = GetType(),
        .apiVersion = renderContext_.apiVersion,
        .commonTheme = GetCommonTheme() };
    std::vector<DescriptorValidationIssue> styleResolverIssues;
    ExtendedStyleResolver::ResolveAndApply(root, *nodeApplier_, dispatchCtx, styleResolverIssues);
    for (const auto& issue : styleResolverIssues) {
        ReportSchemaWarning(issue.code, issue.message, issue.path);
    }
    ApplyComponentSpecificStyles(root, *nodeApplier_);
    isApplyingStyleDeltaUpdate_ = false;

    if (styleName == "shadow") {
        cachedShadowValue_ = styleValue;
        hasCachedShadow_ = true;
    }

    appliedStyleKeys_.insert(styleName);
}

void ExtendedComponent::ParseAndRegisterEventHandlers(const JsonValue& descriptor)
{
    eventHandlers_.clear();
    eventHandlers_ = EventHandlerParser::Parse(descriptor);
    if (!eventHandlers_.empty()) {
        LOG_A2UI(LOG_DEBUG,
            "ExtendedComponent::ParseAndRegisterEventHandlers - parsed event handlers, "
            "componentId=%{public}s, count=%{public}zu",
            GetComponentId().c_str(), eventHandlers_.size());
    }
    RegisterExtendedListeners();
}

JsonValue ExtendedComponent::MergeEventContext(const JsonValue& baseContext, const JsonValue& extraContext)
{
    if (!extraContext.IsValid()) {
        if (baseContext.IsObject()) {
            return CloneJsonValue(baseContext);
        }
        return CreateEmptyObjectValue();
    }

    if (!extraContext.IsObject()) {
        if (!baseContext.IsObject() || !JsonObjectHasEntries(baseContext)) {
            return CloneJsonValue(extraContext);
        }

        std::unique_ptr<JsonAdapter> contextAdapter = JsonAdapter::Clone(baseContext);
        if (contextAdapter == nullptr) {
            return JsonValue();
        }

        JsonValue contextRoot = contextAdapter->GetRoot();
        // Preserve resolved DSL context fields while still surfacing a primitive event payload.
        if (contextRoot.Has("value")) {
            contextRoot.Replace("value", extraContext);
        } else {
            contextRoot.Put("value", extraContext);
        }
        return contextRoot;
    }

    std::unique_ptr<JsonAdapter> contextAdapter =
        baseContext.IsObject() ? JsonAdapter::Clone(baseContext) : JsonAdapter::CreateObject();
    if (contextAdapter == nullptr) {
        return JsonValue();
    }

    JsonValue contextRoot = contextAdapter->GetRoot();
    for (JsonValue child = extraContext.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }
        if (contextRoot.Has(key.c_str())) {
            contextRoot.Replace(key.c_str(), child);
        } else {
            contextRoot.Put(key.c_str(), child);
        }
    }

    return contextRoot;
}

std::shared_ptr<ExtendedCommonTheme> ExtendedComponent::GetCommonTheme()
{
    // Try to get from cache first
    auto theme = cachedCommonTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }

    // Cache miss, get from base class
    std::shared_ptr<ThemeBase> baseTheme = GetTheme();
    theme = std::dynamic_pointer_cast<ExtendedCommonTheme>(baseTheme);
    if (theme != nullptr) {
        cachedCommonTheme_ = theme;
    } else {
        LOG_A2UI(LOG_WARN, "ExtendedComponent::GetCommonTheme: ExtendedCommonTheme is null, componentId=%{public}s",
            GetComponentId().c_str());
    }
    return theme;
}

void ExtendedComponent::OnFontSizeScaleChanged(float newScale)
{
    renderContext_.fontSizeScale = newScale;
}

} // namespace NativeModule
