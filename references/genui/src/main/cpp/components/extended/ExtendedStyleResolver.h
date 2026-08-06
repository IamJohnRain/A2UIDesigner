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

#ifndef A2UI_EXTENDED_STYLE_RESOLVER_H
#define A2UI_EXTENDED_STYLE_RESOLVER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "styles/StyleTypes.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "ExtendedDescriptorNormalizer.h"

namespace NativeModule {

class ExtendedCommonTheme;

struct ConstraintDispatchContext {
    int32_t renderId = -1;
    std::string componentId;
    int32_t nodeUniqueId = -1;
    std::string componentType;
    std::string parentComponentType;
    int32_t apiVersion = 0;
    std::shared_ptr<ExtendedCommonTheme> commonTheme;
};

class ExtendedStyleResolver final {
public:
    static void ResolveAndApply(const JsonValue& styles, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static void ResolveAndApply(const JsonValue& styles, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext = std::nullopt);
    static void ApplyTextComponentStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier);
    static void Reset(const StyleResetProperty& property, ArkUINodeApiAdapter& applier,
        int32_t apiVersion = MIN_API_VERSION_LAYOUT_POLICY, const std::string& parentComponentType = "");
    static bool ParseColor(const JsonValue& value, uint32_t& color);
    static void ApplyShadow(const JsonValue& value, ArkUINodeApiAdapter& applier,
        std::vector<DescriptorValidationIssue>& issues, std::shared_ptr<ExtendedCommonTheme> commonTheme);

private:
    static void ApplySizeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
        std::vector<DescriptorValidationIssue>& issues,
        std::optional<ConstraintDispatchContext> dispatchContext = std::nullopt);
    static void ApplyColorStyles(
        const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyTextStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier);
    static void ApplyTextFontStyles(const JsonValue& styles, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier);
    static void ApplyTextLineStyles(const JsonValue& styles, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier);
    static void ApplyEdgeStyles(
        const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyPaddingStyles(
        const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyMarginStyles(
        const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyDecorationStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyBorderWidthStyle(
        const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyOpacityStyle(const JsonValue& styles, ArkUINodeApiAdapter& applier);
    static void ApplyVisibilityStyle(
        const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyBackgroundImageSizeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyBackgroundImageSize(const JsonValue& value, const char* propertyName, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyLinearGradient(
        const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyCommonNodeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyFlexShrinkStyle(const JsonValue& value, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyClipStyle(
        const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyLayoutWeightStyle(
        const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyConstraintSizeStyle(const JsonValue& value, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyBackgroundImage(const char* propertyName, const JsonValue& value, ArkUINodeApiAdapter& applier,
        std::vector<DescriptorValidationIssue>& issues);
    static void ApplyTextDecoration(const JsonValue& value, ArkUINodeApiAdapter& applier);
    static bool ParseEdgeStyle(const JsonValue& styles, const char* allKey, const char* topKey, const char* rightKey,
        const char* bottomKey, const char* leftKey, StyleEdge& edge);
    static bool DimensionToFloat(const StyleDimension& dimension, float& value);
    static void ApplyDimension(const JsonValue& value, ArkUINodeApiAdapter& applier, bool isWidth,
        std::vector<DescriptorValidationIssue>& issues,
        std::optional<ConstraintDispatchContext> dispatchContext = std::nullopt);
    static void ApplyAspectRatio(
        const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues);
    static void ApplyRadius(const JsonValue& value, ArkUINodeApiAdapter& applier,
        std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues);
    static bool ResetLayoutProperty(
        StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier, int32_t apiVersion);
    static bool ResetTextProperty(
        StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier);
    static bool ResetCommonProperty(StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle,
        ArkUINodeApiAdapter& applier, const std::string& parentComponentType);
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_STYLE_RESOLVER_H
