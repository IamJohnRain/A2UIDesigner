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

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "StyleApplyUtilsInternal.h"

namespace NativeModule {

namespace {

constexpr int32_t A2UI_IMAGE_SIZE_AUTO_VALUE = static_cast<int32_t>(A2UIImageSize::AUTO);
constexpr int32_t A2UI_IMAGE_SIZE_COVER_VALUE = static_cast<int32_t>(A2UIImageSize::COVER);
constexpr int32_t A2UI_IMAGE_SIZE_CONTAIN_VALUE = static_cast<int32_t>(A2UIImageSize::CONTAIN);
constexpr int32_t A2UI_IMAGE_SIZE_FILL_VALUE = static_cast<int32_t>(A2UIImageSize::FILL);

constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_VALUE = static_cast<int32_t>(A2UILinearGradientDirection::LEFT);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_TOP_VALUE = static_cast<int32_t>(A2UILinearGradientDirection::TOP);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_VALUE = static_cast<int32_t>(A2UILinearGradientDirection::RIGHT);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_BOTTOM_VALUE =
    static_cast<int32_t>(A2UILinearGradientDirection::BOTTOM);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_TOP_VALUE =
    static_cast<int32_t>(A2UILinearGradientDirection::LEFT_TOP);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_BOTTOM_VALUE =
    static_cast<int32_t>(A2UILinearGradientDirection::LEFT_BOTTOM);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_TOP_VALUE =
    static_cast<int32_t>(A2UILinearGradientDirection::RIGHT_TOP);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_BOTTOM_VALUE =
    static_cast<int32_t>(A2UILinearGradientDirection::RIGHT_BOTTOM);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_NONE_VALUE = static_cast<int32_t>(A2UILinearGradientDirection::NONE);
constexpr int32_t A2UI_LINEAR_GRADIENT_DIRECTION_CUSTOM_VALUE =
    static_cast<int32_t>(A2UILinearGradientDirection::CUSTOM);

constexpr int32_t A2UI_SHADOW_TYPE_COLOR_VALUE = static_cast<int32_t>(A2UIShadowType::COLOR);
constexpr int32_t A2UI_SHADOW_TYPE_BLUR_VALUE = static_cast<int32_t>(A2UIShadowType::BLUR);

constexpr int32_t A2UI_SHADOW_STYLE_OUTER_DEFAULT_XS_VALUE = static_cast<int32_t>(A2UIShadowStyle::OUTER_DEFAULT_XS);
constexpr int32_t A2UI_SHADOW_STYLE_OUTER_DEFAULT_SM_VALUE = static_cast<int32_t>(A2UIShadowStyle::OUTER_DEFAULT_SM);
constexpr int32_t A2UI_SHADOW_STYLE_OUTER_DEFAULT_MD_VALUE = static_cast<int32_t>(A2UIShadowStyle::OUTER_DEFAULT_MD);
constexpr int32_t A2UI_SHADOW_STYLE_OUTER_DEFAULT_LG_VALUE = static_cast<int32_t>(A2UIShadowStyle::OUTER_DEFAULT_LG);
constexpr int32_t A2UI_SHADOW_STYLE_OUTER_FLOATING_SM_VALUE = static_cast<int32_t>(A2UIShadowStyle::OUTER_FLOATING_SM);
constexpr int32_t A2UI_SHADOW_STYLE_OUTER_FLOATING_MD_VALUE = static_cast<int32_t>(A2UIShadowStyle::OUTER_FLOATING_MD);

const std::unordered_map<std::string, int32_t> IMAGE_SIZE_MAP = { { "auto", A2UI_IMAGE_SIZE_AUTO_VALUE },
    { "cover", A2UI_IMAGE_SIZE_COVER_VALUE }, { "contain", A2UI_IMAGE_SIZE_CONTAIN_VALUE },
    { "fill", A2UI_IMAGE_SIZE_FILL_VALUE } };

const std::unordered_map<std::string, int32_t> LINEAR_GRADIENT_DIRECTION_MAP = {
    { "left", A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_VALUE }, { "top", A2UI_LINEAR_GRADIENT_DIRECTION_TOP_VALUE },
    { "right", A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_VALUE }, { "bottom", A2UI_LINEAR_GRADIENT_DIRECTION_BOTTOM_VALUE },
    { "lefttop", A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_TOP_VALUE },
    { "topleft", A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_TOP_VALUE },
    { "leftbottom", A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_BOTTOM_VALUE },
    { "bottomleft", A2UI_LINEAR_GRADIENT_DIRECTION_LEFT_BOTTOM_VALUE },
    { "righttop", A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_TOP_VALUE },
    { "topright", A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_TOP_VALUE },
    { "rightbottom", A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_BOTTOM_VALUE },
    { "bottomright", A2UI_LINEAR_GRADIENT_DIRECTION_RIGHT_BOTTOM_VALUE },
    { "none", A2UI_LINEAR_GRADIENT_DIRECTION_NONE_VALUE }, { "custom", A2UI_LINEAR_GRADIENT_DIRECTION_CUSTOM_VALUE }
};

const std::unordered_map<std::string, int32_t> SHADOW_STYLE_MAP = { { "outerdefaultxs",
                                                                        A2UI_SHADOW_STYLE_OUTER_DEFAULT_XS_VALUE },
    { "outerdefaultsm", A2UI_SHADOW_STYLE_OUTER_DEFAULT_SM_VALUE },
    { "outerdefaultmd", A2UI_SHADOW_STYLE_OUTER_DEFAULT_MD_VALUE },
    { "outerdefaultlg", A2UI_SHADOW_STYLE_OUTER_DEFAULT_LG_VALUE },
    { "outerfloatingsm", A2UI_SHADOW_STYLE_OUTER_FLOATING_SM_VALUE },
    { "outerfloatingmd", A2UI_SHADOW_STYLE_OUTER_FLOATING_MD_VALUE } };

bool ParseImageSizeToken(const std::string& value, int32_t& imageSize)
{
    std::string token = StyleApplyUtilsInternal::NormalizeNameToken(value);
    return StyleApplyUtilsInternal::TryFindMappedValue(IMAGE_SIZE_MAP, token, imageSize);
}

bool ParseLinearGradientDirectionToken(const std::string& value, int32_t& direction)
{
    std::string token = StyleApplyUtilsInternal::NormalizeNameToken(value);
    return StyleApplyUtilsInternal::TryFindMappedValue(LINEAR_GRADIENT_DIRECTION_MAP, token, direction);
}

bool ParseShadowStyleNumberValue(int32_t value, int32_t& shadowStyle)
{
    if (value < A2UI_SHADOW_STYLE_OUTER_DEFAULT_XS_VALUE || value > A2UI_SHADOW_STYLE_OUTER_FLOATING_MD_VALUE) {
        return false;
    }
    shadowStyle = value;
    return true;
}

bool ParseShadowStyleToken(const std::string& value, int32_t& shadowStyle)
{
    std::string token = StyleApplyUtilsInternal::NormalizeNameToken(value);
    return StyleApplyUtilsInternal::TryFindMappedValue(SHADOW_STYLE_MAP, token, shadowStyle);
}

bool ParseShadowTypeValue(const JsonValue& value, int32_t& shadowType);

bool ApplyShadowStyleValue(int32_t shadowStyle, StyleShadow& shadow)
{
    shadow.kind = StyleShadowKind::STYLE;
    shadow.style = shadowStyle;
    shadow.valid = true;
    return true;
}

bool ParseShadowFromNumberValue(const JsonValue& value, StyleShadow& shadow)
{
    constexpr double epsilon = 0.0001;
    double parsedValue = value.GetNumberValue(-1.0);
    if (!std::isfinite(parsedValue)) {
        return false;
    }

    int32_t parsedStyle = static_cast<int32_t>(std::lround(parsedValue));
    int32_t shadowStyle = 0;
    if (std::fabs(parsedValue - static_cast<double>(parsedStyle)) > epsilon ||
        !ParseShadowStyleNumberValue(parsedStyle, shadowStyle)) {
        return false;
    }
    return ApplyShadowStyleValue(shadowStyle, shadow);
}

bool ParseShadowFromStringValue(const JsonValue& value, StyleShadow& shadow)
{
    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    int32_t shadowStyle = 0;
    if (ParseShadowStyleToken(token, shadowStyle)) {
        return ApplyShadowStyleValue(shadowStyle, shadow);
    }

    float shadowStyleValue = 0.0F;
    if (!StyleApplyUtilsInternal::ParseFloatToken(token, shadowStyleValue)) {
        return false;
    }

    constexpr double epsilon = 0.0001;
    int32_t normalizedStyle = static_cast<int32_t>(std::lround(shadowStyleValue));
    if (std::fabs(shadowStyleValue - static_cast<float>(normalizedStyle)) > epsilon ||
        !ParseShadowStyleNumberValue(normalizedStyle, shadowStyle)) {
        return false;
    }
    return ApplyShadowStyleValue(shadowStyle, shadow);
}

bool ParseShadowFromObjectValue(const JsonValue& value, StyleShadow& shadow)
{
    if (value.Has("path") || value.Has("call")) {
        return false;
    }

    bool hasSupportedField = false;

    float parsedNumber = 0.0F;
    if (value.Has("radius")) {
        if (!StyleApplyUtils::ParseNumber(value.GetItem("radius"), parsedNumber)) {
            return false;
        }
        shadow.radius = std::max(0.0F, parsedNumber);
        hasSupportedField = true;
    }
    if (value.Has("offsetX")) {
        if (!StyleApplyUtils::ParseNumber(value.GetItem("offsetX"), shadow.offsetX)) {
            return false;
        }
        hasSupportedField = true;
    }
    if (value.Has("offsetY")) {
        if (!StyleApplyUtils::ParseNumber(value.GetItem("offsetY"), shadow.offsetY)) {
            return false;
        }
        hasSupportedField = true;
    }
    if (value.Has("type")) {
        if (!ParseShadowTypeValue(value.GetItem("type"), shadow.type)) {
            return false;
        }
        hasSupportedField = true;
    }
    if (value.Has("fill")) {
        if (!value.GetItem("fill").IsBool()) {
            return false;
        }
        shadow.fill = value.GetBool("fill", false);
        hasSupportedField = true;
    }
    if (value.Has("color")) {
        if (!StyleApplyUtils::ParseColor(value.GetItem("color"), shadow.color)) {
            return false;
        }
        shadow.hasColor = true;
        hasSupportedField = true;
    }

    if (!hasSupportedField) {
        return false;
    }

    shadow.kind = StyleShadowKind::CUSTOM;
    shadow.valid = true;
    return true;
}

bool ParseShadowTypeValue(const JsonValue& value, int32_t& shadowType)
{
    if (!value.IsValid()) {
        return true;
    }

    if (value.IsNumber()) {
        constexpr double epsilon = 0.0001;
        double parsedValue = value.GetNumberValue(static_cast<double>(A2UI_SHADOW_TYPE_COLOR_VALUE));
        if (!std::isfinite(parsedValue)) {
            return false;
        }
        int32_t parsedType = static_cast<int32_t>(std::lround(parsedValue));
        if (std::fabs(parsedValue - static_cast<double>(parsedType)) > epsilon) {
            return false;
        }
        if (parsedType < A2UI_SHADOW_TYPE_COLOR_VALUE || parsedType > A2UI_SHADOW_TYPE_BLUR_VALUE) {
            return false;
        }
        shadowType = parsedType;
        return true;
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtilsInternal::NormalizeNameToken(value.GetStringValue(""));
    static const std::unordered_map<std::string, int32_t> shadowTypeMap = { { "color", A2UI_SHADOW_TYPE_COLOR_VALUE },
        { "blur", A2UI_SHADOW_TYPE_BLUR_VALUE } };
    return StyleApplyUtilsInternal::TryFindMappedValue(shadowTypeMap, token, shadowType);
}

bool ParseAngleToken(const JsonValue& value, float& angle)
{
    if (value.IsNumber()) {
        angle = static_cast<float>(value.GetNumberValue(180.0));
        return true;
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    const float rightAngle = 90.0F;
    const float straightAngle = 180.0F;
    const float fullAngle = 360.0F;
    const float pi = 3.14159265358979323846F;
    float factor = 1.0F;
    if (token.size() > 3 && token.compare(token.size() - 3, 3, "deg") == 0) {
        token = StyleApplyUtils::TrimToken(token.substr(0, token.size() - 3));
    } else if (token.size() > 3 && token.compare(token.size() - 3, 3, "rad") == 0) {
        token = StyleApplyUtils::TrimToken(token.substr(0, token.size() - 3));
        factor = straightAngle / pi;
    } else if (token.size() > 4 && token.compare(token.size() - 4, 4, "grad") == 0) {
        token = StyleApplyUtils::TrimToken(token.substr(0, token.size() - 4));
        factor = rightAngle / 100.0F;
    } else if (token.size() > 4 && token.compare(token.size() - 4, 4, "turn") == 0) {
        token = StyleApplyUtils::TrimToken(token.substr(0, token.size() - 4));
        factor = fullAngle;
    }

    float parsed = 0.0F;
    if (!StyleApplyUtilsInternal::ParseFloatToken(token, parsed)) {
        return false;
    }
    angle = parsed * factor;
    return true;
}

bool ParseColorStop(const JsonValue& value, uint32_t& color, float& stop)
{
    if (StyleApplyUtils::ParseColor(value, color)) {
        return false;
    }
    if (value.IsArray()) {
        if (value.GetArraySize() < 2) {
            return false;
        }
        if (!StyleApplyUtils::ParseColor(value.GetArrayItem(0), color) ||
            !StyleApplyUtils::ParseNumber(value.GetArrayItem(1), stop)) {
            return false;
        }
        return true;
    }

    if (!value.IsObject()) {
        return false;
    }
    if (!StyleApplyUtils::ParseColor(value.GetItem("color"), color)) {
        return false;
    }
    JsonValue stopValue = value.GetItem("stop");
    if (!stopValue.IsValid()) {
        stopValue = value.GetItem("position");
    }
    return StyleApplyUtils::ParseNumber(stopValue, stop);
}

bool ParseGradientColorValue(const JsonValue& value, uint32_t& color)
{
    return StyleApplyUtils::ParseColor(value, color);
}

bool ParseGradientStopOverride(const JsonValue& value, float& stop)
{
    return StyleApplyUtils::ParseNumber(value, stop) && std::isfinite(stop);
}

float ClampStop(float stop)
{
    if (stop < 0.0F) {
        return 0.0F;
    }
    if (stop > 1.0F) {
        return 1.0F;
    }
    return stop;
}

bool TryParseImageSizeDimension(const JsonValue& value, float& parsedValue)
{
    StyleDimension dimension;
    if (!StyleApplyUtils::ParseDimension(value, dimension)) {
        return false;
    }

    switch (dimension.unit) {
        case StyleDimensionUnit::VP:
        case StyleDimensionUnit::FP:
        case StyleDimensionUnit::PERCENT:
            parsedValue = dimension.value;
            return std::isfinite(parsedValue) && parsedValue >= 0.0F;
        default:
            return false;
    }
}

} // namespace

bool StyleApplyUtils::ParseShadow(const JsonValue& value, StyleShadow& shadow)
{
    shadow = StyleShadow();
    switch (value.GetType()) {
        case JsonValueType::NUMBER:
            return ParseShadowFromNumberValue(value, shadow);
        case JsonValueType::STRING:
            return ParseShadowFromStringValue(value, shadow);
        case JsonValueType::OBJECT:
            return ParseShadowFromObjectValue(value, shadow);
        default:
            return false;
    }
}

bool StyleApplyUtils::ParseBackgroundImageSize(const JsonValue& value, StyleBackgroundImageSize& imageSize)
{
    imageSize = StyleBackgroundImageSize();
    if (!value.IsValid()) {
        return false;
    }
    if (value.IsString()) {
        int32_t parsedImageSize = 0;
        if (!ParseImageSizeToken(value.GetStringValue(""), parsedImageSize)) {
            return false;
        }
        imageSize.kind = StyleBackgroundImageSizeKind::IMAGE_SIZE;
        imageSize.imageSize = parsedImageSize;
        return true;
    }
    if (!value.IsObject() || value.Has("path") || value.Has("call")) {
        return false;
    }

    bool hasWidth = value.Has("width");
    bool hasHeight = value.Has("height");
    if (!hasWidth && !hasHeight) {
        return false;
    }

    float width = 0.0F;
    float height = 0.0F;
    if (hasWidth && !TryParseImageSizeDimension(value.GetItem("width"), width)) {
        return false;
    }
    if (hasHeight && !TryParseImageSizeDimension(value.GetItem("height"), height)) {
        return false;
    }

    imageSize.kind = StyleBackgroundImageSizeKind::SIZE;
    imageSize.width = width;
    imageSize.height = height;
    return true;
}

bool StyleApplyUtils::ParseLinearGradient(const JsonValue& value, StyleLinearGradient& gradient)
{
    gradient = StyleLinearGradient();
    gradient.direction = A2UI_LINEAR_GRADIENT_DIRECTION_NONE_VALUE;
    if (!value.IsObject() || value.Has("path") || value.Has("call")) {
        return false;
    }

    bool hasAngle = false;
    float parsedAngle = 180.0F;
    if (ParseAngleToken(value.GetItem("angle"), parsedAngle)) {
        gradient.angle = parsedAngle;
        gradient.direction = A2UI_LINEAR_GRADIENT_DIRECTION_CUSTOM_VALUE;
        hasAngle = true;
    }

    if (!hasAngle && value.Has("direction")) {
        int32_t parsedDirection = A2UI_LINEAR_GRADIENT_DIRECTION_BOTTOM_VALUE;
        if (!value.GetItem("direction").IsString() ||
            !ParseLinearGradientDirectionToken(value.GetString("direction", ""), parsedDirection)) {
            return false;
        }
        gradient.direction = parsedDirection;
    }

    if (value.Has("repeating")) {
        if (!value.GetItem("repeating").IsBool()) {
            return false;
        }
        gradient.repeating = value.GetBool("repeating", false);
    }

    JsonValue colors = value.GetItem("colors");
    if (!colors.IsArray()) {
        return false;
    }

    std::vector<float> stopOverrides(static_cast<size_t>(colors.GetArraySize()), 0.0F);
    std::vector<bool> hasStopOverrides(static_cast<size_t>(colors.GetArraySize()), false);
    JsonValue stops = value.GetItem("stops");
    if (stops.IsArray()) {
        int stopCount = std::min(colors.GetArraySize(), stops.GetArraySize());
        for (int index = 0; index < stopCount; ++index) {
            float parsedStop = 0.0F;
            if (ParseGradientStopOverride(stops.GetArrayItem(index), parsedStop)) {
                stopOverrides[static_cast<size_t>(index)] = parsedStop;
                hasStopOverrides[static_cast<size_t>(index)] = true;
            }
        }
    }

    float previousStop = 0.0F;
    int totalColors = colors.GetArraySize();
    for (int index = 0; index < colors.GetArraySize(); ++index) {
        uint32_t color = 0;
        float stop = 0.0F;
        JsonValue colorValue = colors.GetArrayItem(index);
        if (!ParseColorStop(colorValue, color, stop)) {
            if (!ParseGradientColorValue(colorValue, color)) {
                continue;
            }
            if (hasStopOverrides[static_cast<size_t>(index)]) {
                stop = stopOverrides[static_cast<size_t>(index)];
            } else {
                stop = totalColors <= 1 ? 0.0F : static_cast<float>(index) / static_cast<float>(totalColors - 1);
            }
        }
        stop = ClampStop(stop);
        if (!gradient.stops.empty() && stop < previousStop) {
            stop = previousStop;
        }
        gradient.colors.push_back(color);
        gradient.stops.push_back(stop);
        previousStop = stop;
    }

    return !gradient.colors.empty() && gradient.colors.size() == gradient.stops.size();
}

bool StyleApplyUtils::ParseFlexShrink(const JsonValue& value, float& flexShrink)
{
    float parsed = 0.0F;
    if (!ParseNumber(value, parsed) || !std::isfinite(parsed) || parsed < 0.0F || parsed > 1.0F) {
        return false;
    }
    flexShrink = parsed;
    return true;
}

bool StyleApplyUtils::ParseBackgroundImage(const JsonValue& value, std::string& backgroundImage)
{
    if (!value.IsString()) {
        return false;
    }
    backgroundImage = TrimToken(value.GetStringValue(""));
    return true;
}

bool StyleApplyUtils::ParseClip(const JsonValue& value, bool& clip)
{
    if (!value.IsBool()) {
        return false;
    }
    clip = value.GetBoolValue(false);
    return true;
}

} // namespace NativeModule
