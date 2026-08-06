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

#ifndef A2UI_COMPONENT_H
#define A2UI_COMPONENT_H

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "composition/ChildListParser.h"
#include "data/DataBinding.h"
#include "theme/ThemeBase.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "SurfaceContext.h"

namespace NativeModule {

class DataModel;
class BindingEngine;
class Component;
class RenderManager;
class RenderSlot;
class ThemeManager;
class SurfaceSlot;
struct ResolvedValue;

enum class PropertyValueType { STRING = 0, NUMBER, BOOLEAN, ENUM_STRING, OBJECT };

struct PropertyDeclaration {
    std::string name;
    PropertyValueType type = PropertyValueType::STRING;
    bool allowDynamic = false;
    bool allowExpression = false;
    bool acceptNumberForString = false;
    bool resetOnTypeMismatch = false;
    bool reportDynamicNumberRange = false;
    double dynamicNumberMin = 0.0;
    bool dynamicNumberMinExclusive = false;
    std::string fallbackString;
    double fallbackNumber = 0.0;
    bool fallbackBool = false;
    std::vector<std::string> enumAllowed;
    std::string enumFallback;
    std::function<void(const JsonValue&)> applyValue;
};

struct CommonMargin {
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;
    float left = 0.0F;
};

class Component : public std::enable_shared_from_this<Component> {
public:
    explicit Component(ArkUI_NodeHandle nativeView, bool ownsNativeView = true, bool isCompositeType = false);
    virtual ~Component();

    void AddChild(const std::shared_ptr<Component>& child);
    void AddChildAt(const std::shared_ptr<Component>& child, size_t index);
    bool HasChild(const std::shared_ptr<Component>& child) const;
    virtual void RemoveAllChildren();
    void ApplyDescriptor(const JsonValue& descriptor);
    void ApplyParentsRelations(SurfaceSlot* surfaceSlot);
    const std::list<std::shared_ptr<Component>>& GetChildren() const
    {
        return children_;
    }

    void SetComponentId(const std::string& componentId);
    const std::string& GetComponentId() const;

    void SetSurfaceId(const std::string& surfaceId);
    const std::string& GetSurfaceId() const;

    void SetRenderId(int32_t renderId);
    int32_t GetRenderId() const;

    void SetSurfaceContext(const SurfaceContext& surfaceContext);
    const SurfaceContext& GetSurfaceContext() const;

    void SetLocalVariables(const std::map<std::string, JsonValue>& localVariables);
    const std::map<std::string, JsonValue>& GetLocalVariables() const;
    static void RegisterPendingLocalVariablesForComponents(const std::map<std::string, JsonValue>& descriptorsById,
        const std::map<std::string, JsonValue>& localVariables);
    static void ClearPendingLocalVariablesForComponents(const std::map<std::string, JsonValue>& descriptorsById);

    ArkUI_NodeHandle GetNativeView() const;
    ArkUI_NodeHandle GetHandle() const;
    const CommonMargin& GetCommonMargin() const;

    virtual std::string GetType() const;
    void ClearChildren();
    void SetVisibility(A2UIVisibility visibility);

    // DataBind and dataUpdate
    void AddBinding(const std::string& prop, const std::string& path);
    void AddFunctionCallBinding(const std::string& prop, const std::string& path, const JsonValue& descriptor);
    void RemoveBindingsForProperty(const std::string& property);
    void RemoveProperty(const std::string& propertyName);
    virtual void OnDataUpdate(const std::string& property, const JsonValue& value);
    virtual void Render() const;
    virtual void SetMargin(float top, float right, float bottom, float left);
    const std::vector<DataBinding>& GetDataBindings() const;
    void MarkDescriptorDynamicBindingsResolved();
    bool ConsumeDescriptorDynamicBindingsResolved();

    // Theme update interface - called when theme configuration changes
    virtual void OnConfigChange(const ThemeContext& context);

    /**
     * @brief Get theme for the current component type
     * Usage: auto theme = GetTheme();
     * @return Shared pointer to the theme base, or nullptr if not available
     */
    std::shared_ptr<ThemeBase> GetTheme();
    const ChildListDescriptor& GetChildListDescriptor() const;
    void AttachStaticChildrenByIds(
        const std::list<std::string>& childIds, const std::map<std::string, std::shared_ptr<Component>>& allComponents);
    void BuildChildren(SurfaceSlot& surfaceSlot);
    void AttachToParentIfNeeded(
        const std::map<std::string, std::shared_ptr<Component>>& allComponents, std::string& parentId);
    bool IsChildIdsUnchanged(const std::list<std::string>& childIds) const;
    void SetParentId(const std::string& parentId);
    const std::string& GetParentId() const;
    bool IsRootNode() const
    {
        return GetComponentId() == "root";
    }
    void ClearParentId();
    void SetParent(const std::shared_ptr<Component>& parent);
    std::shared_ptr<Component> GetParent() const;
    void ClearParent();
    void SetBuildDepth(int32_t depth);
    int32_t GetBuildDepth() const;
    const std::list<std::string>& GetChildIds() const;
    bool HasChildId(const std::string& childId) const;
    virtual std::string GetRuntimeStateScope() const
    {
        return "";
    }
    virtual std::string GetRuntimeStateKey() const
    {
        return "";
    }
    virtual JsonValue CaptureRuntimeState() const
    {
        return JsonValue();
    }
    virtual void RestoreRuntimeState(const JsonValue& state)
    {
        static_cast<void>(state);
    }

protected:
    static size_t CountNativeDescendantViews(const std::shared_ptr<Component>& node);
    static void CollectNativeDescendantViews(
        const std::shared_ptr<Component>& node, std::vector<ArkUI_NodeHandle>& nativeViews);
    size_t ResolveNativeChildIndex(const Component* target, size_t fallback) const;
    virtual void OnAttachToParent();
    virtual void OnAddChild(const std::shared_ptr<Component>& child, size_t index);
    virtual void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex);
    virtual void OnRemoveChild(const std::shared_ptr<Component>& child);
    virtual void ApplyCommonAttributes(const JsonValue& descriptor);
    virtual void ApplyPrivateAttributes(const JsonValue& descriptor);
    virtual PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName);
    virtual bool HandleSpecialProperty(const std::string& propertyName, const JsonValue& value);
    virtual bool ShouldValidateUnknownDescriptorFields() const;
    virtual bool IsExpressionSupported() const;
    virtual bool IsExpressionCandidate(const JsonValue& value) const;
    virtual std::shared_ptr<DataModel> GetDynamicResolveDataModel() const;
    virtual bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const;
    virtual bool IsKnownNestedDescriptorKey(const std::string& objectName, const std::string& propertyName) const;
    virtual bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds);
    bool ExpandTemplateChildrenEager(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds);
    virtual std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const;
    virtual void ValidateComponentDescriptorSchema(const JsonValue& descriptor);
    virtual bool AcceptsChild(const std::shared_ptr<Component>& child) const;
    bool ResolveDynamicBindingUpdateValue(
        const std::string& property, const JsonValue& value, JsonValue& resolvedValue) const;

    // Common native attributes shared by all components.
    void SetLayoutWeight(float weight);
    void SetAccessibilityLabel(const std::string& label);
    void SetAccessibilityDescription(const std::string& description);
    void SetCommonMargin(float top, float right, float bottom, float left);
    void ValidateChecksSpecialProperty(const JsonValue& value);
    void ValidateActionSpecialProperty(const JsonValue& value);
    std::shared_ptr<ThemeManager> GetThemeManager() const;

    // Child spacing management (for linear layout containers like Row/Column)
    std::shared_ptr<Component> GetChildAtIndex(size_t index) const;
    void ApplyChildSpacingForIndex(size_t index);
    void ClearChildSpacing(const std::shared_ptr<Component>& child);
    void ApplyChildSpacing(const std::shared_ptr<Component>& child, size_t index);
    void RefreshSpacingOnChildAdded(const std::shared_ptr<Component>& child, size_t index);
    void RefreshSpacingOnChildMoved(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex);
    virtual void ApplyMarginToChild(const std::shared_ptr<Component>& child, float startMargin, float endMargin);
    void CalculateChildSpacing(size_t index, size_t totalChildren, float& startMargin, float& endMargin);
    float spacing_ = 0.0F;

    // 浠?descriptor 涓彁鍙栧睘鎬у苟璁剧疆锛堟敮鎸佸璞℃牸寮忕殑鏁版嵁缁戝畾锛?
    void SetPropertyFromDescriptor(
        const std::string& propertyKey, const JsonValue& descriptor, const std::string& bindingKey = "");
    void ApplySchemaProperty(const std::string& propertyKey, const JsonValue& descriptor);

protected:
    virtual void CollectChildListDescriptor(const JsonValue& descriptor);
    virtual void OnPropertyRemoved(const std::string& propertyName);
    virtual void OnPropertyApplied(const std::string& propertyName, const JsonValue& value);
    void ApplyRuntimeProperty(const std::string& property, const JsonValue& value, bool fromDynamicUpdate);
    // Custom component {{ }} expression support: evaluate one expression string (read-only).
    // Returns the type-preserved JsonValue (bool/number/string/object/array); invalid on failure.
    JsonValue EvaluateCustomExpression(const std::string& rawExpr) const;
    void ReportSchemaWarning(
        const std::string& code, const std::string& message, const std::string& propertyPath) const;
    ChildListDescriptor childListDescriptor_;
    ArkUI_NodeHandle nativeView_ = nullptr;
    std::string componentId_;
    std::string surfaceId_;
    int32_t renderId_ = -1;
    SurfaceContext surfaceContext_;
    std::vector<DataBinding> dataBindings_;
    std::string BuildSchemaWarningPath(const std::string& propertyPath) const;
    std::string ResolveSchemaWarningItemName() const;

private:
    struct TemplateChildBuildContext {
        const ChildListDescriptor& childList;
        SurfaceSlot& surfaceSlot;
        const JsonValue& arrayValue;
        const std::map<std::string, JsonValue>& descriptorStore;
        std::list<std::string>& childIds;
    };

    struct PropertyBindingState {
        std::string descriptorKey;
        std::string declarationKey;
        std::string resolvedBindingKey;
        PropertyDeclaration declaration;
        bool hasDeclaration = false;
        bool shouldFallbackOnNullOrEmptyObject = false;
        bool allowExpression = false;
    };

    PropertyDeclaration GetCommonPropertyDeclaration(const std::string& propertyName);
    bool TryGetPropertyDeclaration(const std::string& property, PropertyDeclaration& declaration);
    bool IsKnownDescriptorKey(const std::string& propertyName);
    void ValidateComponentDirectRequiredProperties(const JsonValue& descriptor);
    void ValidateUnknownDescriptorFields(const JsonValue& descriptor);
    void ValidateUnknownObjectFields(
        const JsonValue& objectValue, const std::string& propertyPath, const std::string& objectName);
    void ResolveAndBindProperty(const std::string& descriptorKey, const std::string& declarationKey,
        const std::string& bindingKey, const JsonValue& valueJson);
    bool HandleUnsupportedExpression(const PropertyBindingState& state, const JsonValue& valueJson);
    bool HandleObjectLiteral(const PropertyBindingState& state, const JsonValue& valueJson);
    void HandleResolvedPathBinding(const PropertyBindingState& state, const ResolvedValue& resolved);
    void HandleResolvedFunctionCall(
        const PropertyBindingState& state, const JsonValue& valueJson, const ResolvedValue& resolved);
    void ApplyFunctionCallMissingPathFallback(const PropertyBindingState& state, const JsonValue& valueJson);
    void HandleInvalidPathBinding(const PropertyBindingState& state);
    void HandleResolvedExpression(
        const PropertyBindingState& state, const JsonValue& valueJson, const ResolvedValue& resolved);
    void HandleDirectResolvedValue(
        const PropertyBindingState& state, const JsonValue& valueJson, const ResolvedValue& resolved);
    void SyncExpressionBindings(const std::string& bindingKey, const JsonValue& valueJson);
    bool ResolveEagerTemplateArray(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, JsonValue& arrayValue);
    bool BuildEagerTemplateChild(const TemplateChildBuildContext& context, int32_t itemIndex);
    void ValidateActionEventProperty(const JsonValue& value);
    void ValidateActionFunctionCallProperty(const JsonValue& value);
    void ApplyResolvedPathValue(const std::string& declarationKey, const std::string& resolvedBindingKey,
        const ResolvedValue& resolved, const PropertyDeclaration& declaration, bool shouldFallbackOnNullOrEmptyObject);
    void ApplyResolvedFunctionCallValue(
        const std::string& declarationKey, const std::string& resolvedBindingKey, const ResolvedValue& resolved);
    void SyncPathBinding(const std::string& bindingKey, const std::string& path);
    void SyncFunctionCallBindings(const std::string& bindingKey, const JsonValue& descriptor);
    void ApplyResolvedPropertyValue(
        const std::string& declarationKey, const std::string& bindingKey, const JsonValue& value);
    size_t ResolveDesiredChildOrder(const std::shared_ptr<Component>& child, size_t fallbackOrder) const;
    std::pair<std::list<std::shared_ptr<Component>>::iterator, size_t> ResolveInsertPositionByDesiredOrder(
        size_t desiredOrder, std::list<std::shared_ptr<Component>>::iterator excludeIt);
    size_t ResolveInsertIndexInParent(const std::shared_ptr<Component>& parentNode) const;
    void SetChildIds(const std::list<std::string>& childIds);
    void DetachCurrentChildren();

    std::list<std::shared_ptr<Component>> children_;
    std::unordered_map<std::shared_ptr<Component>, size_t> desiredChildOrder_;
    std::vector<std::unique_ptr<JsonAdapter>> localVariableAdapters_;
    std::map<std::string, JsonValue> localVariables_;
    CommonMargin commonMargin_;
    bool descriptorDynamicBindingsResolved_ = false;
    bool ownsNativeView_ = true;
    bool isCompositeType_ = false;

    std::string parentId_;
    std::weak_ptr<Component> parent_;
    std::list<std::string> childIds_;
    int32_t buildDepth_ = 0;
};

} // namespace NativeModule

#endif // A2UI_COMPONENT_H
