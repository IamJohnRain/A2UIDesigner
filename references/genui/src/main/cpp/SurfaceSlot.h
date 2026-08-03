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

#ifndef A2UI_SURFACE_SLOT_H
#define A2UI_SURFACE_SLOT_H

#include <cstddef>
#include <functional>
#include <js_native_api.h>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "adapter/A2UIArkUITypes.h"
#include "catalog/Catalog.h"
#include "components/A2UI/modal/ModalCoordinator.h"
#include "components/Component.h"
#include "utils/JsonAdapter.h"

#include "SurfaceContext.h"

namespace NativeModule {

class BindingEngine;
struct ChildListDescriptor;
struct BuildWorkingSet;
struct RenderContext;
class ThemeManager;
class DataModel;

enum class SurfaceProtocolMode { UNKNOWN = 0, A2UI_STANDARD = 1, EXTENDED_PROTOCOL = 2 };

class SurfaceSlot {
public:
    struct BuildNodeDepthComparator {
        bool operator()(const std::shared_ptr<Component>& lhs, const std::shared_ptr<Component>& rhs) const;
    };

    SurfaceSlot();
    ~SurfaceSlot();

    void SetContentHandle(A2UINodeContentHandle handle);
    A2UINodeContentHandle GetContentHandle() const;

    void SetRootComponent(const std::shared_ptr<Component>& rootComponent);
    std::shared_ptr<Component> GetRootComponent() const;
    void DismissActiveModal();

    bool UpdateComponents(const JsonValue& messageBody);
    bool UpdateDataModel(const JsonValue& messageBody);
    std::shared_ptr<BindingEngine> GetBindingEngine() const;

    void SetSurfaceId(const std::string& surfaceId);
    const std::string& GetSurfaceId() const;

    /**
     * Initialize ThemeManager with theme context
     * Should be called after SetSurfaceId
     * @param context The theme context
     */
    void InitializeThemeManager(const ThemeContext& context);

    void Dispose();

    void SetCatalog(const std::shared_ptr<Catalog>& catalog);
    std::shared_ptr<Catalog> GetCatalog() const;
    void SetSurfaceCatalogId(const std::string& catalogId);
    void SetSurfaceContext(const SurfaceContext& surfaceContext);
    const SurfaceContext& GetSurfaceContext() const;

    void SetRenderId(int32_t renderId);
    int32_t GetRenderId() const;
    void SetForceRootFill(bool forceFill);
    std::shared_ptr<Component> FindComponentById(const std::string& componentId) const;
    std::vector<std::shared_ptr<Component>> GetAllComponents() const;
    template<typename Visitor>
    void ForEachComponent(Visitor visitor) const
    {
        for (const auto& entry : allComponents_) {
            if (entry.second != nullptr) {
                visitor(entry.second);
            }
        }
    }
    void SetFontSizeScale(float scale);
    float GetFontSizeScale() const;
    void SetApiVersion(int32_t apiVersion);
    int32_t GetApiVersion() const;
    void StoreRuntimeState(const std::string& scope, const std::string& key, const JsonValue& state);
    bool GetRuntimeState(const std::string& scope, const std::string& key, JsonValue& state) const;
    void ForEachRuntimeState(
        const std::string& scope, const std::function<void(const std::string&, const JsonValue&)>& visitor) const;
    void ClearRuntimeStateStore();
    void CaptureRuntimeStateTree(const std::shared_ptr<Component>& component);
    void RestoreRuntimeStateTree(const std::shared_ptr<Component>& component) const;

    // Get all components map
    std::map<std::string, std::shared_ptr<Component>>& GetAllComponents();

    /**
     * Get the ThemeManager for this SurfaceSlot
     * @return Shared pointer to the ThemeManager
     */
    std::shared_ptr<ThemeManager> GetThemeManager() const
    {
        return themeManager_;
    }
    std::map<std::string, std::string>& GetParentsRelations();
    std::map<std::string, JsonValue>& GetDescriptorsById();
    const std::map<std::string, JsonValue>& GetAllComponentDescriptorStore() const;
    std::shared_ptr<Component> CreateOrUpdateComponentNode(
        const JsonValue& nodeValue, const std::string& nodeId, const std::string& componentType);
    std::shared_ptr<DataModel> GetOrCreateDataModel();

    std::shared_ptr<Component> BuildRootFromComponents(const std::string& rootId,
        const std::map<std::string, JsonValue>& descriptorsById, bool& hasProcessedNode, bool& sawRootDescriptor,
        bool updateSurfaceRoot = true);
    bool IsExtendedProtocolSurface() const;
#ifdef TDD_BUILD
    int32_t ResolveBuildDepthForTest(const std::string& nodeId) const;
#endif

    void OnTemplateExpansionDeferred(const std::string& containerId);
    void OnTemplateExpansionResolved(const std::string& containerId);

private:
    std::shared_ptr<Component> BuildComponent(const std::string& componentType);
    std::shared_ptr<Component> BuildExtendedComponent(const std::string& componentType) const;
    void ApplyExtendedComponentDescriptor(const JsonValue& nodeValue, const std::shared_ptr<Component>& node,
        bool isNewNode, const RenderContext& renderContext) const;
    std::string ResolveProtocolCatalogId() const;
    void UpdateSurfaceProtocolMode();
    bool UpdateComponentsArray(const JsonValue& componentsValue);
    std::shared_ptr<Component> BuildRootFromComponents(
        const JsonValue& componentsValue, bool& hasProcessedNode, bool& sawRootDescriptor);
    void DebugPrintGlobalMapsInternal() const;
    void PrepareDescriptorById(const JsonValue& componentsValue);
    std::shared_ptr<Component> GetOrCreateComponentNode(
        const JsonValue& nodeValue, const std::string& nodeId, const std::string& componentType, bool& isNewNode);
    void RegisterComponentIfNeeded(const std::shared_ptr<Component>& node, bool isNewNode) const;
    void RefreshLazyAdapters(const std::string& changedPath, bool refreshAll = false);
    void BuildComponentTree(const std::set<std::shared_ptr<Component>, BuildNodeDepthComparator>& buildNodes);
    void SyncExtendedCheckboxGroupState();
    void ClearMountedChildren();
    void RemoveOldChildren(const std::string& parentId, const std::shared_ptr<Component>& parentNode);
    void AttachStaticChildren(const std::string& parentId, const std::shared_ptr<Component>& parentNode,
        const std::list<std::string>& childIds);
    void AttachChildToParent(const std::string& parentId, const std::shared_ptr<Component>& parentNode,
        const std::string& childId, const std::shared_ptr<Component>& childNode);
    std::optional<JsonValue> GetTemplateArrayValue(const std::string& parentId, const std::string& templatePath) const;
    void ExpandTemplateChildrenEager(const std::string& parentId, const std::string& componentType,
        const ChildListDescriptor& childList, const std::shared_ptr<Component>& parentNode,
        BuildWorkingSet& workingSet);
    void SetupLazyListAdapter(const std::string& parentId, const ChildListDescriptor& childList,
        const std::shared_ptr<Component>& parentNode, BuildWorkingSet& workingSet);

    void ProcessPendingTemplateContainers();

    A2UINodeContentHandle contentHandle_ = nullptr;
    std::shared_ptr<Catalog> catalog_;
    SurfaceContext surfaceContext_;
    std::shared_ptr<Component> rootComponent_;
    std::map<std::string, std::shared_ptr<Component>> allComponents_;
    std::map<std::string, std::list<std::string>> childrenRelations_;
    std::map<std::string, std::string> parentsRelations_; // childId -> parentId (cross-batch relation index)
    std::map<std::string, JsonValue> descriptorsById_;
    std::map<std::string, JsonValue> allComponentDescriptorStore_;
    std::map<std::string, std::map<std::string, JsonValue>> runtimeStateStore_;

    // 数据绑定引擎
    std::shared_ptr<BindingEngine> bindingEngine_;

    // Surface ID 用于标识不同的数据模型
    std::string surfaceId_;
    std::string surfaceCatalogId_;
    bool hasSurfaceCatalogId_ = false;
    std::unique_ptr<ModalCoordinator> modalCoordinator_;

    // Render ID 用于标识所属的 RenderSlot
    int32_t renderId_ = -1;

    // NAPI environment
    napi_env env_ = nullptr;

    // 当前渲染区域尺寸（vp），用于 root 默认尺寸策略
    float viewportWidthVp_ = 0.0F;
    float viewportHeightVp_ = 0.0F;
    bool forceRootFill_ = false;
    float fontSizeScale_ = 1.0F;
    int32_t apiVersion_ = 0;

    SurfaceProtocolMode surfaceProtocolMode_ = SurfaceProtocolMode::UNKNOWN;

    std::set<std::string> pendingTemplateContainers_;

    // Theme Manager for this SurfaceSlot
    std::shared_ptr<ThemeManager> themeManager_;
};

} // namespace NativeModule

#endif // A2UI_SURFACE_SLOT_H
