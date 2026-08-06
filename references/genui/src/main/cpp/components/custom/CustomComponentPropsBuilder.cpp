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

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "components/custom/CustomComponent.h"
#include "composition/ChildListDescriptor.h"
#include "data/DataBinding.h"
#include "data/DynamicValueResolver.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

constexpr const char* TABS_COMPONENT_TYPE = "Tabs";
constexpr const char* A2UI_BINDINGS_KEY = "__a2uiBindings";

bool CloneJsonValue(const JsonValue& input, JsonValue& output)
{
    if (!input.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsValid();
}

} // namespace

std::map<std::string, JsonValue> CustomComponent::CollectTabsProperties(
    const std::map<std::string, JsonValue>& properties) const
{
    std::map<std::string, JsonValue> tabsProperties;
    for (const auto& pair : properties) {
        if (pair.first.find("tabs[") == 0) {
            tabsProperties[pair.first] = pair.second;
        }
    }
    return tabsProperties;
}

JsonValue CustomComponent::GetOrCreateTabsArray(const std::map<std::string, JsonValue>& properties) const
{
    auto tabsIterator = properties.find("tabs");
    if (tabsIterator != properties.end() && tabsIterator->second.IsArray()) {
        JsonValue clonedTabs;
        if (CloneJsonValue(tabsIterator->second, clonedTabs) && clonedTabs.IsArray()) {
            return clonedTabs;
        }
        LOG_A2UI(LOG_ERROR, "BuildCustomProps: failed to clone original tabs array");
    }

    std::unique_ptr<JsonAdapter> tabsAdapter = JsonAdapter::CreateArray();
    if (tabsAdapter == nullptr) {
        return JsonValue();
    }
    return tabsAdapter->GetRoot();
}

void CustomComponent::UpdateTabsWithProperties(
    JsonValue& tabsArray, const std::map<std::string, JsonValue>& tabsProperties) const
{
    if (!tabsArray.IsArray()) {
        return;
    }

    for (const auto& pair : tabsProperties) {
        const std::string& key = pair.first;
        size_t startPos = key.find('[');
        size_t endPos = key.find(']');
        size_t dotPos = key.find('.');
        if (startPos == std::string::npos || endPos == std::string::npos || dotPos == std::string::npos) {
            continue;
        }

        int index = std::stoi(key.substr(startPos + 1, endPos - startPos - 1));
        std::string propPath = key.substr(dotPos + 1);
        while (tabsArray.GetArraySize() <= index) {
            std::unique_ptr<JsonAdapter> emptyObject = JsonAdapter::CreateObject();
            if (emptyObject == nullptr || !tabsArray.Append(emptyObject->GetRoot())) {
                break;
            }
        }

        JsonValue tabObj = tabsArray.GetArrayItem(index);
        if (!tabObj.IsObject()) {
            continue;
        }

        JsonValue clonedValue;
        if (!CloneJsonValue(pair.second, clonedValue)) {
            continue;
        }
        if (!tabObj.Set(propPath.c_str(), clonedValue)) {
            continue;
        }
    }
}

void CustomComponent::ResolveFunctionCallsInTabsArray(JsonValue& tabsArray) const
{
    if (!tabsArray.IsArray()) {
        return;
    }

    DynamicResolveContext context = { .renderId = GetRenderId(),
        .surfaceId = GetSurfaceId(),
        .componentId = GetComponentId(),
        .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };

    int tabCount = tabsArray.GetArraySize();
    for (int i = 0; i < tabCount; ++i) {
        JsonValue tabObj = tabsArray.GetArrayItem(i);
        if (!tabObj.IsObject()) {
            continue;
        }

        JsonValue titleItem = tabObj.GetItem("title");
        if (!titleItem.IsObject()) {
            continue;
        }

        JsonValue callItem = titleItem.GetItem("call");
        if (!callItem.IsString()) {
            continue;
        }

        JsonValue clonedTitle;
        if (!CloneJsonValue(titleItem, clonedTitle)) {
            continue;
        }

        ResolvedValue result = DynamicValueResolver::Resolve(clonedTitle, context);
        if (result.success && result.source == ResolveSource::FUNCTION_CALL && result.value.IsString()) {
            std::unique_ptr<JsonAdapter> newTitle = JsonAdapter::CreateString(result.value.GetStringValue(""));
            if (newTitle != nullptr) {
                tabObj.Set("title", newTitle->GetRoot());
            }
            LOG_A2UI(LOG_INFO, "ResolveFunctionCallsInTabsArray: resolved tabs[%{public}d].title via function call", i);
        } else {
            LOG_A2UI(
                LOG_WARN, "ResolveFunctionCallsInTabsArray: failed to resolve tabs[%{public}d].title function call", i);
        }
    }
}

void CustomComponent::AddTabsToBuilder(
    std::ostringstream& builder, const JsonValue& tabsArray, bool& hasCustomProp) const
{
    (void)builder;
    (void)tabsArray;
    (void)hasCustomProp;
}

void CustomComponent::AddOtherPropertiesToBuilder(
    std::ostringstream& builder, const std::map<std::string, JsonValue>& properties, bool& hasCustomProp) const
{
    (void)builder;
    (void)properties;
    (void)hasCustomProp;
}

JsonValue BuildCustomBindingProps(const std::vector<DataBinding>& bindings)
{
    std::unique_ptr<JsonAdapter> bindingsAdapter = JsonAdapter::CreateObject();
    if (bindingsAdapter == nullptr) {
        return JsonValue();
    }

    JsonValue bindingsObject = bindingsAdapter->GetRoot();
    for (const auto& binding : bindings) {
        if (binding.propertyName_.empty() || binding.dataPath_.empty()) {
            continue;
        }
        bindingsObject.PutString(binding.propertyName_.c_str(), binding.dataPath_);
    }
    return bindingsObject.GetChild().IsValid() ? bindingsObject : JsonValue();
}

JsonValue BuildExtendedTabsTemplateChildren(const ChildListDescriptor& childList)
{
    if (childList.type != ChildListType::TEMPLATE_PATH) {
        return JsonValue();
    }

    std::unique_ptr<JsonAdapter> templateAdapter = JsonAdapter::CreateObject();
    if (templateAdapter == nullptr) {
        return JsonValue();
    }

    JsonValue templateChildren = templateAdapter->GetRoot();
    templateChildren.PutString("componentId", childList.templateComponentId);
    templateChildren.PutString("path", childList.templatePath);
    if (!childList.useDefaultIndexVar) {
        templateChildren.PutString("indexVar", childList.resolvedIndexVarName);
    }
    if (!childList.useDefaultItemVar) {
        templateChildren.PutString("itemVar", childList.resolvedItemVarName);
    }
    return templateChildren;
}

const std::map<std::string, JsonValue>& CustomComponent::BuildEffectiveCustomProperties(
    std::map<std::string, JsonValue>& effectiveProperties) const
{
    if (!preserveDynamicDescriptors_ || rawDynamicProperties_.empty()) {
        return properties_;
    }

    effectiveProperties = properties_;
    for (const auto& pair : rawDynamicProperties_) {
        effectiveProperties[pair.first] = pair.second;
    }
    return effectiveProperties;
}

void CustomComponent::PutTabsCustomProp(
    JsonValue& customProps, const std::map<std::string, JsonValue>& properties) const
{
    std::map<std::string, JsonValue> tabsProperties = CollectTabsProperties(properties);
    JsonValue tabsArray = GetOrCreateTabsArray(properties);
    UpdateTabsWithProperties(tabsArray, tabsProperties);
    MergeTabsFromChildIds(tabsArray);
    if (!preserveDynamicDescriptors_) {
        ResolveFunctionCallsInTabsArray(tabsArray);
    }

    auto tabsIterator = properties.find("tabs");
    bool hasRawTabs = tabsIterator != properties.end() && tabsIterator->second.IsValid();
    if (hasRawTabs) {
        if (tabsIterator->second.IsArray()) {
            customProps.Put("tabs", tabsArray);
        } else {
            JsonValue rawTabsValue;
            if (CloneJsonValue(tabsIterator->second, rawTabsValue)) {
                customProps.Put("tabs", rawTabsValue);
            }
        }
    } else if (tabsArray.IsArray() && tabsArray.GetArraySize() > 0) {
        customProps.Put("tabs", tabsArray);
    }
}

void CustomComponent::PutChildrenCustomProp(JsonValue& customProps) const
{
    if (IsExtendedTabsType() && childListDescriptor_.type == ChildListType::TEMPLATE_PATH) {
        JsonValue templateChildren = BuildExtendedTabsTemplateChildren(childListDescriptor_);
        if (templateChildren.IsValid()) {
            customProps.Put("children", templateChildren);
        }
        return;
    }

    std::unique_ptr<JsonAdapter> childrenAdapter = JsonAdapter::CreateArray();
    if (childrenAdapter == nullptr) {
        return;
    }
    JsonValue childrenArray = childrenAdapter->GetRoot();
    MergeTabsChildren(childrenArray);
    MergeRowChildren(childrenArray);
    if (childrenArray.IsArray() && childrenArray.GetArraySize() > 0) {
        customProps.Put("children", childrenArray);
        return;
    }
    if (!IsExtendedTabsType()) {
        return;
    }
    if (childListDescriptor_.type == ChildListType::STATIC_IDS) {
        customProps.Put("children", childrenArray);
    }
}

void CustomComponent::PutNamedCustomProperties(
    JsonValue& customProps, const std::map<std::string, JsonValue>& properties) const
{
    for (const std::string& key : customPropertyNames_) {
        if (key == "tabs" || key.find("tabs[") == 0) {
            continue;
        }
        if (key == "children" && IsExtendedTabsType() && !tabChildIds_.empty()) {
            continue;
        }

        auto iterator = properties.find(key);
        if (iterator == properties.end() || !iterator->second.IsValid()) {
            continue;
        }
        customProps.Put(key.c_str(), iterator->second);
    }
}

JsonValue CustomComponent::BuildCustomProps() const
{
    std::unique_ptr<JsonAdapter> customPropsAdapter = JsonAdapter::CreateObject();
    if (customPropsAdapter == nullptr) {
        return JsonValue();
    }
    JsonValue customProps = customPropsAdapter->GetRoot();
    std::map<std::string, JsonValue> effectiveProperties;
    const auto& properties = BuildEffectiveCustomProperties(effectiveProperties);
    PutTabsCustomProp(customProps, properties);
    PutChildrenCustomProp(customProps);
    PutNamedCustomProperties(customProps, properties);

    JsonValue bindingsProps = BuildCustomBindingProps(GetDataBindings());
    if (bindingsProps.IsValid()) {
        customProps.Put(A2UI_BINDINGS_KEY, bindingsProps);
    }

    if (!customProps.GetChild().IsValid()) {
        if (descriptor_.type == TABS_COMPONENT_TYPE) {
            return customProps;
        }
        return JsonValue();
    }
    if (IsExtendedEtsExpressionScope()) {
        return ResolveExpressionsInValue(customProps, "");
    }
    return customProps;
}

} // namespace NativeModule
