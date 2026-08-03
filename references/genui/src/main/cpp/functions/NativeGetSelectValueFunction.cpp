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

#include "NativeGetSelectValueFunction.h"

#include <cmath>

#include "components/custom/CustomComponent.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "NativeFunctionComponentUtils.h"
#include "RenderManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

std::string ResolveTargetComponentId(const JsonValue& resolvedArgs)
{
    JsonValue componentIdArg = resolvedArgs.GetItem("componentId");
    if (componentIdArg.IsString()) {
        std::string componentId = componentIdArg.GetStringValue("");
        if (!componentId.empty()) {
            return componentId;
        }
    }
    return "";
}

bool TryGetSelectedIndex(const CustomComponent& customComp, int32_t& selectedIndex)
{
    auto selectedOpt = customComp.GetProperty("selected");
    if (!selectedOpt.has_value() || !selectedOpt->IsNumber()) {
        return false;
    }
    double selectedValue = selectedOpt->GetNumberValue(-1.0);
    if (!std::isfinite(selectedValue)) {
        return false;
    }
    int32_t parsedIndex = static_cast<int32_t>(selectedValue);
    if (parsedIndex < 0 || static_cast<double>(parsedIndex) != selectedValue) {
        return false;
    }
    selectedIndex = parsedIndex;
    return true;
}

std::string ResolveOptionValueFromArray(const JsonValue& optionsValue, int32_t selectedIndex)
{
    if (!optionsValue.IsArray() || selectedIndex < 0 || selectedIndex >= optionsValue.GetArraySize()) {
        return "";
    }
    JsonValue selectedOption = optionsValue.GetArrayItem(selectedIndex);
    JsonValue selectedOptionValue = selectedOption.GetItem("value");
    if (!selectedOptionValue.IsString()) {
        return "";
    }
    return selectedOptionValue.GetStringValue("");
}

std::string ResolveOptionValue(const JsonValue& optionsValue, int32_t selectedIndex)
{
    if (optionsValue.IsArray()) {
        return ResolveOptionValueFromArray(optionsValue, selectedIndex);
    }
    if (!optionsValue.IsString()) {
        return "";
    }
    std::unique_ptr<JsonAdapter> optionsAdapter = JsonAdapter::Parse(optionsValue.GetStringValue(""));
    if (optionsAdapter == nullptr) {
        return "";
    }
    return ResolveOptionValueFromArray(optionsAdapter->GetRoot(), selectedIndex);
}

std::string ResolveInitialSelectedValue(const CustomComponent& customComp)
{
    int32_t selectedIndex = -1;
    if (!TryGetSelectedIndex(customComp, selectedIndex)) {
        return "";
    }
    auto optionsOpt = customComp.GetProperty("options");
    if (!optionsOpt.has_value()) {
        return "";
    }
    return ResolveOptionValue(optionsOpt.value(), selectedIndex);
}

} // namespace

std::string NativeGetSelectValueFunction::GetName() const
{
    return "getSelectValue";
}

FunctionResult NativeGetSelectValueFunction::Execute(const JsonValue& resolvedArgs)
{
    (void)resolvedArgs;
    LOG_A2UI(LOG_WARN, "getSelectValue: requires component context, cannot execute without context");
    return FunctionResult(std::string(""));
}

FunctionResult NativeGetSelectValueFunction::ExecuteWithContext(
    const JsonValue& resolvedArgs, const DynamicResolveContext& context)
{
    std::string targetComponentId = ResolveTargetComponentId(resolvedArgs);

    if (context.surfaceId.empty() || targetComponentId.empty()) {
        LOG_A2UI(LOG_WARN, "getSelectValue: surfaceId or target componentId is empty");
        return targetComponentId.empty() ? FunctionResult() : FunctionResult(std::string(""));
    }

    SurfaceSlot* slot = RenderManager::GetInstance().FindSurface(context.surfaceId);
    if (slot == nullptr) {
        LOG_A2UI(LOG_WARN, "getSelectValue: surface not found, surfaceId=%{public}s", context.surfaceId.c_str());
        return FunctionResult(std::string(""));
    }

    std::shared_ptr<Component> comp = slot->FindComponentById(targetComponentId);
    if (comp == nullptr) {
        LOG_A2UI(LOG_WARN, "getSelectValue: component not found, componentId=%{public}s", targetComponentId.c_str());
        return FunctionResult(std::string(""));
    }

    auto* customComp = dynamic_cast<CustomComponent*>(comp.get());
    if (customComp == nullptr) {
        LOG_A2UI(LOG_WARN, "getSelectValue: component is not a CustomComponent");
        return FunctionResult(std::string(""));
    }
    if (NativeFunctionComponentUtils::GetShortType(customComp->GetType()) != "Select") {
        LOG_A2UI(
            LOG_WARN, "getSelectValue: component is not Select, componentId=%{public}s", targetComponentId.c_str());
        return FunctionResult(std::string(""));
    }

    auto propOpt = customComp->GetProperty("value");
    if (propOpt.has_value() && propOpt->IsString() && !propOpt->GetStringValue("").empty()) {
        return FunctionResult(propOpt->GetStringValue(""));
    }
    return FunctionResult(ResolveInitialSelectedValue(*customComp));
}

} // namespace NativeModule
