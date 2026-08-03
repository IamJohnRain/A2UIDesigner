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

#include "NativeGetToggleValueFunction.h"

#include <memory>

#include "components/extended/ExtendedToggleComponent.h"
#include "utils/LogA2UI.h"

#include "NativeFunctionComponentUtils.h"

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

FunctionResult CreateEmptyObjectResult()
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    return adapter != nullptr ? FunctionResult(adapter->GetRoot()) : FunctionResult();
}

} // namespace

std::string NativeGetToggleValueFunction::GetName() const
{
    return "getToggleValue";
}

FunctionResult NativeGetToggleValueFunction::Execute(const JsonValue& resolvedArgs)
{
    (void)resolvedArgs;
    LOG_A2UI(LOG_WARN, "NativeGetToggleValueFunction::Execute: direct Execute called, use ExecuteWithContext instead");
    return CreateEmptyObjectResult();
}

FunctionResult NativeGetToggleValueFunction::ExecuteWithContext(
    const JsonValue& resolvedArgs, const DynamicResolveContext& context)
{
    std::string componentId = ResolveComponentId(resolvedArgs);
    if (componentId.empty()) {
        return FunctionResult();
    }

    SurfaceSlot* surface = NativeFunctionComponentUtils::FindSurfaceForContext(context);
    if (surface == nullptr) {
        return CreateEmptyObjectResult();
    }

    std::shared_ptr<Component> component = surface->FindComponentById(componentId);
    std::shared_ptr<ExtendedToggleComponent> toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(component);
    if (toggle == nullptr) {
        return CreateEmptyObjectResult();
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return FunctionResult();
    }

    JsonValue root = adapter->GetRoot();
    if (!root.PutBool("isOn", toggle->GetIsOn()) || !root.PutString("label", toggle->GetLabel())) {
        return FunctionResult();
    }
    return FunctionResult(root);
}

} // namespace NativeModule
