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

#include <cmath>
#include <limits>
#include <unordered_map>

#include "StyleApplyUtilsInternal.h"

namespace NativeModule {

namespace {

constexpr int32_t A2UI_FONT_WEIGHT_W100_VALUE = static_cast<int32_t>(A2UIFontWeight::W100);
constexpr int32_t A2UI_FONT_WEIGHT_W200_VALUE = static_cast<int32_t>(A2UIFontWeight::W200);
constexpr int32_t A2UI_FONT_WEIGHT_W300_VALUE = static_cast<int32_t>(A2UIFontWeight::W300);
constexpr int32_t A2UI_FONT_WEIGHT_W400_VALUE = static_cast<int32_t>(A2UIFontWeight::W400);
constexpr int32_t A2UI_FONT_WEIGHT_W500_VALUE = static_cast<int32_t>(A2UIFontWeight::W500);
constexpr int32_t A2UI_FONT_WEIGHT_W600_VALUE = static_cast<int32_t>(A2UIFontWeight::W600);
constexpr int32_t A2UI_FONT_WEIGHT_W700_VALUE = static_cast<int32_t>(A2UIFontWeight::W700);
constexpr int32_t A2UI_FONT_WEIGHT_W800_VALUE = static_cast<int32_t>(A2UIFontWeight::W800);
constexpr int32_t A2UI_FONT_WEIGHT_W900_VALUE = static_cast<int32_t>(A2UIFontWeight::W900);
constexpr int32_t A2UI_FONT_WEIGHT_BOLD_VALUE = static_cast<int32_t>(A2UIFontWeight::BOLD);
constexpr int32_t A2UI_FONT_WEIGHT_NORMAL_VALUE = static_cast<int32_t>(A2UIFontWeight::NORMAL);
constexpr int32_t A2UI_FONT_WEIGHT_BOLDER_VALUE = static_cast<int32_t>(A2UIFontWeight::BOLDER);
constexpr int32_t A2UI_FONT_WEIGHT_LIGHTER_VALUE = static_cast<int32_t>(A2UIFontWeight::LIGHTER);
constexpr int32_t A2UI_FONT_WEIGHT_MEDIUM_VALUE = static_cast<int32_t>(A2UIFontWeight::MEDIUM);
constexpr int32_t A2UI_FONT_WEIGHT_REGULAR_VALUE = static_cast<int32_t>(A2UIFontWeight::REGULAR);

const std::unordered_map<std::string, int32_t> FONT_WEIGHT_MAP = { { "bold", A2UI_FONT_WEIGHT_BOLD_VALUE },
    { "normal", A2UI_FONT_WEIGHT_NORMAL_VALUE }, { "bolder", A2UI_FONT_WEIGHT_BOLDER_VALUE },
    { "lighter", A2UI_FONT_WEIGHT_LIGHTER_VALUE }, { "medium", A2UI_FONT_WEIGHT_MEDIUM_VALUE },
    { "regular", A2UI_FONT_WEIGHT_REGULAR_VALUE } };

const std::unordered_map<std::string, int32_t> WORD_BREAK_MAP = { { "normal",
                                                                      static_cast<int32_t>(A2UIWordBreak::NORMAL) },
    { "breakall", static_cast<int32_t>(A2UIWordBreak::BREAK_ALL) },
    { "breakword", static_cast<int32_t>(A2UIWordBreak::BREAK_WORD) },
    { "hyphenation", static_cast<int32_t>(A2UIWordBreak::HYPHENATION) } };

const std::unordered_map<std::string, int32_t> PROGRESS_TYPE_MAP = { { "linear", 0 }, { "line", 0 }, { "ring", 1 },
    { "eclipse", 2 }, { "scalering", 3 }, { "capsule", 4 } };

bool ParseDecorationTypeToken(const std::string& value, int32_t& type)
{
    std::string token = StyleApplyUtilsInternal::ToLowerToken(value);
    static const std::unordered_map<std::string, int32_t> decorationTypeMap = { { "none", 0 }, { "underline", 1 },
        { "overline", 2 }, { "linethrough", 3 } };
    return StyleApplyUtilsInternal::TryFindMappedValue(decorationTypeMap, token, type);
}

bool ParseDecorationStyleToken(const std::string& value, int32_t& style)
{
    std::string token = StyleApplyUtilsInternal::ToLowerToken(value);
    static const std::unordered_map<std::string, int32_t> decorationStyleMap = { { "solid", 0 }, { "double", 1 },
        { "dotted", 2 }, { "dashed", 3 }, { "wavy", 4 } };
    return StyleApplyUtilsInternal::TryFindMappedValue(decorationStyleMap, token, style);
}

bool ParseFontWeightNumberValue(double value, int32_t& fontWeight)
{
    constexpr int32_t minFontWeight = 100;
    constexpr int32_t maxFontWeight = 900;
    constexpr int32_t fontWeightStep = 100;
    constexpr double epsilon = 0.0001;

    if (!std::isfinite(value)) {
        fontWeight = A2UI_FONT_WEIGHT_W400_VALUE;
        return true;
    }

    int32_t normalized = static_cast<int32_t>(std::lround(value));
    if (std::fabs(value - static_cast<double>(normalized)) > epsilon || normalized < minFontWeight ||
        normalized > maxFontWeight || normalized % fontWeightStep != 0) {
        fontWeight = A2UI_FONT_WEIGHT_W400_VALUE;
        return true;
    }

    fontWeight = (normalized / fontWeightStep) - 1;
    return true;
}

bool ParseFontWeightKeyword(const std::string& value, int32_t& fontWeight)
{
    std::string token = StyleApplyUtilsInternal::ToLowerToken(value);
    auto iter = FONT_WEIGHT_MAP.find(token);
    if (iter != FONT_WEIGHT_MAP.end()) {
        fontWeight = iter->second;
        return true;
    }
    return false;
}

} // namespace

bool StyleApplyUtils::ParseFontWeight(const JsonValue& value, int32_t& fontWeight)
{
    if (!value.IsValid()) {
        return false;
    }
    if (value.IsNumber()) {
        return ParseFontWeightNumberValue(value.GetNumberValue(400.0), fontWeight);
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = TrimToken(value.GetStringValue(""));
    if (ParseFontWeightKeyword(token, fontWeight)) {
        return true;
    }

    float parsedNumber = 0.0F;
    if (StyleApplyUtilsInternal::ParseFloatToken(token, parsedNumber)) {
        return ParseFontWeightNumberValue(parsedNumber, fontWeight);
    }

    fontWeight = A2UI_FONT_WEIGHT_W400_VALUE;
    return true;
}

bool StyleApplyUtils::ParseMaxLines(const JsonValue& value, int32_t& maxLines)
{
    if (!value.IsNumber() && !value.IsString()) {
        return false;
    }

    double rawValue = value.ToNumber(std::numeric_limits<double>::quiet_NaN());
    if (!std::isfinite(rawValue) || rawValue < 0.0 ||
        rawValue > static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return false;
    }
    maxLines = static_cast<int32_t>(rawValue);
    return rawValue == 0.0 || maxLines > 0;
}

bool StyleApplyUtils::ParseTextOverflow(const JsonValue& value, int32_t& textOverflow)
{
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtilsInternal::ToLowerToken(value.GetStringValue(""));
    static const std::unordered_map<std::string, int32_t> textOverflowMap = { { "none", 0 }, { "clip", 1 },
        { "ellipsis", 2 }, { "marquee", 3 } };
    return StyleApplyUtilsInternal::TryFindMappedValue(textOverflowMap, token, textOverflow);
}

bool StyleApplyUtils::ParseTextAlign(const JsonValue& value, int32_t& textAlign)
{
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtilsInternal::ToLowerToken(value.GetStringValue(""));
    static const std::unordered_map<std::string, int32_t> textAlignMap = { { "start", 0 }, { "center", 1 },
        { "end", 2 }, { "justify", 3 } };
    return StyleApplyUtilsInternal::TryFindMappedValue(textAlignMap, token, textAlign);
}

bool StyleApplyUtils::ParseWordBreak(const JsonValue& value, int32_t& wordBreak)
{
    if (!value.IsString()) {
        return false;
    }
    return StyleApplyUtilsInternal::TryFindMappedValue(
        WORD_BREAK_MAP, StyleApplyUtilsInternal::NormalizeNameToken(value.GetStringValue("")), wordBreak);
}

bool StyleApplyUtils::ParseProgressType(const JsonValue& value, int32_t& progressType)
{
    if (!value.IsString()) {
        return false;
    }
    return StyleApplyUtilsInternal::TryFindMappedValue(
        PROGRESS_TYPE_MAP, StyleApplyUtilsInternal::NormalizeNameToken(value.GetStringValue("")), progressType);
}

bool StyleApplyUtils::ParseTextDecoration(const JsonValue& value, StyleTextDecoration& decoration)
{
    if (!value.IsObject() || value.Has("path") || value.Has("call")) {
        return false;
    }

    StyleTextDecoration nextDecoration;
    JsonValue typeValue = value.GetItem("type");
    if (typeValue.IsValid()) {
        if (!typeValue.IsString()) {
            return false;
        }
        if (!ParseDecorationTypeToken(value.GetString("type", ""), nextDecoration.type)) {
            return false;
        }
    }

    uint32_t parsedColor = 0;
    if (ParseColor(value.GetItem("color"), parsedColor)) {
        nextDecoration.color = parsedColor;
        nextDecoration.hasColor = true;
    }

    int32_t parsedStyle = 0;
    if (value.GetItem("style").IsString() && ParseDecorationStyleToken(value.GetString("style", ""), parsedStyle)) {
        nextDecoration.style = parsedStyle;
        nextDecoration.hasStyle = true;
    } else if (!value.GetItem("style").IsValid()) {
        nextDecoration.style = 0;
        nextDecoration.hasStyle = true;
    }

    float parsedThicknessScale = 0.0F;
    if (ParseNumber(value.GetItem("thicknessScale"), parsedThicknessScale)) {
        nextDecoration.thicknessScale = parsedThicknessScale;
        nextDecoration.hasThicknessScale = true;
    } else if (!value.GetItem("thicknessScale").IsValid()) {
        nextDecoration.thicknessScale = 1.0F;
        nextDecoration.hasThicknessScale = true;
    }

    decoration = nextDecoration;
    return true;
}

bool StyleApplyUtils::ParseVisibility(const JsonValue& value, A2UIVisibility& visibility)
{
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtilsInternal::ToLowerToken(value.GetStringValue(""));
    static const std::unordered_map<std::string, A2UIVisibility> visibilityMap = {
        { "visible", A2UIVisibility::VISIBLE }, { "hidden", A2UIVisibility::HIDDEN }, { "none", A2UIVisibility::NONE }
    };
    return StyleApplyUtilsInternal::TryFindMappedValue(visibilityMap, token, visibility);
}

} // namespace NativeModule
