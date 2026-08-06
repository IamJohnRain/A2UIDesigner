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

#ifndef A2UI_CUSTOM_COMPONENT_H
#define A2UI_CUSTOM_COMPONENT_H

#include <cstdint>
#include <js_native_api.h>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "checks/ChecksEngine.h"
#include "components/actions/EventHandlerParser.h"
#include "theme/ThemeBase.h"

#include "../Component.h"
#include "ArkUINodeApiAdapter.h"
#include "CustomComponentDescriptor.h"

namespace NativeModule {

struct DynamicValueDependencies;
struct ResolvedValue;

std::unique_ptr<JsonAdapter> BuildComponentThemeJson(const ThemeContext& themeContext);
void SetComponentThemeProperty(napi_env env, napi_value object, const ThemeContext& themeContext);
std::string ResolveCustomPropertyWarningPath(const std::string& componentId, const std::string& propertyKey);

class CustomComponent : public Component {
public:
    explicit CustomComponent(const std::string& componentType, bool preserveDynamicDescriptors = false);
    ~CustomComponent() override;
    static CustomComponent* FindByHandle(uintptr_t handle);
    bool ValidateChecks(const std::string& targetJsonLiteral, std::string* failedMessage) const;
    std::string GetType() const override;
    uintptr_t GetCustomComponentHandle() const;
    void RemoveAllChildren() override;
    void SetMargin(float top, float right, float bottom, float left) override;
    void DispatchEvent(const std::string& listenerName, const JsonValue& extraContext);
    JsonValue GetCustomProperty(const std::string& propertyName) const;
    bool SetRuntimeCustomProperty(const std::string& propertyName, const JsonValue& value);
    bool RegisterDynamicValueCallback(const std::string& propertyName, const JsonValue& descriptor, napi_env env,
        napi_value callback, std::string* errorMessage);
    void ClearDynamicValueCallback(const std::string& propertyName);
    void SyncCheckedToBoundDataModel(const std::string& bindingPath, bool value);
    std::optional<JsonValue> GetProperty(const std::string& key) const;

protected:
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;
    void OnAttachToParent() override;
    void ApplyCommonAttributes(const JsonValue& descriptor) override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void OnConfigChange(const ThemeContext& context) override;
    void OnPropertyRemoved(const std::string& propertyName) override;
    void OnPropertyApplied(const std::string& propertyName, const JsonValue& value) override;
    bool ShouldValidateUnknownDescriptorFields() const override;
    void ValidateComponentDescriptorSchema(const JsonValue& descriptor) override;
    bool AcceptsChild(const std::shared_ptr<Component>& child) const override;
    bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds) override;

private:
    enum class FlexShrinkStyleState { UNSPECIFIED, EXPLICIT_VALUE, DYNAMIC_VALUE, PARENT_DEFAULT };

    void DetachChildFromSlots(const std::shared_ptr<Component>& child);
    void CacheRawDynamicProperty(const std::string& key, const JsonValue& child);
    void ApplyCustomProperties(const JsonValue& descriptor);
    bool IsExtendedEtsExpressionScope() const;
    // Build a new customProps tree with every {{ }} leaf resolved (read-only).
    JsonValue ResolveExpressionsInValue(const JsonValue& node, const std::string& path) const;
    JsonValue ResolveExpressionsInObject(const JsonValue& node, const std::string& path) const;
    JsonValue ResolveExpressionsInArray(const JsonValue& node, const std::string& path) const;
    JsonValue CloneExpressionValue(const JsonValue& node) const;
    void ParseChecks(const JsonValue& descriptor);
    void ParseExtendedTabsChildListDescriptor(const JsonValue& descriptor);
    void ParseTabsChildListDescriptor(const JsonValue& descriptor);
    void ParseRowChildListDescriptor(const JsonValue& descriptor);
    void ParseTabContentChildListDescriptor(const JsonValue& descriptor);
    bool CreateCustomComponent();
    void UpdateCustomComponent();
    napi_value CreateAttributeValue() const;
    void PopulateAttributeIdentity(napi_value attributeValue) const;
    void PopulateAttributeThemeAndCustomProps(napi_value attributeValue) const;
    void PopulateAttributeCommonProperties(napi_value attributeValue) const;
    void PopulateAttributeDataModel(napi_value attributeValue) const;
    JsonValue ParseCheckTargetValue(const std::string& targetJsonLiteral) const;
    JsonValue BuildCustomProps() const;
    const std::map<std::string, JsonValue>& BuildEffectiveCustomProperties(
        std::map<std::string, JsonValue>& effectiveProperties) const;
    void PutTabsCustomProp(JsonValue& customProps, const std::map<std::string, JsonValue>& properties) const;
    void PutChildrenCustomProp(JsonValue& customProps) const;
    void PutNamedCustomProperties(JsonValue& customProps, const std::map<std::string, JsonValue>& properties) const;
    void DisposeComponentContent();
    void ResetReferences();
    void ParseTabsMapping(const JsonValue& descriptor);
    void RegisterDataBindings(const JsonValue& descriptor);
    void ParseListeners(const JsonValue& descriptor);
    void SyncChildSlots(napi_value childSlots);
    void SyncChildSlotEntry(napi_value childSlots, napi_value key);
    void SyncUpdatedChildSlots(napi_value result);
    void ResolveChildSlotValues(napi_value& childSlot, napi_value& childSlots);
    bool InvokeCreateFunction(napi_ref createCustomComponentRef, napi_value& result);
    bool ExtractContentAndCreateRefs(
        napi_value result, ArkUI_NodeHandle& handle, A2UINodeContentHandle& childSlotHandle);
    bool ApplyDynamicCustomProperty(const std::string& key, const JsonValue& child, const JsonValue& descriptor);
    bool IsTabsType() const;
    bool IsExtendedTabsType() const;
    bool IsExtendedProtocolSurface() const;
    bool IsRowType() const;
    std::string GetShortType(const std::string& type) const;
    std::list<std::string> ResolveTabsChildIds(const JsonValue& descriptor) const;
    std::list<std::string> ResolveRowChildIds(const JsonValue& descriptor) const;
    std::list<std::string> ResolveTemplateChildIds(
        const std::string& templateComponentId, const std::string& templatePath) const;
    void MergeTabsFromChildIds(JsonValue& tabsArray) const;
    void MergeRowChildren(JsonValue& childrenArray) const;

    std::map<std::string, JsonValue> CollectTabsProperties(const std::map<std::string, JsonValue>& properties) const;
    JsonValue GetOrCreateTabsArray(const std::map<std::string, JsonValue>& properties) const;
    void UpdateTabsWithProperties(JsonValue& tabsArray, const std::map<std::string, JsonValue>& tabsProperties) const;
    void ResolveFunctionCallsInTabsArray(JsonValue& tabsArray) const;
    void AddTabsToBuilder(std::ostringstream& builder, const JsonValue& tabsArray, bool& hasCustomProp) const;
    void AddOtherPropertiesToBuilder(
        std::ostringstream& builder, const std::map<std::string, JsonValue>& properties, bool& hasCustomProp) const;
    void MergeTabsChildren(JsonValue& childrenArray) const;
    void NormalizeCustomProperty(const std::string& propertyName, JsonValue& value);
    void NormalizeExtendedCommonStyles(JsonValue& value);
    void UpdateFlexShrinkStyleState(const JsonValue& styles);
    void SyncFlexShrinkParentDefaultProperties();
    void NormalizeExtendedTabsProperty(const std::string& propertyName, JsonValue& value);
    void NormalizeExtendedTabContentProperty(const std::string& propertyName, JsonValue& value);
    void NormalizeExtendedTabContentStyles(JsonValue& value);
    void NormalizeExtendedTabContentNumberStyles(JsonValue& value);
    void NormalizeExtendedTabContentStringStyles(JsonValue& value);
    void NormalizeExtendedTabContentFontWeightStyle(JsonValue& value);
    void ReportCustomSchemaWarning(
        const std::string& code, const std::string& message, const std::string& propertyPath) const;
    void ClearDynamicValueCallbacks();
    bool DispatchDynamicValueCallback(const std::string& propertyName, const JsonValue& value);
    void DispatchCurrentDynamicValue(const std::string& propertyName, const std::string& path);
    void SyncDynamicValueBindings();

    struct DynamicValueCallbackInfo {
        napi_env env = nullptr;
        napi_ref callbackRef = nullptr;
        std::string dataPath;
    };

    struct PersistentDynamicValueRegistrationContext {
        const std::string& propertyName;
        const JsonValue& descriptor;
        const ResolvedValue& resolved;
        const DynamicValueDependencies& dependencies;
        const DynamicValueCallbackInfo& callbackInfo;
        std::string* errorMessage = nullptr;
    };

    bool RegisterPathDynamicValueCallback(const std::string& propertyName, const std::string& path,
        const DynamicValueCallbackInfo& callbackInfo, std::string* errorMessage);
    bool RegisterFunctionCallDynamicBindings(
        const std::string& propertyName, const JsonValue& descriptor, const DynamicValueDependencies& dependencies);
    bool RegisterExpressionDynamicBindings(
        const std::string& propertyName, const JsonValue& descriptor, const DynamicValueDependencies& dependencies);
    bool RegisterPersistentDynamicValueCallback(const PersistentDynamicValueRegistrationContext& registration);
    bool DispatchOneShotDynamicValueCallback(const std::string& propertyName, const ResolvedValue& resolved,
        const DynamicValueCallbackInfo& callbackInfo, std::string* errorMessage);
    bool ResolveAndRegisterDynamicValueCallback(const std::string& propertyName, const JsonValue& descriptor,
        const DynamicValueCallbackInfo& callbackInfo, std::string* errorMessage);

    CustomComponentDescriptor descriptor_;
    std::set<std::string> customPropertyNames_;
    std::map<std::string, JsonValue> properties_;
    std::map<std::string, JsonValue> rawDynamicProperties_;
    std::map<std::string, DynamicValueCallbackInfo> dynamicValueCallbacks_;
    std::set<std::string> dynamicResolverBindingKeys_;
    std::unique_ptr<ChecksEngine> checksEngine_;
    mutable JsonValue currentCheckTargetValue_;
    bool hasCreatedCustomComponent_ = false;
    ArkUI_NodeHandle customContentHandle_ = nullptr;
    A2UINodeContentHandle childSlotHandle_ = nullptr;
    std::map<std::string, A2UINodeContentHandle> childSlotHandles_;
    std::map<std::string, std::string> childToSlotMapping_; // childId -> slotKey
    EventHandlerMap eventHandlers_;
    std::vector<std::string> tabChildIds_;
    std::vector<std::string> rowChildIds_;
    napi_env env_ = nullptr;
    napi_ref componentContentRef_ = nullptr;
    napi_ref childSlotRef_ = nullptr;
    std::map<std::string, napi_ref> childSlotRefs_;
    bool preserveDynamicDescriptors_ = false;
    FlexShrinkStyleState flexShrinkStyleState_ = FlexShrinkStyleState::UNSPECIFIED;
};

} // namespace NativeModule

#endif // A2UI_CUSTOM_COMPONENT_H
