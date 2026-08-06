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

#ifndef A2UI_STYLE_TYPES_H
#define A2UI_STYLE_TYPES_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "utils/JsonAdapter.h"

#include "StyleErrors.h"

namespace NativeModule {

enum class StyleValueKind { STATIC_VALUE = 0, EXPRESSION, PATH_BINDING, FUNCTION_CALL, COMPOSITE_OBJECT, INVALID };

enum class StylePropertyName {
    WIDTH = 0,
    HEIGHT,
    PADDING,
    MARGIN,
    BACKGROUND_COLOR,
    BORDER_RADIUS,
    BORDER_WIDTH,
    BORDER_COLOR,
    FONT_COLOR,
    FONT_SIZE,
    FONT_WEIGHT,
    TEXT_ALIGN,
    MAX_LINES,
    TEXT_MIN_FONT_SIZE,
    TEXT_MAX_FONT_SIZE,
    TEXT_OVERFLOW,
    WORD_BREAK,
    DECORATION,
    FLEX_SHRINK,
    BACKGROUND_IMAGE,
    BACKGROUND_IMAGE_SIZE,
    LINEAR_GRADIENT,
    CLIP,
    PLACEHOLDER_COLOR,
    CARET_COLOR,
    SHOW_UNDERLINE,
    VISIBILITY,
    OPACITY,
    SHADOW,
    LAYOUT_WEIGHT,
    CONSTRAINT_SIZE,
    ASPECT_RATIO,
    UNKNOWN
};

enum class StyleDimensionUnit { VP = 0, FP, PERCENT, MATCH_PARENT, WRAP_CONTENT, FIX_AT_IDEAL_SIZE, INVALID };

struct StyleDimension {
    StyleDimensionUnit unit = StyleDimensionUnit::INVALID;
    float value = 0.0F;
};

struct StyleEdge {
    StyleDimension top;
    StyleDimension right;
    StyleDimension bottom;
    StyleDimension left;
};

struct StyleRadius {
    StyleDimension topLeft;
    StyleDimension topRight;
    StyleDimension bottomRight;
    StyleDimension bottomLeft;
};

enum class StyleShadowKind { STYLE = 0, CUSTOM };

struct StyleShadow {
    StyleShadowKind kind = StyleShadowKind::CUSTOM;
    int32_t style = 0;
    float radius = 0.0F;
    bool useColorStrategy = false;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    int32_t type = 0;
    uint32_t color = 0xFF000000;
    bool hasColor = false;
    bool fill = false;
    bool valid = false;
};

struct StyleTextDecoration {
    int32_t type = 0;
    uint32_t color = 0;
    int32_t style = 0;
    float thicknessScale = 0.0F;
    bool hasColor = false;
    bool hasStyle = false;
    bool hasThicknessScale = false;
};

enum class StyleBackgroundImageSizeKind { SIZE = 0, IMAGE_SIZE };

struct StyleBackgroundImageSize {
    StyleBackgroundImageSizeKind kind = StyleBackgroundImageSizeKind::SIZE;
    float width = 0.0F;
    float height = 0.0F;
    int32_t imageSize = 0;
};

struct StyleLinearGradient {
    float angle = 180.0F;
    int32_t direction = 0;
    bool repeating = false;
    std::vector<uint32_t> colors;
    std::vector<float> stops;
};

struct StyleProperty {
    std::string rawName;
    StylePropertyName name = StylePropertyName::UNKNOWN;
    StyleValueKind kind = StyleValueKind::INVALID;
    JsonValue rawValue;
};

struct StyleParseResult {
    bool success = false;
    std::vector<StyleProperty> properties;
    std::vector<StyleError> errors;
};

enum class StyleBindingKind { PATH = 0, FUNCTION_CALL, EXPRESSION };

struct StyleBindingPlan {
    std::string bindingProperty;
    std::string dataPath;
    StyleProperty property;
    StyleBindingKind kind = StyleBindingKind::PATH;
    JsonValue functionCallDescriptor;
    std::string expression;
    std::vector<std::string> globalVarDeps;
};

struct StyleResetProperty {
    std::string rawName;
    StylePropertyName name = StylePropertyName::UNKNOWN;
};

struct StyleResolveResult {
    bool success = false;
    std::unique_ptr<JsonAdapter> resolvedAdapter;
    JsonValue resolvedStyles;
    std::set<std::string> currentStyleKeys;
    std::set<std::string> dynamicallyResolvedStyleKeys;
    std::vector<std::string> clearBindingProperties;
    std::vector<StyleBindingPlan> bindings;
    std::vector<StyleResetProperty> resetProperties;
    std::vector<StyleError> errors;
};

} // namespace NativeModule

#endif // A2UI_STYLE_TYPES_H
