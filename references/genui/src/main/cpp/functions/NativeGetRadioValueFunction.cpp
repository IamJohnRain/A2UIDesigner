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

#include "NativeGetRadioValueFunction.h"

#include <memory>

#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedRadioComponent.h"
#include "utils/LogA2UI.h"

#include "NativeFunctionComponentUtils.h"

namespace NativeModule {

namespace {

std::string ResolveGroupName(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return "";
    }
    JsonValue groupArg = resolvedArgs.GetItem("group");
    if (!groupArg.IsString()) {
        return "";
    }
    return groupArg.GetStringValue("");
}

} // namespace

std::string NativeGetRadioValueFunction::GetName() const
{
    return "getRadioValue";
}

FunctionResult NativeGetRadioValueFunction::Execute(const JsonValue& resolvedArgs)
{
    (void)resolvedArgs;
    LOG_A2UI(LOG_WARN, "NativeGetRadioValueFunction::Execute: direct Execute called, use ExecuteWithContext instead");
    return FunctionResult(std::string(""));
}

FunctionResult NativeGetRadioValueFunction::ExecuteWithContext(
    const JsonValue& resolvedArgs, const DynamicResolveContext& context)
{
    std::string group = ResolveGroupName(resolvedArgs);
    if (group.empty()) {
        return FunctionResult();
    }

    SurfaceSlot* surface = NativeFunctionComponentUtils::FindSurfaceForContext(context);
    if (surface == nullptr) {
        return FunctionResult(std::string(""));
    }

    std::string selectedValue;
    bool resolved = false;
    surface->ForEachComponent([&](const std::shared_ptr<Component>& component) {
        if (resolved) {
            return;
        }
        std::shared_ptr<ExtendedRadioComponent> radioComponent =
            std::dynamic_pointer_cast<ExtendedRadioComponent>(component);
        if (radioComponent != nullptr) {
            if (radioComponent->GetGroup() == group && radioComponent->GetChecked()) {
                resolved = true;
                selectedValue = radioComponent->GetValue();
            }
            return;
        }

        std::shared_ptr<CustomComponent> customComponent = std::dynamic_pointer_cast<CustomComponent>(component);
        if (customComponent == nullptr ||
            NativeFunctionComponentUtils::GetShortType(customComponent->GetType()) != "Radio") {
            return;
        }

        JsonValue groupValue = customComponent->GetCustomProperty("group");
        if (!groupValue.IsString() || groupValue.GetStringValue("") != group) {
            return;
        }

        JsonValue checkedValue = customComponent->GetCustomProperty("checked");
        if (!checkedValue.IsBool() || !checkedValue.GetBoolValue(false)) {
            return;
        }

        resolved = true;
        JsonValue value = customComponent->GetCustomProperty("value");
        if (value.IsString()) {
            selectedValue = value.GetStringValue("");
        }
    });

    return FunctionResult(std::move(selectedValue));
}

} // namespace NativeModule
