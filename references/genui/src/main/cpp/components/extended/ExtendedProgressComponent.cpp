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

#include "ExtendedProgressComponent.h"

#include <cmath>
#include <functional>
#include <map>

#include "styles/StyleApplyUtils.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "ExtendedProgressTheme.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_PROGRESS_VALUE = 0.0F;
constexpr float DEFAULT_PROGRESS_TOTAL = 100.0F;
constexpr int32_t DEFAULT_PROGRESS_TYPE = 0;
enum class ProgressDfxDecision { APPLIED, FALLBACK, PRESERVED };

const char* ToUpdateTypeLabel(bool isDeltaUpdate)
{
    static constexpr const char* UPDATE_TYPE_LABELS[] = { "full", "delta" };
    return UPDATE_TYPE_LABELS[static_cast<size_t>(isDeltaUpdate)];
}

const char* ToDecisionLabel(ProgressDfxDecision decision)
{
    static constexpr const char* DECISION_LABELS[] = { "applied", "fallback", "preserved" };
    return DECISION_LABELS[static_cast<size_t>(decision)];
}

const char* DescribeStyleResolution(bool isDeltaUpdate)
{
    static_cast<void>(isDeltaUpdate);
    return "fallback to default value";
}

bool HasNonObjectStylePayload(const JsonValue& styles)
{
    return styles.IsValid() && !styles.IsObject();
}

bool IsInvalidProgressTotal(double rawTotal)
{
    return !std::isfinite(rawTotal) || rawTotal <= 0.0;
}

bool IsDynamicProgressPropertyDescriptor(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

bool IsResolvableProgressPropertyValue(const JsonValue& value)
{
    return value.IsNumber() || value.IsString() || IsDynamicProgressPropertyDescriptor(value);
}

std::string DescribeJsonValueForLog(const JsonValue& value)
{
    return value.IsValid() ? value.GetTypeName() : "missing";
}

void LogProgressDfxEvent(const std::string& componentId, const char* event, const char* field, const JsonValue& value,
    bool isDeltaUpdate, ProgressDfxDecision decision, const std::string& extra = "")
{
    LOG_A2UI(LOG_INFO,
        "ExtendedProgressComponent::%{public}s - componentId=%{public}s, updateType=%{public}s, field=%{public}s, "
        "input=%{public}s, result=%{public}s, extra=%{public}s",
        event, componentId.c_str(), ToUpdateTypeLabel(isDeltaUpdate), field, DescribeJsonValueForLog(value).c_str(),
        ToDecisionLabel(decision), extra.c_str());
}

} // namespace

ExtendedProgressComponent::ExtendedProgressComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::PROGRESS))
{
    SetTotal(DEFAULT_PROGRESS_TOTAL);
    SetValue(DEFAULT_PROGRESS_VALUE);
    SetProgressType(DEFAULT_PROGRESS_TYPE);
    SetColor(ResolveDefaultColorByType(progressType_));
}

std::string ExtendedProgressComponent::GetType() const
{
    return "Progress";
}

void ExtendedProgressComponent::ReportStyleWarning(
    const std::string& code, const std::string& styleName, const std::string& message) const
{
    ReportSchemaWarning(code, message, "styles." + styleName);
}

void ExtendedProgressComponent::OnConfigChange(const ThemeContext& context)
{
    if (!useDefaultColor_) {
        return;
    }
    ExtendedProgressTheme theme(context);
    uint32_t resolvedColor = theme.GetDefaultColorByType(progressType_);
    if (resolvedColor != color_) {
        SetColor(resolvedColor);
    }
}

void ExtendedProgressComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    JsonValue valueValue = descriptor.GetItem("value");
    bool hasValue = descriptor.IsObject() && descriptor.Has("value");
    if (!hasValue) {
        ApplyRuntimeProperty("value", JsonValue(), false);
        LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "value", valueValue, false,
            ProgressDfxDecision::FALLBACK, "value=" + std::to_string(value_));
    } else if (IsResolvableProgressPropertyValue(valueValue)) {
        ApplyDeclaredPropertyOrFallback(descriptor, "value");
        LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "value", valueValue, false,
            ProgressDfxDecision::APPLIED, "value=" + std::to_string(value_));
    } else {
        RemoveBindingsForProperty("value");
        ApplyRuntimeProperty("value", JsonValue(), false);
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            std::string("Property value expects number value, got type '") + valueValue.GetTypeName() +
                "', fallback to default value",
            "value");
        LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "value", valueValue, false,
            ProgressDfxDecision::FALLBACK, "value=" + std::to_string(value_));
    }

    JsonValue totalValue = descriptor.GetItem("total");
    bool hasTotal = descriptor.IsObject() && descriptor.Has("total");
    if (!hasTotal) {
        ApplyRuntimeProperty("total", JsonValue(), false);
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property total is missing, fallback to default value " + std::to_string(DEFAULT_PROGRESS_TOTAL), "total");
        LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "total", totalValue, false,
            ProgressDfxDecision::FALLBACK, "total=" + std::to_string(total_));
    } else if (IsResolvableProgressPropertyValue(totalValue)) {
        ApplyDeclaredPropertyOrFallback(descriptor, "total");
        if (totalValue.IsNumber()) {
            double rawTotal = totalValue.GetNumberValue(DEFAULT_PROGRESS_TOTAL);
            if (IsInvalidProgressTotal(rawTotal)) {
                ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property total got invalid number value (non-positive or non-finite), fallback to default value",
                    "total");
                LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "total", totalValue, false,
                    ProgressDfxDecision::FALLBACK, "total=" + std::to_string(total_));
            } else {
                LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "total", totalValue, false,
                    ProgressDfxDecision::APPLIED, "total=" + std::to_string(total_));
            }
        } else {
            LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "total", totalValue, false,
                ProgressDfxDecision::APPLIED, "total=" + std::to_string(total_));
        }
    } else {
        RemoveBindingsForProperty("total");
        ApplyRuntimeProperty("total", JsonValue(), false);
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            std::string("Property total expects number value, got type '") + totalValue.GetTypeName() +
                "', fallback to default value",
            "total");
        LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "total", totalValue, false,
            ProgressDfxDecision::FALLBACK, "total=" + std::to_string(total_));
    }

    if (value_ < 0.0F) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property value is negative (" + std::to_string(value_) + "), clamped to 0", "value");
        SetValue(0.0F);
        LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "value", valueValue, false,
            ProgressDfxDecision::FALLBACK, "value clamped to 0");
    }

    if (total_ > 0.0F && value_ > total_) { // GCOVR_EXCL_BR_LINE
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property value (" + std::to_string(value_) + ") exceeds total (" + std::to_string(total_) +
                "), clamped to total",
            "value");
        SetValue(total_);
        LogProgressDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "value", valueValue, false,
            ProgressDfxDecision::FALLBACK, "value clamped to total=" + std::to_string(total_));
    }
}

PropertyDeclaration ExtendedProgressComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    // GCOVR_EXCL_BR_START
    if (propertyName == "value") {
        PropertyDeclaration declaration;
        declaration.name = "value";
        declaration.type = PropertyValueType::NUMBER;
        declaration.allowDynamic = true;
        declaration.allowExpression = true;
        declaration.fallbackNumber = DEFAULT_PROGRESS_VALUE;
        declaration.applyValue = [this](const JsonValue& value) {
            SetValue(static_cast<float>(value.GetNumberValue(DEFAULT_PROGRESS_VALUE)));
        };
        return declaration;
    }
    if (propertyName == "total") {
        PropertyDeclaration declaration;
        declaration.name = "total";
        declaration.type = PropertyValueType::NUMBER;
        declaration.allowDynamic = true;
        declaration.allowExpression = true;
        declaration.fallbackNumber = DEFAULT_PROGRESS_TOTAL;
        declaration.applyValue = [this](const JsonValue& value) {
            SetTotal(static_cast<float>(value.GetNumberValue(DEFAULT_PROGRESS_TOTAL)));
        };
        return declaration;
    }
    // GCOVR_EXCL_BR_STOP
    return {};
}

void ExtendedProgressComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    const std::string componentId = GetComponentId();

    if (HasNonObjectStylePayload(styles)) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles expects object value, got type '" + std::string(styles.GetTypeName()) + "', " +
                DescribeStyleResolution(isDeltaUpdate),
            "styles");
        SetProgressType(DEFAULT_PROGRESS_TYPE);
        useDefaultColor_ = true;
        SetColor(ResolveDefaultColorByType(progressType_));
        LogProgressDfxEvent(componentId, "ApplyComponentSpecificStyles", "type", styles, isDeltaUpdate,
            ProgressDfxDecision::FALLBACK, "progressType=" + std::to_string(progressType_));
        LogProgressDfxEvent(componentId, "ApplyComponentSpecificStyles", "color", styles, isDeltaUpdate,
            ProgressDfxDecision::FALLBACK, "color_default=applied");
        return;
    }

    if (styles.Has("type")) {
        ApplyProgressTypeValue(styles.GetItem("type"));
    }
    if (styles.Has("color")) {
        ApplyColorValue(styles.GetItem("color"));
    }
}

void ExtendedProgressComponent::SetValue(float value)
{
    value_ = value;
    ArkUINodeApiAdapter::SetNodeProgressValue(nativeView_, value_);
}

void ExtendedProgressComponent::SetTotal(float total)
{
    total_ = total > 0.0F ? total : DEFAULT_PROGRESS_TOTAL;
    ArkUINodeApiAdapter::SetNodeProgressTotal(nativeView_, total_);
}

void ExtendedProgressComponent::SetColor(uint32_t color)
{
    color_ = color;
    ArkUINodeApiAdapter::SetNodeProgressColor(nativeView_, color_);
}

void ExtendedProgressComponent::SetProgressType(int32_t progressType)
{
    progressType_ = progressType;
    ArkUINodeApiAdapter::SetNodeProgressType(nativeView_, progressType_);
}

uint32_t ExtendedProgressComponent::ResolveDefaultColorByType(int32_t progressType) const
{
    ThemeContext themeContext;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        themeContext = themeManager->GetContext();
    }
    ExtendedProgressTheme theme(themeContext);
    return theme.GetDefaultColorByType(progressType);
}

uint32_t ExtendedProgressComponent::ResolveLinearDefaultColor() const
{
    return ResolveDefaultColorByType(DEFAULT_PROGRESS_TYPE);
}

void ExtendedProgressComponent::ApplyColorValue(const JsonValue& value)
{
    uint32_t parsedColor = 0;
    if (value.IsString() && StyleApplyUtils::ParseHexColorString(value.GetStringValue(""), parsedColor)) {
        useDefaultColor_ = false;
        SetColor(parsedColor);
        return;
    }
    if (value.IsValid()) {
        if (value.IsString()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "color",
                "Property color got invalid color value, fallback to default value");
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "color",
                std::string("Property color expects hex color string value, got type '") + value.GetTypeName() +
                    "', fallback to default value");
        }
    }
    useDefaultColor_ = true;
    SetColor(ResolveDefaultColorByType(progressType_));
}

void ExtendedProgressComponent::ApplyProgressTypeValue(const JsonValue& value)
{
    int32_t parsedProgressType = DEFAULT_PROGRESS_TYPE;
    if (StyleApplyUtils::ParseProgressType(value, parsedProgressType)) {
        SetProgressType(parsedProgressType);
        if (useDefaultColor_) {
            SetColor(ResolveDefaultColorByType(progressType_));
        }
        return;
    }
    if (value.IsValid()) {
        if (value.IsString()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "type",
                "Property type got invalid enum value, fallback to default value");
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "type",
                std::string("Property type expects string enum value, got type '") + value.GetTypeName() +
                    "', fallback to default value");
        }
    }
    SetProgressType(DEFAULT_PROGRESS_TYPE);
    if (useDefaultColor_) {
        SetColor(ResolveDefaultColorByType(progressType_));
    }
}

} // namespace NativeModule
