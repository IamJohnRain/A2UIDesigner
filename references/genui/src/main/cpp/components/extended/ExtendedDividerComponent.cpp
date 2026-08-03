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

#include "ExtendedDividerComponent.h"

#include "styles/StyleApplyUtils.h"
#include "styles/StyleResolver.h"
#include "theme/ThemeManager.h"
#include "utils/DisplayDensityUtils.h"
#include "utils/LogA2UI.h"

#include "ExtendedDividerTheme.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_STROKE_WIDTH = 1.0F;
constexpr char DEFAULT_STROKE_WIDTH_TOKEN[] = "1px";
constexpr bool DEFAULT_VERTICAL = false;
constexpr char DEFAULT_COLOR_TOKEN[] = "";
enum class DividerDfxDecision { APPLIED, FALLBACK, PRESERVED };

const char* ToUpdateTypeLabel(bool isDeltaUpdate)
{
    static constexpr const char* UPDATE_TYPE_LABELS[] = { "full", "delta" };
    return UPDATE_TYPE_LABELS[static_cast<size_t>(isDeltaUpdate)];
}

const char* ToDecisionLabel(DividerDfxDecision decision)
{
    static constexpr const char* DECISION_LABELS[] = { "applied", "fallback", "preserved" };
    return DECISION_LABELS[static_cast<size_t>(decision)];
}

const char* ToBoolLabel(bool value)
{
    static constexpr const char* BOOL_LABELS[] = { "false", "true" };
    return BOOL_LABELS[static_cast<size_t>(value)];
}

bool IsPathBindingDescriptorValue(const JsonValue& value)
{
    return value.IsObject() && value.Has("path");
}

std::string DescribeJsonValueForLog(const JsonValue& value)
{
    return value.IsValid() ? value.GetTypeName() : "missing";
}

void LogDividerDfxEvent(const std::string& componentId, const char* event, const char* field, const JsonValue& value,
    bool isDeltaUpdate, DividerDfxDecision decision, const std::string& extra = "")
{
    LOG_A2UI(LOG_INFO,
        "ExtendedDividerComponent::%{public}s - componentId=%{public}s, updateType=%{public}s, field=%{public}s, "
        "input=%{public}s, result=%{public}s, extra=%{public}s",
        event, componentId.c_str(), ToUpdateTypeLabel(isDeltaUpdate), field, DescribeJsonValueForLog(value).c_str(),
        ToDecisionLabel(decision), extra.c_str());
}

float ConvertFpToVp(int32_t renderId, float fp)
{
    return DisplayDensityUtils::GetInstance().ConvertFpToVp(renderId, fp);
}

} // namespace

ExtendedDividerComponent::ExtendedDividerComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::DIVIDER))
{}

std::string ExtendedDividerComponent::GetType() const
{
    return "Divider";
}

void ExtendedDividerComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    bool isStyleBindingProperty = StyleResolver::IsStyleBindingProperty(property);
    std::string styleName =
        isStyleBindingProperty ? StyleResolver::ExtractStyleNameFromBindingProperty(property) : property;
    bool isGeometryStateUpdate = styleName == "strokeWidth" || styleName == "vertical";

    if (styleName == "strokeWidth") {
        ApplyStrokeWidthPrivateValue(value, true);
        if (GetNodeApplier() != nullptr) {
            UpdateCommonDimensionState(JsonValue());
            ApplyGeometryAfterCommonDimensions();
        }
        return;
    }

    if (styleName == "vertical") {
        ApplyVerticalPrivateValue(value, true);
        if (GetNodeApplier() != nullptr) {
            UpdateCommonDimensionState(JsonValue());
            ApplyGeometryAfterCommonDimensions();
        }
        return;
    }

    ExtendedComponent::OnDataUpdate(property, value);
    if (isStyleBindingProperty || !isGeometryStateUpdate) {
        return;
    }

    if (GetNodeApplier() == nullptr) {
        return;
    }
    UpdateCommonDimensionState(JsonValue());
    ApplyGeometryAfterCommonDimensions();
}

void ExtendedDividerComponent::ReportStyleWarning(
    const std::string& code, const std::string& styleName, const std::string& message) const
{
    ReportSchemaWarning(code, message, "styles." + styleName);
}

#ifdef TDD_BUILD
std::string ExtendedDividerComponent::GetStrokeWidthUnitForTest() const
{
    static constexpr const char* STROKE_WIDTH_UNIT_LABELS[] = { "vp", "fp", "px", "%" };
    return STROKE_WIDTH_UNIT_LABELS[static_cast<size_t>(strokeWidth_.unit)];
}
#endif

void ExtendedDividerComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    static constexpr const char* STROKE_WIDTH_UNIT_LABELS[] = { "vp", "fp", "px", "%" };
    JsonValue styles = descriptor.GetItem("styles");
    const std::string componentId = GetComponentId();

    JsonValue strokeWidthValue = styles.GetItem("strokeWidth");
    bool strokeWidthUsesDeferredResolve =
        IsPathBindingDescriptorValue(strokeWidthValue) || IsExpressionCandidate(strokeWidthValue);
    DividerDfxDecision strokeWidthDecision =
        styles.IsObject() && styles.Has("strokeWidth") ? DividerDfxDecision::APPLIED : DividerDfxDecision::FALLBACK;
    float prevStrokeWidthValue = strokeWidth_.value;
    std::string prevStrokeWidthUnitStr = STROKE_WIDTH_UNIT_LABELS[static_cast<size_t>(strokeWidth_.unit)];
    if (strokeWidthDecision == DividerDfxDecision::APPLIED) {
        if (strokeWidthValue.IsString() || strokeWidthValue.IsNumber() || strokeWidthUsesDeferredResolve) {
            ApplyDeclaredPropertyOrFallback(styles, "strokeWidth");
        } else {
            RemoveBindingsForProperty("strokeWidth");
            ApplyRuntimeProperty("strokeWidth", JsonValue(), false);
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "strokeWidth",
                std::string("Property strokeWidth expects string or number value, got type '") +
                    strokeWidthValue.GetTypeName() + "', fallback to default value");
            LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "strokeWidth", strokeWidthValue, false,
                DividerDfxDecision::FALLBACK,
                "prevValue=" + std::to_string(prevStrokeWidthValue) + ", prevUnit=" + prevStrokeWidthUnitStr);
        }

        if (strokeWidthUsesDeferredResolve) {
            LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "strokeWidth", strokeWidthValue, false,
                DividerDfxDecision::APPLIED,
                "value=" + std::to_string(strokeWidth_.value) +
                    ", unit=" + STROKE_WIDTH_UNIT_LABELS[static_cast<size_t>(strokeWidth_.unit)]);
        } else {
            float parsedValue = 0.0F;
            std::string parsedUnit;
            if (StyleApplyUtils::ParseDividerStrokeWidth(strokeWidthValue, parsedValue, parsedUnit)) {
                LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "strokeWidth", strokeWidthValue, false,
                    DividerDfxDecision::APPLIED,
                    "value=" + std::to_string(strokeWidth_.value) +
                        ", unit=" + STROKE_WIDTH_UNIT_LABELS[static_cast<size_t>(strokeWidth_.unit)]);
            } else if (strokeWidthValue.IsString() || strokeWidthValue.IsNumber()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "strokeWidth",
                    "Property strokeWidth got invalid value, fallback to default value");
                LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "strokeWidth", strokeWidthValue, false,
                    DividerDfxDecision::FALLBACK,
                    "prevValue=" + std::to_string(prevStrokeWidthValue) + ", prevUnit=" + prevStrokeWidthUnitStr);
            }
        }
    } else {
        ApplyRuntimeProperty("strokeWidth", JsonValue(), false);
        LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "strokeWidth", strokeWidthValue, false,
            DividerDfxDecision::FALLBACK, "usingDefault=1px");
    }

    JsonValue verticalValue = styles.GetItem("vertical");
    bool verticalUsesDeferredResolve =
        IsPathBindingDescriptorValue(verticalValue) || IsExpressionCandidate(verticalValue);
    DividerDfxDecision verticalDecision =
        styles.IsObject() && styles.Has("vertical") ? DividerDfxDecision::APPLIED : DividerDfxDecision::FALLBACK;
    if (verticalDecision == DividerDfxDecision::APPLIED) {
        if (verticalValue.IsBool() || verticalUsesDeferredResolve) {
            ApplyDeclaredPropertyOrFallback(styles, "vertical");
        } else {
            RemoveBindingsForProperty("vertical");
            ApplyRuntimeProperty("vertical", JsonValue(), false);
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "vertical",
                std::string("Property vertical expects boolean value, got type '") + verticalValue.GetTypeName() +
                    "', fallback to default value");
            LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "vertical", verticalValue, false,
                DividerDfxDecision::FALLBACK, "vertical=" + std::string(ToBoolLabel(vertical_)));
        }

        if (verticalValue.IsBool() || verticalUsesDeferredResolve) {
            LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "vertical", verticalValue, false,
                DividerDfxDecision::APPLIED, "vertical=" + std::string(ToBoolLabel(vertical_)));
        }
    } else {
        ApplyRuntimeProperty("vertical", JsonValue(), false);
        LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "vertical", verticalValue, false,
            DividerDfxDecision::FALLBACK, "vertical=" + std::string(ToBoolLabel(vertical_)));
    }

    JsonValue colorValue = styles.GetItem("color");
    bool colorUsesDeferredResolve = IsPathBindingDescriptorValue(colorValue) || IsExpressionCandidate(colorValue);
    DividerDfxDecision colorDecision =
        styles.IsObject() && styles.Has("color") ? DividerDfxDecision::APPLIED : DividerDfxDecision::FALLBACK;
    if (colorDecision == DividerDfxDecision::APPLIED) {
        if (colorValue.IsString() || colorUsesDeferredResolve) {
            ApplyDeclaredPropertyOrFallback(styles, "color");
        } else {
            RemoveBindingsForProperty("color");
            ApplyRuntimeProperty("color", JsonValue(), false);
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "color",
                std::string("Property color expects string value, got type '") + colorValue.GetTypeName() +
                    "', fallback to default value");
            LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "color", colorValue, false,
                DividerDfxDecision::FALLBACK, "useDefaultColor=true, color=" + std::to_string(color_));
        }

        if (colorUsesDeferredResolve) {
            LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "color", colorValue, false,
                DividerDfxDecision::APPLIED, "color=" + std::to_string(color_));
        } else {
            uint32_t parsedColor = 0;
            if (colorValue.IsString() && StyleApplyUtils::ParseHexColorString(
                                             colorValue.GetStringValue(""), parsedColor)) { // GCOVR_EXCL_BR_LINE
                LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "color", colorValue, false,
                    DividerDfxDecision::APPLIED, "color=" + std::to_string(color_));
            } else if (colorValue.IsString()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "color",
                    "Property color got invalid color value, fallback to default value");
                LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "color", colorValue, false,
                    DividerDfxDecision::FALLBACK, "useDefaultColor=true, color=" + std::to_string(color_));
            }
        }
    } else {
        ApplyRuntimeProperty("color", JsonValue(), false);
        LogDividerDfxEvent(componentId, "ApplyPrivateAttributes", "color", colorValue, false,
            DividerDfxDecision::FALLBACK, "useDefaultColor=true, color=" + std::to_string(color_));
    }
}

void ExtendedDividerComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter&)
{
    ApplyPrivateGeometryStyleState(styles);
    UpdateCommonDimensionState(styles);
    ApplyGeometryAfterCommonDimensions();
}

PropertyDeclaration ExtendedDividerComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    // GCOVR_EXCL_BR_START
    if (propertyName == "strokeWidth") {
        PropertyDeclaration declaration;
        declaration.name = "strokeWidth";
        declaration.type = PropertyValueType::STRING;
        declaration.allowDynamic = true;
        declaration.allowExpression = true;
        declaration.acceptNumberForString = true;
        declaration.fallbackString = DEFAULT_STROKE_WIDTH_TOKEN;
        declaration.applyValue = [this](const JsonValue& value) { SetStrokeWidth(value); };
        return declaration;
    }
    if (propertyName == "vertical") {
        PropertyDeclaration declaration;
        declaration.name = "vertical";
        declaration.type = PropertyValueType::BOOLEAN;
        declaration.allowDynamic = true;
        declaration.allowExpression = true;
        declaration.fallbackBool = DEFAULT_VERTICAL;
        declaration.applyValue = [this](const JsonValue& value) { SetVertical(value.GetBoolValue(DEFAULT_VERTICAL)); };
        return declaration;
    }
    if (propertyName == "color") {
        PropertyDeclaration declaration;
        declaration.name = "color";
        declaration.type = PropertyValueType::STRING;
        declaration.allowDynamic = true;
        declaration.allowExpression = true;
        declaration.fallbackString = DEFAULT_COLOR_TOKEN;
        declaration.applyValue = [this](
                                     const JsonValue& value) { SetColor(value.GetStringValue(DEFAULT_COLOR_TOKEN)); };
        return declaration;
    }
    // GCOVR_EXCL_BR_STOP
    return {};
}

void ExtendedDividerComponent::SetStrokeWidth(const JsonValue& value)
{
    float parsedStrokeWidthValue = 0.0F;
    std::string parsedStrokeWidthUnit;
    if (!StyleApplyUtils::ParseDividerStrokeWidth(value, parsedStrokeWidthValue, parsedStrokeWidthUnit)) {
        strokeWidth_.value = DEFAULT_STROKE_WIDTH;
        strokeWidth_.unit = StrokeWidthUnit::PX;
    } else {
        strokeWidth_.value = parsedStrokeWidthValue;
        if (parsedStrokeWidthUnit == "px") {
            strokeWidth_.unit = StrokeWidthUnit::PX;
        } else if (parsedStrokeWidthUnit == "fp") {
            strokeWidth_.unit = StrokeWidthUnit::FP;
        } else if (parsedStrokeWidthUnit == "%") {
            strokeWidth_.unit = StrokeWidthUnit::PERCENT;
        } else {
            strokeWidth_.unit = StrokeWidthUnit::VP;
        }
    }
}

void ExtendedDividerComponent::SetVertical(bool vertical)
{
    vertical_ = vertical;
}

void ExtendedDividerComponent::SetColor(const std::string& colorValue)
{
    uint32_t parsedColor = 0;
    if (StyleApplyUtils::ParseHexColorString(colorValue, parsedColor)) {
        color_ = parsedColor;
        useDefaultColor_ = false;
    } else {
        color_ = ResolveDefaultColor();
        useDefaultColor_ = true;
    }
    ApplyColor();
}

uint32_t ExtendedDividerComponent::ResolveDefaultColor() const
{
    ThemeContext themeContext;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        themeContext = themeManager->GetContext();
    }
    ExtendedDividerTheme theme(themeContext);
    return theme.GetDefaultColor();
}

void ExtendedDividerComponent::ApplyPrivateGeometryStyleState(const JsonValue& styles)
{
    if (!styles.IsObject()) {
        return;
    }
    if (styles.Has("strokeWidth")) {
        ApplyStrokeWidthPrivateValue(styles.GetItem("strokeWidth"), false);
    }
    if (styles.Has("vertical")) {
        ApplyVerticalPrivateValue(styles.GetItem("vertical"), false);
    }
}

void ExtendedDividerComponent::ApplyStrokeWidthPrivateValue(const JsonValue& value, bool reportWarning)
{
    if (!value.IsString() && !value.IsNumber()) {
        SetStrokeWidth(JsonValue());
        if (reportWarning) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "strokeWidth",
                std::string("Property strokeWidth expects string or number value, got type '") + value.GetTypeName() +
                    "', fallback to default value");
        }
        return;
    }

    float parsedValue = 0.0F;
    std::string parsedUnit;
    if (!StyleApplyUtils::ParseDividerStrokeWidth(value, parsedValue, parsedUnit)) {
        SetStrokeWidth(JsonValue());
        if (reportWarning) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "strokeWidth",
                "Property strokeWidth got invalid value, fallback to default value");
        }
        return;
    }

    SetStrokeWidth(value);
}

void ExtendedDividerComponent::ApplyVerticalPrivateValue(const JsonValue& value, bool reportWarning)
{
    if (!value.IsBool()) {
        SetVertical(DEFAULT_VERTICAL);
        if (reportWarning) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "vertical",
                std::string("Property vertical expects boolean value, got type '") + value.GetTypeName() +
                    "', fallback to default value");
        }
        return;
    }

    SetVertical(value.GetBoolValue(DEFAULT_VERTICAL));
}

void ExtendedDividerComponent::ApplyGeometryAfterCommonDimensions()
{
    bool strokeAxisIsWidth = vertical_;
    bool strokeAxisHasCommonDimension = strokeAxisIsWidth ? hasCommonWidth_ : hasCommonHeight_;
    if (strokeAxisHasCommonDimension) {
        return;
    }
    ApplyThickness(strokeAxisIsWidth);
}

void ExtendedDividerComponent::ApplyColor()
{
    SetBackgroundColor(color_);
}

void ExtendedDividerComponent::UpdateCommonDimensionState(const JsonValue& styles)
{
    hasCommonWidth_ = false;
    hasCommonHeight_ = false;
    if (!styles.IsObject()) {
        return;
    }
    if (styles.Has("width")) {
        hasCommonWidth_ = true;
    }
    if (styles.Has("height")) {
        hasCommonHeight_ = true;
    }
}

void ExtendedDividerComponent::ApplyThickness(bool isWidth)
{
    if (strokeWidth_.unit == StrokeWidthUnit::PERCENT) {
        ResetDimension(isWidth ? A2UINodeAttributeType::WIDTH : A2UINodeAttributeType::HEIGHT);
        SetPercentDimension(isWidth, strokeWidth_.value);
        return;
    }

    float resolvedValue = strokeWidth_.value;
    if (strokeWidth_.unit == StrokeWidthUnit::PX) {
        resolvedValue = DisplayDensityUtils::GetInstance().ConvertPxToVp(GetRenderId(), strokeWidth_.value);
    } else if (strokeWidth_.unit == StrokeWidthUnit::FP) {
        resolvedValue = ConvertFpToVp(GetRenderId(), strokeWidth_.value);
    }
    ResetDimension(isWidth ? A2UINodeAttributeType::WIDTH_PERCENT : A2UINodeAttributeType::HEIGHT_PERCENT);
    SetAbsoluteDimension(isWidth, resolvedValue);
}

void ExtendedDividerComponent::SetAbsoluteDimension(bool isWidth, float value)
{
    if (isWidth) {
        SetWidth(value);
    } else {
        SetHeight(value);
    }
}

void ExtendedDividerComponent::SetPercentDimension(bool isWidth, float percent)
{
    float percentRatio = percent / 100.0F;
    if (isWidth) {
        SetWidthPercent(percentRatio);
    } else {
        SetHeightPercent(percentRatio);
    }
}

void ExtendedDividerComponent::ResetDimension(A2UINodeAttributeType attribute)
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::ResetNodeAttribute(nativeView_, attribute);
}

void ExtendedDividerComponent::OnConfigChange(const ThemeContext& context)
{
    if (!useDefaultColor_) {
        return;
    }
    ExtendedDividerTheme theme(context);
    color_ = theme.GetDefaultColor();
    ApplyColor();
}

} // namespace NativeModule
