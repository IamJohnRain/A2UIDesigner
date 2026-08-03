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
    alignItems_ = DEFAULT_ALIGN_ITEMS;
    justifyContent_ = DEFAULT_JUSTIFY_CONTENT;
    wrap_ = DEFAULT_WRAP;
    itemMarginDisabledByJustify_ = false;
    ApplyFlexOptions();
    if (descriptor.IsObject() && descriptor.Has("itemMargin")) {
        JsonValue itemMarginValue = descriptor.GetItem("itemMargin");
        if (!IsDynamicValueDescriptor(itemMarginValue) && itemMarginValue.IsNumber() &&
            itemMarginValue.GetNumberValue(DEFAULT_ITEM_MARGIN) < 0.0) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property itemMargin has invalid value and has been reset to default", "itemMargin");
        }
    }
    ApplyDeclaredPropertyOrFallback(descriptor, "itemMargin");
    ApplyDeclaredPropertyOrFallback(descriptor, "wrap");
}

PropertyDeclaration ExtendedRowComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ExtendedRowComponent&)>> declarations = {
        { "itemMargin",
            [](ExtendedRowComponent& component) {
                return PropertyDeclaration { .name = "itemMargin",
                    .type = PropertyValueType::NUMBER,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackNumber = DEFAULT_ITEM_MARGIN,
                    .applyValue = [&component](const JsonValue& value) {
                        component.SetItemMargin(static_cast<float>(value.GetNumberValue(DEFAULT_ITEM_MARGIN)));
                    } };
            } },
        { "wrap",
            [](ExtendedRowComponent& component) {
                PropertyDeclaration declaration;
                declaration.name = "wrap";
                declaration.type = PropertyValueType::ENUM_STRING;
                declaration.allowDynamic = true;
                declaration.allowExpression = true;
                declaration.fallbackString = "noWrap";
                declaration.enumFallback = "noWrap";
                declaration.enumAllowed = { "noWrap", "wrap" };
                declaration.applyValue = [&component](const JsonValue& value) {
                    component.SetWrap(ResolveRowWrap(value.GetStringValue("noWrap")));
                };
                return declaration;
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
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
}

void ExtendedRowComponent::ValidateComponentSpecificDynamicStylesDfx(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
{
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "justifyContent",
        { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" });
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "alignItems", { "top", "center", "bottom" });
}

void ExtendedRowComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    if (!styles.IsObject()) {
        return;
    }
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (styles.Has("alignItems")) {
        SetAlignItems(ResolveRowAlign(styles.GetItem("alignItems").GetStringValue("center")));
    } else if (!isDeltaUpdate) {
        SetAlignItems(DEFAULT_ALIGN_ITEMS);
    }
    if (styles.Has("justifyContent")) {
        SetJustifyContent(ResolveRowJustify(styles.GetItem("justifyContent").GetStringValue("start")));
    } else if (!isDeltaUpdate) {
        SetJustifyContent(DEFAULT_JUSTIFY_CONTENT);
    }
}

void ExtendedRowComponent::SetAlignItems(A2UIItemAlignment alignment)
{
    alignItems_ = alignment;
    ApplyFlexOptions();
}

void ExtendedRowComponent::SetJustifyContent(A2UIFlexAlignment alignment)
{
    justifyContent_ = alignment;
    itemMarginDisabledByJustify_ = IsItemMarginDisabledByJustify(alignment);
    ApplyFlexOptions();
    ApplyItemMarginSpace();
}

void ExtendedRowComponent::SetWrap(A2UIFlexWrap wrap)
{
    wrap_ = wrap;
    ApplyFlexOptions();
}

void ExtendedRowComponent::SetItemMargin(float itemMargin)
{
    itemMargin_ = NormalizeItemMargin(itemMargin, DEFAULT_ITEM_MARGIN);
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
