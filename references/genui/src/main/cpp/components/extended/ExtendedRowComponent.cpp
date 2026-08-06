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

#include "ExtendedRowComponent.h"

#include <cmath>

#include "components/ChildListSchemaValidationUtils.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

A2UIFlexAlignment ResolveRowJustify(const std::string& justify)
{
    if (justify == "center") {
        return A2UIFlexAlignment::CENTER;
    }
    if (justify == "end") {
        return A2UIFlexAlignment::END;
    }
    if (justify == "spaceAround") {
        return A2UIFlexAlignment::SPACE_AROUND;
    }
    if (justify == "spaceBetween") {
        return A2UIFlexAlignment::SPACE_BETWEEN;
    }
    if (justify == "spaceEvenly") {
        return A2UIFlexAlignment::SPACE_EVENLY;
    }
    return A2UIFlexAlignment::START;
}

A2UIItemAlignment ResolveRowAlign(const std::string& align)
{
    if (align == "top") {
        return A2UIItemAlignment::START;
    }
    if (align == "center") {
        return A2UIItemAlignment::CENTER;
    }
    if (align == "bottom") {
        return A2UIItemAlignment::END;
    }
    return A2UIItemAlignment::CENTER;
}

A2UIFlexWrap ResolveRowWrap(const std::string& wrap)
{
    if (wrap == "wrap") {
        return A2UIFlexWrap::WRAP;
    }
    return A2UIFlexWrap::NO_WRAP;
}

float NormalizeItemMargin(float itemMargin, float defaultItemMargin)
{
    return std::isfinite(itemMargin) && itemMargin >= 0.0F ? itemMargin : defaultItemMargin;
}

bool IsItemMarginDisabledByJustify(A2UIFlexAlignment alignment)
{
    return alignment == A2UIFlexAlignment::SPACE_AROUND || alignment == A2UIFlexAlignment::SPACE_BETWEEN ||
           alignment == A2UIFlexAlignment::SPACE_EVENLY;
}

} // namespace

ExtendedRowComponent::ExtendedRowComponent() : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::FLEX)) {}

std::string ExtendedRowComponent::GetType() const
{
    return "Row";
}

void ExtendedRowComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    topLevelAlignItems_ = DEFAULT_ALIGN_ITEMS;
    topLevelJustifyContent_ = DEFAULT_JUSTIFY_CONTENT;
    topLevelWrap_ = DEFAULT_WRAP;
    hasStyleAlignItems_ = false;
    hasStyleJustifyContent_ = false;
    hasStyleWrap_ = false;
    ApplyEffectiveLayout();
    ApplyDeclaredPropertyOrFallback(descriptor, "itemMargin");
    ApplyDeclaredPropertyOrFallback(descriptor, "justifyContent");
    ApplyDeclaredPropertyOrFallback(descriptor, "alignItems");
    ApplyDeclaredPropertyOrFallback(descriptor, "wrap");
}

PropertyDeclaration ExtendedRowComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    if (propertyName == "itemMargin") {
        return CreateItemMarginPropertyDeclaration();
    }
    if (propertyName == "justifyContent") {
        return CreateJustifyContentPropertyDeclaration();
    }
    if (propertyName == "alignItems") {
        return CreateAlignItemsPropertyDeclaration();
    }
    if (propertyName == "wrap") {
        return CreateWrapPropertyDeclaration();
    }
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
}

PropertyDeclaration ExtendedRowComponent::CreateItemMarginPropertyDeclaration()
{
    return PropertyDeclaration { .name = "itemMargin",
        .type = PropertyValueType::NUMBER,
        .allowDynamic = true,
        .allowExpression = true,
        .fallbackNumber = DEFAULT_ITEM_MARGIN,
        .applyValue = [this](const JsonValue& value) {
            SetItemMargin(static_cast<float>(value.GetNumberValue(DEFAULT_ITEM_MARGIN)));
        } };
}

PropertyDeclaration ExtendedRowComponent::CreateJustifyContentPropertyDeclaration()
{
    PropertyDeclaration declaration;
    declaration.name = "justifyContent";
    declaration.type = PropertyValueType::ENUM_STRING;
    declaration.allowDynamic = true;
    declaration.allowExpression = true;
    declaration.fallbackString = "start";
    declaration.enumFallback = "start";
    declaration.enumAllowed = { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" };
    declaration.applyValue = [this](const JsonValue& value) {
        SetTopLevelJustifyContent(ResolveRowJustify(value.GetStringValue("start")));
    };
    return declaration;
}

PropertyDeclaration ExtendedRowComponent::CreateAlignItemsPropertyDeclaration()
{
    PropertyDeclaration declaration;
    declaration.name = "alignItems";
    declaration.type = PropertyValueType::ENUM_STRING;
    declaration.allowDynamic = true;
    declaration.allowExpression = true;
    declaration.fallbackString = "center";
    declaration.enumFallback = "center";
    declaration.enumAllowed = { "top", "center", "bottom" };
    declaration.applyValue = [this](const JsonValue& value) {
        SetTopLevelAlignItems(ResolveRowAlign(value.GetStringValue("center")));
    };
    return declaration;
}

PropertyDeclaration ExtendedRowComponent::CreateWrapPropertyDeclaration()
{
    PropertyDeclaration declaration;
    declaration.name = "wrap";
    declaration.type = PropertyValueType::ENUM_STRING;
    declaration.allowDynamic = true;
    declaration.allowExpression = true;
    declaration.fallbackString = "noWrap";
    declaration.enumFallback = "noWrap";
    declaration.enumAllowed = { "noWrap", "wrap" };
    declaration.applyValue = [this](const JsonValue& value) {
        SetTopLevelWrap(ResolveRowWrap(value.GetStringValue("noWrap")));
    };
    return declaration;
}

void ExtendedRowComponent::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
        ReportExtendedSchemaWarning(issue.code, issue.message, issue.propertyPath);
    }
}

void ExtendedRowComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStyleEnumProperty(
        styles, "justifyContent", { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" });
    ValidateStyleEnumProperty(styles, "alignItems", { "top", "center", "bottom" });
    ValidateStyleEnumProperty(styles, "wrap", { "noWrap", "wrap" });
}

void ExtendedRowComponent::ValidateComponentSpecificDynamicStylesDfx(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
{
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "justifyContent",
        { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" });
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "alignItems", { "top", "center", "bottom" });
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "wrap", { "noWrap", "wrap" });
}

void ExtendedRowComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    if (!styles.IsObject()) {
        return;
    }
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (!isDeltaUpdate) {
        hasStyleAlignItems_ = false;
        hasStyleJustifyContent_ = false;
        hasStyleWrap_ = false;
    }
    if (styles.Has("alignItems")) {
        styleAlignItems_ = ResolveRowAlign(styles.GetItem("alignItems").GetStringValue("center"));
        hasStyleAlignItems_ = true;
    }
    if (styles.Has("justifyContent")) {
        styleJustifyContent_ = ResolveRowJustify(styles.GetItem("justifyContent").GetStringValue("start"));
        hasStyleJustifyContent_ = true;
    }
    if (styles.Has("wrap")) {
        styleWrap_ = ResolveRowWrap(styles.GetItem("wrap").GetStringValue("noWrap"));
        hasStyleWrap_ = true;
    }
    ApplyEffectiveLayout();
}

void ExtendedRowComponent::SetTopLevelAlignItems(A2UIItemAlignment alignment)
{
    topLevelAlignItems_ = alignment;
    ApplyEffectiveLayout();
}

void ExtendedRowComponent::SetTopLevelJustifyContent(A2UIFlexAlignment alignment)
{
    topLevelJustifyContent_ = alignment;
    ApplyEffectiveLayout();
}

void ExtendedRowComponent::SetTopLevelWrap(A2UIFlexWrap wrap)
{
    topLevelWrap_ = wrap;
    ApplyEffectiveLayout();
}

void ExtendedRowComponent::SetItemMargin(float itemMargin)
{
    if (!std::isfinite(itemMargin) || itemMargin < 0.0F) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property itemMargin must be greater than or equal to 0 and has been reset to default", "itemMargin");
    }
    itemMargin_ = NormalizeItemMargin(itemMargin, DEFAULT_ITEM_MARGIN);
    ApplyItemMarginSpace();
}

void ExtendedRowComponent::ApplyEffectiveLayout()
{
    alignItems_ = hasStyleAlignItems_ ? styleAlignItems_ : topLevelAlignItems_;
    justifyContent_ = hasStyleJustifyContent_ ? styleJustifyContent_ : topLevelJustifyContent_;
    wrap_ = hasStyleWrap_ ? styleWrap_ : topLevelWrap_;
    itemMarginDisabledByJustify_ = IsItemMarginDisabledByJustify(justifyContent_);
    ApplyFlexOptions();
    ApplyItemMarginSpace();
}

void ExtendedRowComponent::ApplyItemMarginSpace()
{
    SetSpace(itemMarginDisabledByJustify_ ? 0.0F : itemMargin_);
}

void ExtendedRowComponent::SetSpace(float space)
{
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeFlexSpace(nativeView_, space, space / 2.0F);
}

void ExtendedRowComponent::ApplyFlexOptions()
{
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeFlexOption(
        nativeView_, A2UIFlexDirection::ROW, wrap_, justifyContent_, alignItems_, A2UIFlexAlignment::START);
}

} // namespace NativeModule
