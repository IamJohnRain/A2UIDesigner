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

#include "StyleParser.h"

#include <unordered_map>

#include "StyleApplyUtils.h"

namespace NativeModule {

namespace {

const std::unordered_map<std::string, StylePropertyName>& GetStylePropertyNameMap()
{
    static const std::unordered_map<std::string, StylePropertyName> propertyNameMap = { { "width",
                                                                                            StylePropertyName::WIDTH },
        { "height", StylePropertyName::HEIGHT }, { "padding", StylePropertyName::PADDING },
        { "paddingTop", StylePropertyName::PADDING }, { "paddingRight", StylePropertyName::PADDING },
        { "paddingBottom", StylePropertyName::PADDING }, { "paddingLeft", StylePropertyName::PADDING },
        { "margin", StylePropertyName::MARGIN }, { "marginTop", StylePropertyName::MARGIN },
        { "marginRight", StylePropertyName::MARGIN }, { "marginBottom", StylePropertyName::MARGIN },
        { "marginLeft", StylePropertyName::MARGIN }, { "backgroundColor", StylePropertyName::BACKGROUND_COLOR },
        { "borderRadius", StylePropertyName::BORDER_RADIUS }, { "borderWidth", StylePropertyName::BORDER_WIDTH },
        { "borderColor", StylePropertyName::BORDER_COLOR }, { "fontColor", StylePropertyName::FONT_COLOR },
        { "fontSize", StylePropertyName::FONT_SIZE }, { "fontWeight", StylePropertyName::FONT_WEIGHT },
        { "textAlign", StylePropertyName::TEXT_ALIGN }, { "maxLines", StylePropertyName::MAX_LINES },
        { "minFontSize", StylePropertyName::TEXT_MIN_FONT_SIZE },
        { "maxFontSize", StylePropertyName::TEXT_MAX_FONT_SIZE }, { "textOverflow", StylePropertyName::TEXT_OVERFLOW },
        { "wordBreak", StylePropertyName::WORD_BREAK }, { "decoration", StylePropertyName::DECORATION },
        { "placeholderColor", StylePropertyName::PLACEHOLDER_COLOR }, { "caretColor", StylePropertyName::CARET_COLOR },
        { "showUnderline", StylePropertyName::SHOW_UNDERLINE }, { "visibility", StylePropertyName::VISIBILITY },
        { "opacity", StylePropertyName::OPACITY }, { "shadow", StylePropertyName::SHADOW },
        { "flexShrink", StylePropertyName::FLEX_SHRINK }, { "backgroundImage", StylePropertyName::BACKGROUND_IMAGE },
        { "backgroundimage", StylePropertyName::BACKGROUND_IMAGE },
        { "backgroundImageSizeWithStyle", StylePropertyName::BACKGROUND_IMAGE_SIZE },
        { "backgroundimageSizeWithStyle", StylePropertyName::BACKGROUND_IMAGE_SIZE },
        { "clip", StylePropertyName::CLIP }, { "layoutWeight", StylePropertyName::LAYOUT_WEIGHT },
        { "constraintSize", StylePropertyName::CONSTRAINT_SIZE } };
    return propertyNameMap;
}

} // namespace

StyleParseResult StyleParser::Parse(const JsonValue& styles)
{
    StyleParseResult result;
    result.success = true;

    if (!styles.IsValid() || styles.IsNull()) {
        return result;
    }

    if (!styles.IsObject()) {
        result.success = false;
        result.errors.push_back(
            { .code = StyleErrorCode::INVALID_STYLES, .property = "", .message = "styles must be an object" });
        return result;
    }

    for (JsonValue child = styles.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string rawName = child.GetKey();
        if (rawName.empty()) {
            continue;
        }

        StyleProperty property;
        property.rawName = rawName;
        property.name = ToPropertyName(rawName);
        property.kind = InferValueKind(property.name, child);
        property.rawValue = child;
        result.properties.push_back(property);
    }

    return result;
}

StylePropertyName StyleParser::ToPropertyName(const std::string& rawName)
{
    const auto& propertyNameMap = GetStylePropertyNameMap();
    auto iter = propertyNameMap.find(rawName);
    if (iter != propertyNameMap.end()) {
        return iter->second;
    }
    if (rawName == "backgroundImageSize" || rawName == "backgroundimageSize" ||
        rawName == "backgroundImageSizeWithStyle" || rawName == "backgroundimageSizeWithStyle") {
        return StylePropertyName::BACKGROUND_IMAGE_SIZE;
    }
    if (rawName == "linearGradient") {
        return StylePropertyName::LINEAR_GRADIENT;
    }
    return StylePropertyName::UNKNOWN;
}

bool StyleParser::IsCompositeProperty(StylePropertyName propertyName)
{
    switch (propertyName) {
        case StylePropertyName::PADDING:
        case StylePropertyName::MARGIN:
        case StylePropertyName::BORDER_RADIUS:
        case StylePropertyName::SHADOW:
        case StylePropertyName::BACKGROUND_IMAGE_SIZE:
        case StylePropertyName::LINEAR_GRADIENT:
        case StylePropertyName::DECORATION:
        case StylePropertyName::CONSTRAINT_SIZE:
            return true;
        default:
            return false;
    }
}

StyleValueKind StyleParser::InferValueKind(StylePropertyName propertyName, const JsonValue& value)
{
    if (!value.IsValid()) {
        return StyleValueKind::INVALID;
    }

    if (value.IsObject()) {
        if (value.Has("path")) {
            return StyleValueKind::PATH_BINDING;
        }
        if (value.Has("call")) {
            return StyleValueKind::FUNCTION_CALL;
        }
        if (IsCompositeProperty(propertyName) || propertyName == StylePropertyName::UNKNOWN) {
            return StyleValueKind::COMPOSITE_OBJECT;
        }
        return StyleValueKind::STATIC_VALUE;
    }

    if (value.IsString() && StyleApplyUtils::IsExpressionString(value.GetStringValue(""))) {
        return StyleValueKind::EXPRESSION;
    }

    return StyleValueKind::STATIC_VALUE;
}

} // namespace NativeModule
