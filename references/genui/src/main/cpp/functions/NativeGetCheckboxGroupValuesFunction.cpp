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

#include "NativeGetCheckboxGroupValuesFunction.h"

#include <map>
#include <memory>

#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedCheckboxComponent.h"
#include "utils/LogA2UI.h"

#include "NativeFunctionComponentUtils.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

constexpr const char* CHECKBOX_SELECT_RUNTIME_STATE_SCOPE = "ExtendedCheckbox.select";
constexpr const char* RUNTIME_STATE_GROUP_KEY = "group";
constexpr const char* RUNTIME_STATE_VALUE_KEY = "value";
constexpr const char* RUNTIME_STATE_SELECT_KEY = "select";

struct RuntimeSelections {
    std::map<std::string, bool> selectedByRuntimeKey;
    std::map<std::string, bool> selectedByValue;
};

std::string ResolveGroup(const JsonValue& resolvedArgs)
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

FunctionResult BuildEmptyArrayResult()
{
    auto arrAdapter = JsonAdapter::CreateArray();
    return (arrAdapter == nullptr) ? FunctionResult(std::string("")) : FunctionResult(arrAdapter->GetRoot());
}

FunctionResult BuildArrayResult(const std::vector<std::string>& labels)
{
    auto arrAdapter = JsonAdapter::CreateArray();
    if (arrAdapter == nullptr) {
        return FunctionResult(std::string(""));
    }
    JsonValue root = arrAdapter->GetRoot();
    for (const auto& label : labels) {
        auto strAdapter = JsonAdapter::CreateString(label);
        if (strAdapter != nullptr) {
            root.Append(strAdapter->GetRoot());
        }
    }
    return FunctionResult(root);
}

void MergeSelectionByValue(const std::string& value, bool selected, std::map<std::string, bool>& selectedByValue)
{
    if (value.empty()) {
        return;
    }
    auto iter = selectedByValue.find(value);
    if (iter == selectedByValue.end()) {
        selectedByValue[value] = selected;
        return;
    }
    iter->second = iter->second || selected;
}

void ReadRuntimeSelections(SurfaceSlot* surface, const std::string& group, RuntimeSelections& selections)
{
    if (surface == nullptr || group.empty()) {
        return;
    }
    surface->ForEachRuntimeState(
        CHECKBOX_SELECT_RUNTIME_STATE_SCOPE, [&group, &selections](const std::string& key, const JsonValue& state) {
            if (!state.IsObject() || state.GetString(RUNTIME_STATE_GROUP_KEY, "") != group) {
                return;
            }
            std::string value = state.GetString(RUNTIME_STATE_VALUE_KEY, "");
            JsonValue selectedValue = state.GetItem(RUNTIME_STATE_SELECT_KEY);
            if (value.empty() || !selectedValue.IsBool()) {
                return;
            }
            bool selected = selectedValue.GetBoolValue(false);
            selections.selectedByRuntimeKey[key] = selected;
            MergeSelectionByValue(value, selected, selections.selectedByValue);
        });
}

void AddCurrentCheckboxSelection(const std::string& value, bool selected, std::map<std::string, bool>& selectedByValue)
{
    MergeSelectionByValue(value, selected, selectedByValue);
}

void CollectCheckboxSelection(
    const std::shared_ptr<Component>& component, const std::string& group, RuntimeSelections& selections)
{
    std::shared_ptr<ExtendedCheckboxComponent> checkboxComponent =
        std::dynamic_pointer_cast<ExtendedCheckboxComponent>(component);
    if (checkboxComponent != nullptr) {
        if (checkboxComponent->GetGroup() == group) {
            if (selections.selectedByRuntimeKey.find(checkboxComponent->GetRuntimeStateKey()) ==
                selections.selectedByRuntimeKey.end()) {
                AddCurrentCheckboxSelection(
                    checkboxComponent->GetValue(), checkboxComponent->GetSelect(), selections.selectedByValue);
            }
        }
        return;
    }

    std::shared_ptr<CustomComponent> customComponent = std::dynamic_pointer_cast<CustomComponent>(component);
    if (customComponent == nullptr ||
        NativeFunctionComponentUtils::GetShortType(customComponent->GetType()) != "Checkbox") {
        return;
    }

    JsonValue groupValue = customComponent->GetCustomProperty("group");
    if (!groupValue.IsString() || groupValue.GetStringValue("") != group) {
        return;
    }

    JsonValue checkedValue = customComponent->GetCustomProperty("select");
    if (!checkedValue.IsBool()) {
        return;
    }

    JsonValue labelValue = customComponent->GetCustomProperty("value");
    if (labelValue.IsString()) {
        std::string cbValue = labelValue.GetStringValue("");
        AddCurrentCheckboxSelection(cbValue, checkedValue.GetBoolValue(false), selections.selectedByValue);
    }
}

} // namespace

std::string NativeGetCheckboxGroupValuesFunction::GetName() const
{
    return "getCheckboxGroupValues";
}

FunctionResult NativeGetCheckboxGroupValuesFunction::Execute(const JsonValue& resolvedArgs)
{
    (void)resolvedArgs;
    LOG_A2UI(LOG_WARN, "getCheckboxGroupValues: requires component context, cannot execute without context");
    return BuildEmptyArrayResult();
}

FunctionResult NativeGetCheckboxGroupValuesFunction::ExecuteWithContext(
    const JsonValue& resolvedArgs, const DynamicResolveContext& context)
{
    std::string group = ResolveGroup(resolvedArgs);
    if (group.empty()) {
        LOG_A2UI(LOG_WARN, "getCheckboxGroupValues: group arg is empty");
        return FunctionResult();
    }

    SurfaceSlot* surface = NativeFunctionComponentUtils::FindSurfaceForContext(context);
    if (surface == nullptr) {
        LOG_A2UI(
            LOG_WARN, "getCheckboxGroupValues: surface not found, surfaceId=%{public}s", context.surfaceId.c_str());
        return BuildEmptyArrayResult();
    }

    RuntimeSelections selections;
    ReadRuntimeSelections(surface, group, selections);

    surface->ForEachComponent(
        [&](const std::shared_ptr<Component>& component) { CollectCheckboxSelection(component, group, selections); });

    std::vector<std::string> selectedLabels;
    for (const auto& entry : selections.selectedByValue) {
        if (entry.second) {
            selectedLabels.push_back(entry.first);
        }
    }

    LOG_A2UI(LOG_DEBUG, "getCheckboxGroupValues: group=%{public}s, count=%{public}d, names=%{public}s", group.c_str(),
        static_cast<int>(selectedLabels.size()), BuildArrayResult(selectedLabels).ToString().c_str());

    return BuildArrayResult(selectedLabels);
}

} // namespace NativeModule
