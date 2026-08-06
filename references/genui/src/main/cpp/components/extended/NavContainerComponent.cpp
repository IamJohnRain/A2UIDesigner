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

#include "NavContainerComponent.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>

#include "components/ChildListSchemaValidationUtils.h"
#include "utils/LogA2UI.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr int32_t NAV_CONTAINER_CURRENT_INDEX_FALLBACK = 0;

int32_t ClampCurrentIndex(int32_t index, size_t childCount)
{
    if (childCount == 0) {
        return 0;
    }
    if (index < 0) {
        return 0;
    }
    if (static_cast<size_t>(index) >= childCount) {
        return static_cast<int32_t>(childCount - 1);
    }
    return index;
}

} // namespace

NavContainerComponent::NavContainerComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN))
{}

std::string NavContainerComponent::GetType() const
{
    return "NavContainer";
}

void NavContainerComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    isApplyingCurrentIndexDescriptor_ = true;
    hasDescriptorCurrentIndex_ = descriptor.IsObject() && descriptor.Has("currentIndex");
    hasDescriptorChildCount_ = false;
    descriptorChildCount_ = 0;
    if (descriptor.IsObject() && descriptor.Has("children")) {
        JsonValue childrenValue = descriptor.GetItem("children");
        if (childrenValue.IsArray()) {
            hasDescriptorChildCount_ = true;
            descriptorChildCount_ = static_cast<size_t>(childrenValue.GetArraySize());
        }
    }
    ApplySchemaProperty("currentIndex", descriptor);
    isApplyingCurrentIndexDescriptor_ = false;
    RefreshChildVisibility();
}

void NavContainerComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListDescriptor();
    if (!descriptor.IsObject() || !descriptor.Has("children")) {
        return;
    }

    JsonValue childrenValue = descriptor.GetItem("children");
    childListDescriptor_ = ChildListParser::ParseChildren(childrenValue);
}

void NavContainerComponent::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
        ReportExtendedSchemaWarning(issue.code, issue.message, issue.propertyPath);
    }
}

PropertyDeclaration NavContainerComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(NavContainerComponent&)>> declarations = {
        { "currentIndex",
            [](NavContainerComponent& component) {
                return PropertyDeclaration { .name = "currentIndex",
                    .type = PropertyValueType::NUMBER,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackNumber = NAV_CONTAINER_CURRENT_INDEX_FALLBACK,
                    .applyValue = [&component](const JsonValue& value) { component.ApplyCurrentIndexValue(value); } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

void NavContainerComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    ExtendedComponent::OnAddChild(child, index);
    RefreshChildVisibility();
}

void NavContainerComponent::OnMoveChild(
    const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    ExtendedComponent::OnMoveChild(child, currentIndex, targetIndex);
    RefreshChildVisibility();
}

void NavContainerComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    ExtendedComponent::OnRemoveChild(child);
    RefreshChildVisibility();
}

std::vector<std::string> NavContainerComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return {};
}

bool NavContainerComponent::NavigateToTargetComponent(const std::string& targetComponentId)
{
    if (targetComponentId.empty()) {
        return false;
    }

    int32_t targetIndex = 0;
    bool found = false;
    const auto& children = GetChildren();
    for (const auto& child : children) {
        if (child != nullptr && child->GetComponentId() == targetComponentId) {
            found = true;
            break;
        }
        ++targetIndex;
    }

    if (!found) {
        return false;
    }

    SetCurrentIndex(targetIndex);
    RefreshChildVisibility();
    return true;
}

void NavContainerComponent::ApplyCurrentIndexValue(const JsonValue& value)
{
    if (isApplyingCurrentIndexDescriptor_ && !hasDescriptorCurrentIndex_) {
        SetCurrentIndex(NAV_CONTAINER_CURRENT_INDEX_FALLBACK);
        RefreshChildVisibility();
        return;
    }

    double currentIndex = value.GetNumberValue(NAV_CONTAINER_CURRENT_INDEX_FALLBACK);
    size_t childCount = ResolveCurrentIndexValidationChildCount();
    bool isInvalidNumber = !std::isfinite(currentIndex) || currentIndex < 0.0 ||
                           std::floor(currentIndex) != currentIndex ||
                           currentIndex > static_cast<double>(std::numeric_limits<int32_t>::max());
    if (isInvalidNumber) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property currentIndex must be a non-negative integer less than children.length, fallback to 0",
            "currentIndex");
        SetCurrentIndex(NAV_CONTAINER_CURRENT_INDEX_FALLBACK);
        RefreshChildVisibility();
        return;
    }

    int32_t resolvedIndex = static_cast<int32_t>(currentIndex);
    if (currentIndex >= static_cast<double>(childCount)) {
        resolvedIndex = ClampCurrentIndex(resolvedIndex, childCount);
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property currentIndex exceeds children.length, clamped to " + std::to_string(resolvedIndex),
            "currentIndex");
    }

    SetCurrentIndex(resolvedIndex);
    RefreshChildVisibility();
}

void NavContainerComponent::SetCurrentIndex(int32_t currentIndex)
{
    currentIndex_ = currentIndex;
}

int32_t NavContainerComponent::ResolveVisibleIndex(size_t childCount) const
{
    return ClampCurrentIndex(currentIndex_, childCount);
}

size_t NavContainerComponent::ResolveCurrentIndexValidationChildCount() const
{
    if (isApplyingCurrentIndexDescriptor_ && hasDescriptorChildCount_) {
        return descriptorChildCount_;
    }
    return GetChildren().size();
}

void NavContainerComponent::RefreshChildVisibility()
{
    const auto& children = GetChildren();
    if (children.empty()) {
        return;
    }

    int32_t visibleIndex = ResolveVisibleIndex(children.size());
    size_t index = 0;
    for (const auto& child : children) {
        if (child == nullptr) {
            ++index;
            continue;
        }
        child->SetVisibility(
            index == static_cast<size_t>(visibleIndex) ? A2UIVisibility::VISIBLE : A2UIVisibility::NONE);
        ++index;
    }
}

} // namespace NativeModule
