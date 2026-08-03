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

#include "RowComponent.h"

#include "RowTheme.h"

namespace NativeModule {

RowComponent::RowComponent() : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::ROW))
{
    spacing_ = RowTheme::GetDefaultSpace();
}

PropertyDeclaration RowComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(RowComponent&)>> declarations = {
        { "align",
            [](RowComponent& rowComponent) {
                return PropertyDeclaration { .name = "align",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "start",
                    .enumAllowed = { "start", "center", "end" },
                    .enumFallback = "start",
                    .applyValue = [&rowComponent](const JsonValue& value) {
                        rowComponent.SetAlignItems(RowTheme::ResolveAlignItems(value.GetStringValue("start")));
                    } };
            } },
        { "justify",
            [](RowComponent& rowComponent) {
                return PropertyDeclaration { .name = "justify",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "start",
                    .enumAllowed = { "start", "center", "end", "spaceAround", "spaceBetween", "spaceEvenly" },
                    .enumFallback = "start",
                    .applyValue = [&rowComponent](const JsonValue& value) {
                        rowComponent.SetJustifyContent(RowTheme::ResolveJustifyContent(value.GetStringValue("start")));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

std::string RowComponent::GetType() const
{
    return "Row";
}

void RowComponent::SetAlignItems(A2UIVerticalAlignment alignment)
{
    ArkUINodeApiAdapter::SetNodeRowAlignItems(nativeView_, alignment);
}

void RowComponent::SetJustifyContent(A2UIFlexAlignment alignment)
{
    ArkUINodeApiAdapter::SetNodeRowJustifyContent(nativeView_, alignment);
}

void RowComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    A2UIComponent::OnAddChild(child, index);
    RefreshSpacingOnChildAdded(child, index);
}

void RowComponent::OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    A2UIComponent::OnMoveChild(child, currentIndex, targetIndex);
    RefreshSpacingOnChildMoved(child, currentIndex, targetIndex);
}

void RowComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    ClearChildSpacing(child);
    A2UIComponent::OnRemoveChild(child);
}

void RowComponent::ApplyMarginToChild(const std::shared_ptr<Component>& child, float startMargin, float endMargin)
{
    if (child == nullptr) {
        return;
    }

    child->SetMargin(0.0F, endMargin, 0.0F, startMargin);
}

void RowComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ApplySchemaProperty("align", descriptor);
    ApplySchemaProperty("justify", descriptor);
}

std::shared_ptr<RowTheme> RowComponent::GetTheme()
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
    theme = std::dynamic_pointer_cast<RowTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void RowComponent::OnConfigChange(const ThemeContext& context)
{
    auto rowTheme = GetTheme();
    if (rowTheme == nullptr) {
        return;
    }

    // TODO: Re-apply styles based on new theme context
}

void RowComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

bool RowComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    return ExpandTemplateChildrenEager(childList, surfaceSlot, childIds);
}

} // namespace NativeModule
