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

#include "ColumnComponent.h"

#include "ColumnTheme.h"

namespace NativeModule {

ColumnComponent::ColumnComponent() : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN))
{
    spacing_ = ColumnTheme::GetDefaultSpace();
}

PropertyDeclaration ColumnComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ColumnComponent&)>> declarations = {
        { "align",
            [](ColumnComponent& columnComponent) {
                return PropertyDeclaration { .name = "align",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "start",
                    .enumAllowed = { "start", "center", "end" },
                    .enumFallback = "start",
                    .applyValue = [&columnComponent](const JsonValue& value) {
                        columnComponent.SetAlignItems(ColumnTheme::ResolveAlignItems(value.GetStringValue("start")));
                    } };
            } },
        { "justify",
            [](ColumnComponent& columnComponent) {
                return PropertyDeclaration { .name = "justify",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "start",
                    .enumAllowed = { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" },
                    .enumFallback = "start",
                    .applyValue = [&columnComponent](const JsonValue& value) {
                        columnComponent.SetJustifyContent(
                            ColumnTheme::ResolveJustifyContent(value.GetStringValue("start")));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

std::string ColumnComponent::GetType() const
{
    return "Column";
}

void ColumnComponent::SetAlignItems(A2UIHorizontalAlignment alignment)
{
    ArkUINodeApiAdapter::SetNodeColumnAlignItems(nativeView_, alignment);
}

void ColumnComponent::SetJustifyContent(A2UIFlexAlignment alignment)
{
    ArkUINodeApiAdapter::SetNodeColumnJustifyContent(nativeView_, alignment);
}

void ColumnComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    A2UIComponent::OnAddChild(child, index);
    RefreshSpacingOnChildAdded(child, index);
}

void ColumnComponent::OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    A2UIComponent::OnMoveChild(child, currentIndex, targetIndex);
    RefreshSpacingOnChildMoved(child, currentIndex, targetIndex);
}

void ColumnComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    ClearChildSpacing(child);
    A2UIComponent::OnRemoveChild(child);
}

void ColumnComponent::ApplyMarginToChild(const std::shared_ptr<Component>& child, float startMargin, float endMargin)
{
    if (child == nullptr) {
        return;
    }

    child->SetMargin(startMargin, 0.0F, endMargin, 0.0F);
}

void ColumnComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ApplySchemaProperty("align", descriptor);
    ApplySchemaProperty("justify", descriptor);
}

std::shared_ptr<ColumnTheme> ColumnComponent::GetTheme()
{
    // Try to get from cache first
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }

    // Cache miss, get from base class
    std::shared_ptr<ThemeBase> baseTheme = A2UIComponent::GetTheme();
    if (baseTheme == nullptr) {
        return nullptr;
    }

    // Cast to specific type and cache it
    theme = std::dynamic_pointer_cast<ColumnTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void ColumnComponent::OnConfigChange(const ThemeContext& context)
{
    auto columnTheme = GetTheme();
    if (columnTheme == nullptr) {
        return;
    }

    // TODO: Re-apply styles based on new theme context
}

void ColumnComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

bool ColumnComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    return ExpandTemplateChildrenEager(childList, surfaceSlot, childIds);
}

} // namespace NativeModule
