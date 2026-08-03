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

#ifndef A2UI_STYLE_APPLY_UTILS_H
#define A2UI_STYLE_APPLY_UTILS_H

#include <cstdint>
#include <string>

#include "adapter/A2UIArkUITypes.h"
#include "utils/JsonAdapter.h"

#include "StyleTypes.h"

namespace NativeModule {

class StyleApplyUtils final {
public:
    static std::string TrimToken(const std::string& value);
    static bool ParseColor(const JsonValue& value, uint32_t& color);
    static bool ParseHexColorString(const std::string& value, uint32_t& color);
    static bool ParseNumber(const JsonValue& value, float& number);
    static bool ParseDividerStrokeWidth(const JsonValue& value, float& strokeWidth, std::string& unit);
    static bool ParseDimension(const JsonValue& value, StyleDimension& dimension);
    static bool ParseEdge(const JsonValue& value, StyleEdge& edge);
    static bool ParseRadius(const JsonValue& value, StyleRadius& radius);
    static bool ParseShadow(const JsonValue& value, StyleShadow& shadow);
    static bool ParseBackgroundImageSize(const JsonValue& value, StyleBackgroundImageSize& imageSize);
    static bool ParseLinearGradient(const JsonValue& value, StyleLinearGradient& gradient);
    static bool ParseFontWeight(const JsonValue& value, int32_t& fontWeight);
    static bool ParseMaxLines(const JsonValue& value, int32_t& maxLines);
    static bool ParseTextOverflow(const JsonValue& value, int32_t& textOverflow);
    static bool ParseTextAlign(const JsonValue& value, int32_t& textAlign);
    static bool ParseWordBreak(const JsonValue& value, int32_t& wordBreak);
    static bool ParseProgressType(const JsonValue& value, int32_t& progressType);
    static bool ParseTextDecoration(const JsonValue& value, StyleTextDecoration& decoration);
    static bool ParseFlexShrink(const JsonValue& value, float& flexShrink);
    static bool ParseBackgroundImage(const JsonValue& value, std::string& backgroundImage);
    static bool ParseClip(const JsonValue& value, bool& clip);
    static bool ParseVisibility(const JsonValue& value, A2UIVisibility& visibility);
    static bool IsExpressionString(const std::string& value);

private:
    static bool ParseEdgeObject(const JsonValue& value, StyleEdge& edge);
    static bool ParseEdgeShorthand(const std::string& value, StyleEdge& edge);
    static bool ParseRadiusObject(const JsonValue& value, StyleRadius& radius);
    static bool ParseRadiusShorthand(const std::string& value, StyleRadius& radius);
};

} // namespace NativeModule

#endif // A2UI_STYLE_APPLY_UTILS_H
