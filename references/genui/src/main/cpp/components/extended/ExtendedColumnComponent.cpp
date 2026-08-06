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

#include "ExtendedColumnComponent.h"

#include <cmath>
#include <functional>
#include <map>

#include "components/ChildListSchemaValidationUtils.h"
#include "composition/ChildListParser.h"
#include "utils/LogA2UI.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

A2UIFlexAlignment ResolveColumnJustify(const std::string& justify)
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

A2UIHorizontalAlignment ResolveColumnAlign(const std::string& align)
{
    if (align == "start") {
        return A2UIHorizontalAlignment::START;
    }
    if (align == "center") {
        return A2UIHorizontalAlignment::CENTER;
    }
    if (align == "end") {
        return A2UIHorizontalAlignment::END;
    }
    return A2UIHorizontalAlignment::START;
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

void SetChildMargin(ArkUI_NodeHandle childNode, const CommonMargin& margin)
{
    ArkUINodeApiAdapter::SetNodeMargin(childNode, margin.top, margin.right, margin.bottom, margin.left);
}

CommonMargin GetChildCommonMargin(const std::shared_ptr<Component>& child)
{
    return child == nullptr ? CommonMargin() : child->GetCommonMargin();
}

bool ShouldApplyItemMarginToChild(
    const std::shared_ptr<Component>& child, const std::shared_ptr<Component>& excludedChild)
{
    return child != nullptr && child->GetNativeView() != nullptr &&
           (excludedChild == nullptr || child.get() != excludedChild.get());
}

} // namespace

ExtendedColumnComponent::ExtendedColumnComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN))
{}

std::string ExtendedColumnComponent::GetType() const
{
    return "Column";
}

void ExtendedColumnComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    SetAlignItems(A2UIHorizontalAlignment::START);
    SetJustifyContent(A2UIFlexAlignment::START);
    ApplyDeclaredPropertyOrFallback(descriptor, "itemMargin");
}

void ExtendedColumnComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListDescriptor();
    if (!descriptor.IsObject() || !descriptor.Has("children")) {
        return;
    }
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

PropertyDeclaration ExtendedColumnComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ExtendedColumnComponent&)>> declarations = {
        { "itemMargin",
            [](ExtendedColumnComponent& component) {
                return PropertyDeclaration { .name = "itemMargin",
                    .type = PropertyValueType::NUMBER,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackNumber = DEFAULT_ITEM_MARGIN,
                    .applyValue = [&component](const JsonValue& value) {
                        component.SetItemMargin(static_cast<float>(value.GetNumberValue(DEFAULT_ITEM_MARGIN)));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
}

void ExtendedColumnComponent::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
        ReportExtendedSchemaWarning(issue.code, issue.message, issue.propertyPath);
    }
}

void ExtendedColumnComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStyleEnumProperty(
        styles, "justifyContent", { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" });
    ValidateStyleEnumProperty(styles, "alignItems", { "start", "center", "end" });
}

void ExtendedColumnComponent::ValidateComponentSpecificDynamicStylesDfx(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
{
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "justifyContent",
        { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" });
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "alignItems", { "start", "center", "end" });
}

void ExtendedColumnComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    if (!styles.IsObject()) {
        return;
    }
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (styles.Has("alignItems")) {
        SetAlignItems(ResolveColumnAlign(styles.GetItem("alignItems").GetStringValue("start")));
    } else if (!isDeltaUpdate) {
        SetAlignItems(A2UIHorizontalAlignment::START);
    }
    if (styles.Has("justifyContent")) {
        SetJustifyContent(ResolveColumnJustify(styles.GetItem("justifyContent").GetStringValue("start")));
    } else if (!isDeltaUpdate) {
        SetJustifyContent(A2UIFlexAlignment::START);
    }
}

void ExtendedColumnComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    A2UIComponent::OnAddChild(child, index);
    ApplyItemMarginToChildren();
}

void ExtendedColumnComponent::OnMoveChild(
    const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    A2UIComponent::OnMoveChild(child, currentIndex, targetIndex);
    ApplyItemMarginToChildren();
}

void ExtendedColumnComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    RestoreCommonMarginForChild(child);
    A2UIComponent::OnRemoveChild(child);
    ApplyItemMarginToChildren(child);
}

void ExtendedColumnComponent::RemoveAllChildren()
{
    RestoreCommonMarginForChildren();
    Component::RemoveAllChildren();
}

void ExtendedColumnComponent::SetAlignItems(A2UIHorizontalAlignment alignment)
{
    ArkUINodeApiAdapter::SetNodeColumnAlignItems(nativeView_, alignment);
}

void ExtendedColumnComponent::SetJustifyContent(A2UIFlexAlignment alignment)
{
    itemMarginDisabledByJustify_ = IsItemMarginDisabledByJustify(alignment);
    ArkUINodeApiAdapter::SetNodeColumnJustifyContent(nativeView_, alignment);
    ApplyItemMarginToChildren();
}

void ExtendedColumnComponent::SetItemMargin(float itemMargin)
{
    if (!std::isfinite(itemMargin) || itemMargin < 0.0F) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property itemMargin must be greater than or equal to 0 and has been reset to default", "itemMargin");
    }
    itemMargin_ = NormalizeItemMargin(itemMargin, DEFAULT_ITEM_MARGIN);
    ApplyItemMarginToChildren();
}

void ExtendedColumnComponent::ApplyItemMarginToChildren(const std::shared_ptr<Component>& excludedChild)
{
    if (nativeView_ == nullptr) {
        return;
    }

    const auto& children = GetChildren();
    size_t childCount = 0;
    for (const auto& child : children) {
        if (ShouldApplyItemMarginToChild(child, excludedChild)) {
            ++childCount;
        }
    }
    size_t index = 0;
    float effectiveItemMargin = itemMarginDisabledByJustify_ ? 0.0F : itemMargin_;
    float halfMargin = effectiveItemMargin / 2.0F;
    for (const auto& child : children) {
        if (!ShouldApplyItemMarginToChild(child, excludedChild)) {
            continue;
        }
        CommonMargin margin = GetChildCommonMargin(child);
        margin.top += index > 0 ? halfMargin : 0.0F;
        margin.bottom += index + 1 < childCount ? halfMargin : 0.0F;
        SetChildMargin(child->GetNativeView(), margin);
        ++index;
    }
}

void ExtendedColumnComponent::RestoreCommonMarginForChild(const std::shared_ptr<Component>& child)
{
    if (nativeView_ == nullptr || child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }
    SetChildMargin(child->GetNativeView(), GetChildCommonMargin(child));
}

void ExtendedColumnComponent::RestoreCommonMarginForChildren()
{
    for (const auto& child : GetChildren()) {
        RestoreCommonMarginForChild(child);
    }
}

} // namespace NativeModule
