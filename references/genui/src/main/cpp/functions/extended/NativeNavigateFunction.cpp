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

#include "NativeNavigateFunction.h"

#include <memory>
#include <string>

#include "components/extended/NavContainerComponent.h"
#include "functions/NativeFunctionComponentUtils.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

std::string ResolveComponentId(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return "";
    }

    JsonValue componentIdArg = resolvedArgs.GetItem("componentId");
    if (!componentIdArg.IsString()) {
        return "";
    }
    return componentIdArg.GetStringValue("");
}

std::string ResolveTargetComponentId(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return "";
    }

    JsonValue targetComponentIdArg = resolvedArgs.GetItem("targetComponentId");
    if (!targetComponentIdArg.IsString()) {
        return "";
    }
    return targetComponentIdArg.GetStringValue("");
}

} // namespace

std::string NativeNavigateFunction::GetName() const
{
    return "navigate";
}

FunctionResult NativeNavigateFunction::Execute(const JsonValue& resolvedArgs)
{
    (void)resolvedArgs;
    LOG_A2UI(LOG_WARN, "NativeNavigateFunction::Execute: direct Execute called, use ExecuteWithContext instead");
    return FunctionResult(false);
}

FunctionResult NativeNavigateFunction::ExecuteWithContext(
    const JsonValue& resolvedArgs, const DynamicResolveContext& context)
{
    std::string componentId = ResolveComponentId(resolvedArgs);
    std::string targetComponentId = ResolveTargetComponentId(resolvedArgs);
    if (componentId.empty() || targetComponentId.empty()) {
        return FunctionResult(false);
    }

    SurfaceSlot* surface = NativeFunctionComponentUtils::FindSurfaceForContext(context);
    if (surface == nullptr) {
        LOG_A2UI(LOG_WARN, "NativeNavigateFunction::ExecuteWithContext: surface not found, surfaceId=%{public}s",
            context.surfaceId.c_str());
        return FunctionResult(false);
    }

    std::shared_ptr<Component> component = surface->FindComponentById(componentId);
    auto navContainer = std::dynamic_pointer_cast<NavContainerComponent>(component);
    if (navContainer == nullptr) {
        LOG_A2UI(LOG_WARN,
            "NativeNavigateFunction::ExecuteWithContext: component is not NavContainer, componentId=%{public}s",
            componentId.c_str());
        return FunctionResult(false);
    }

    bool success = navContainer->NavigateToTargetComponent(targetComponentId);
    return FunctionResult(success);
}

} // namespace NativeModule
