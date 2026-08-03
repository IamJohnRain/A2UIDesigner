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

#include "ExtendedStackComponent.h"

#include "components/ChildListSchemaValidationUtils.h"

namespace NativeModule {

namespace {

A2UIAlignment ResolveStackAlignContent(const std::string& alignContent)
{
    if (alignContent == "topStart") {
        return A2UIAlignment::TOP_START;
    }
    if (alignContent == "top") {
        return A2UIAlignment::TOP;
    }
    if (alignContent == "topEnd") {
        return A2UIAlignment::TOP_END;
    }
    if (alignContent == "start") {
        return A2UIAlignment::START;
    }
    if (alignContent == "center") {
        return A2UIAlignment::CENTER;
    }
    if (alignContent == "end") {
        return A2UIAlignment::END;
    }
    if (alignContent == "bottomStart") {
        return A2UIAlignment::BOTTOM_START;
    }
    if (alignContent == "bottom") {
        return A2UIAlignment::BOTTOM;
    }
    if (alignContent == "bottomEnd") {
        return A2UIAlignment::BOTTOM_END;
    }
    return A2UIAlignment::CENTER;
}

} // namespace

ExtendedStackComponent::ExtendedStackComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::STACK))
{}

std::string ExtendedStackComponent::GetType() const
{
    return "Stack";
}

void ExtendedStackComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    static_cast<void>(descriptor);
    SetAlignContent(A2UIAlignment::CENTER);
}

void ExtendedStackComponent::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
        ReportExtendedSchemaWarning(issue.code, issue.message, issue.propertyPath);
    }
}

void ExtendedStackComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStyleEnumProperty(styles, "alignContent",
        { "topStart", "top", "topEnd", "start", "center", "end", "bottomStart", "bottom", "bottomEnd" });
}

void ExtendedStackComponent::ValidateComponentSpecificDynamicStylesDfx(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
{
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "alignContent",
        { "topStart", "top", "topEnd", "start", "center", "end", "bottomStart", "bottom", "bottomEnd" });
}

PropertyDeclaration ExtendedStackComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
}

bool ExtendedStackComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    return ExpandTemplateChildrenEager(childList, surfaceSlot, childIds);
}

void ExtendedStackComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    if (!styles.IsObject()) {
        return;
    }
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (styles.Has("alignContent")) {
        SetAlignContent(ResolveStackAlignContent(styles.GetItem("alignContent").GetStringValue("center")));
    } else if (!isDeltaUpdate) {
        SetAlignContent(A2UIAlignment::CENTER);
    }
}

void ExtendedStackComponent::SetAlignContent(A2UIAlignment alignment)
{
    ArkUINodeApiAdapter::SetNodeStackAlignContent(nativeView_, alignment);
}

} // namespace NativeModule
