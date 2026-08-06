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

#include "ArkUIConstraintSizeAdapter.h"

#include <array>
#include <cmath>
#include <memory>
#include <unordered_map>

#include "ArkUINodeApiAdapter.h"
#include "ArkUIOHApiAdapter.h"

namespace NativeModule {
namespace {

constexpr float PERCENT_BASE = 100.0F;
constexpr int32_t CUSTOM_EVENT_TARGET_ID = 0;
constexpr int32_t ADAPTER_ERROR = -1;

struct ResolvedConstraintSize {
    float minWidth = 0.0F;
    float maxWidth = FLT_MAX;
    float minHeight = 0.0F;
    float maxHeight = FLT_MAX;
};

struct ConstraintNodeState {
    ArkUI_NodeHandle content = nullptr;
    ArkUI_NodeHandle wrapper = nullptr;
    A2UIConstraintSizeSpec spec;
    ResolvedConstraintSize lastApplied;
    bool active = false;
    bool hasLastApplied = false;
    bool applying = false;
};

ArkUI_NativeNodeAPI_1* GetNativeNodeAPI()
{
    return reinterpret_cast<ArkUI_NativeNodeAPI_1*>(ArkUINodeApiAdapter::GetNativeNodeAPI());
}

float GetDensityPixels()
{
    float densityPixels = 0.0F;
    if (ArkUIOHApiAdapter::GetDefaultDisplayDensityPixels(&densityPixels) != DISPLAY_MANAGER_OK ||
        !std::isfinite(densityPixels) || densityPixels <= 0.0F) {
        return 1.0F;
    }
    return densityPixels;
}

float ResolveConstraint(const A2UIConstraintDimension& dimension, float percentReferenceVp, float unresolvedFallback)
{
    if (!std::isfinite(dimension.value) || dimension.value < 0.0F) {
        return unresolvedFallback;
    }
    if (!dimension.isPercent) {
        return dimension.value;
    }
    if (!std::isfinite(percentReferenceVp) || percentReferenceVp < 0.0F) {
        return unresolvedFallback;
    }
    const double resolved = static_cast<double>(percentReferenceVp) * static_cast<double>(dimension.value) /
                            static_cast<double>(PERCENT_BASE);
    if (!std::isfinite(resolved) || resolved > static_cast<double>(FLT_MAX)) {
        return unresolvedFallback;
    }
    return static_cast<float>(resolved);
}

bool IsSameConstraint(const ResolvedConstraintSize& left, const ResolvedConstraintSize& right)
{
    return left.minWidth == right.minWidth && left.maxWidth == right.maxWidth && left.minHeight == right.minHeight &&
           left.maxHeight == right.maxHeight;
}

bool HasNonZeroFloatValue(const ArkUI_AttributeItem* item)
{
    if (item == nullptr || item->value == nullptr || item->size <= 0) {
        return false;
    }
    for (int32_t index = 0; index < item->size; ++index) {
        if (item->value[index].f32 != 0.0F) {
            return true;
        }
    }
    return false;
}

void CopyAttribute(
    ArkUI_NativeNodeAPI_1* api, ArkUI_NodeHandle source, ArkUI_NodeHandle target, ArkUI_NodeAttributeType attribute)
{
    if (api == nullptr || api->getAttribute == nullptr || api->setAttribute == nullptr) {
        return;
    }
    const ArkUI_AttributeItem* item = api->getAttribute(source, attribute);
    if (item != nullptr) {
        static_cast<void>(api->setAttribute(target, attribute, const_cast<ArkUI_AttributeItem*>(item)));
    }
}

void CopyParentLayoutAttributes(ArkUI_NativeNodeAPI_1* api, ArkUI_NodeHandle content, ArkUI_NodeHandle wrapper)
{
    if (api == nullptr || api->getAttribute == nullptr || api->setAttribute == nullptr) {
        return;
    }
    const ArkUI_AttributeItem* percentMargin = api->getAttribute(content, NODE_MARGIN_PERCENT);
    if (HasNonZeroFloatValue(percentMargin)) {
        static_cast<void>(
            api->setAttribute(wrapper, NODE_MARGIN_PERCENT, const_cast<ArkUI_AttributeItem*>(percentMargin)));
    } else {
        CopyAttribute(api, content, wrapper, NODE_MARGIN);
    }
    CopyAttribute(api, content, wrapper, NODE_FLEX_SHRINK);
    CopyAttribute(api, content, wrapper, NODE_LAYOUT_WEIGHT);
}

int32_t FindChildIndex(ArkUI_NativeNodeAPI_1* api, ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    if (api == nullptr || api->getTotalChildCount == nullptr || api->getChildAt == nullptr) {
        return ADAPTER_ERROR;
    }
    const uint32_t childCount = api->getTotalChildCount(parent);
    for (uint32_t index = 0; index < childCount; ++index) {
        if (api->getChildAt(parent, static_cast<int32_t>(index)) == child) {
            return static_cast<int32_t>(index);
        }
    }
    return ADAPTER_ERROR;
}

int32_t AttachContentToWrapper(ArkUI_NativeNodeAPI_1* api, ConstraintNodeState& state)
{
    ArkUI_NodeHandle parent = api->getParent(state.content);
    if (parent == nullptr) {
        return api->addChild(state.wrapper, state.content);
    }

    const int32_t childIndex = FindChildIndex(api, parent, state.content);
    if (childIndex < 0) {
        return ADAPTER_ERROR;
    }
    int32_t result = api->removeChild(parent, state.content);
    if (result != A2UI_ERROR_CODE_NO_ERROR) {
        return result;
    }
    result = api->addChild(state.wrapper, state.content);
    if (result != A2UI_ERROR_CODE_NO_ERROR) {
        static_cast<void>(api->insertChildAt(parent, state.content, childIndex));
        return result;
    }
    result = api->insertChildAt(parent, state.wrapper, childIndex);
    if (result != A2UI_ERROR_CODE_NO_ERROR) {
        static_cast<void>(api->removeChild(state.wrapper, state.content));
        static_cast<void>(api->insertChildAt(parent, state.content, childIndex));
    }
    return result;
}

bool HasRequiredNodeLifecycleApi(const ArkUI_NativeNodeAPI_1& api)
{
    return api.createNode != nullptr && api.disposeNode != nullptr && api.addChild != nullptr &&
           api.removeChild != nullptr && api.insertChildAt != nullptr && api.getParent != nullptr &&
           api.getTotalChildCount != nullptr && api.getChildAt != nullptr;
}

bool HasRequiredCustomLayoutApi(const ArkUI_NativeNodeAPI_1& api)
{
    return api.registerNodeCustomEvent != nullptr && api.unregisterNodeCustomEvent != nullptr &&
           api.addNodeCustomEventReceiver != nullptr && api.removeNodeCustomEventReceiver != nullptr &&
           api.setMeasuredSize != nullptr && api.setLayoutPosition != nullptr && api.getMeasuredSize != nullptr &&
           api.measureNode != nullptr && api.layoutNode != nullptr;
}

class ConstraintSizeManager final {
public:
    static ConstraintSizeManager& GetInstance()
    {
        static ConstraintSizeManager instance;
        return instance;
    }

    int32_t Set(ArkUI_NodeHandle node, const A2UIConstraintSizeSpec& spec)
    {
        if (node == nullptr) {
            return ADAPTER_ERROR;
        }
        ArkUI_NodeHandle content = GetContentNode(node);
        auto state = nodeStates_.find(content);
        if (state == nodeStates_.end()) {
            int32_t createResult = CreateState(content);
            if (createResult != A2UI_ERROR_CODE_NO_ERROR) {
                return createResult;
            }
            state = nodeStates_.find(content);
        }
        state->second->spec = spec;
        state->second->active = true;
        state->second->hasLastApplied = false;
        MarkMeasureDirty(*state->second);
        return A2UI_ERROR_CODE_NO_ERROR;
    }

    void Clear(ArkUI_NodeHandle node)
    {
        ArkUI_NodeHandle content = GetContentNode(node);
        auto state = nodeStates_.find(content);
        if (state == nodeStates_.end()) {
            return;
        }
        state->second->active = false;
        state->second->hasLastApplied = false;
        MarkMeasureDirty(*state->second);
    }

    void Dispose(ArkUI_NodeHandle node)
    {
        ArkUI_NodeHandle content = GetContentNode(node);
        auto state = nodeStates_.find(content);
        if (state == nodeStates_.end()) {
            return;
        }
        ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPI();
        ConstraintNodeState* nodeState = state->second.get();
        wrapperStates_.erase(nodeState->wrapper);
        if (api != nullptr) {
            if (api->unregisterNodeCustomEvent != nullptr) {
                api->unregisterNodeCustomEvent(nodeState->wrapper, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
                api->unregisterNodeCustomEvent(nodeState->wrapper, ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT);
            }
            if (api->removeNodeCustomEventReceiver != nullptr) {
                static_cast<void>(api->removeNodeCustomEventReceiver(nodeState->wrapper, HandleCustomEvent));
            }
            if (api->getParent != nullptr && api->removeChild != nullptr) {
                ArkUI_NodeHandle parent = api->getParent(nodeState->wrapper);
                if (parent != nullptr) {
                    static_cast<void>(api->removeChild(parent, nodeState->wrapper));
                }
                static_cast<void>(api->removeChild(nodeState->wrapper, nodeState->content));
            }
            if (api->disposeNode != nullptr) {
                api->disposeNode(nodeState->wrapper);
            }
        }
        nodeStates_.erase(state);
    }

    ArkUI_NodeHandle GetMountNode(ArkUI_NodeHandle node) const
    {
        if (node == nullptr || wrapperStates_.find(node) != wrapperStates_.end()) {
            return node;
        }
        auto state = nodeStates_.find(node);
        return state == nodeStates_.end() ? node : state->second->wrapper;
    }

    ArkUI_NodeHandle GetContentNode(ArkUI_NodeHandle node) const
    {
        auto state = wrapperStates_.find(node);
        return state == wrapperStates_.end() ? node : state->second->content;
    }

private:
    static void HandleCustomEvent(ArkUI_NodeCustomEvent* event)
    {
        if (event == nullptr) {
            return;
        }
        auto* state = static_cast<ConstraintNodeState*>(ArkUIOHApiAdapter::NodeCustomEventGetUserData(event));
        if (state == nullptr || ArkUIOHApiAdapter::NodeCustomEventGetNodeHandle(event) != state->wrapper) {
            return;
        }
        const int32_t eventType = ArkUIOHApiAdapter::NodeCustomEventGetEventType(event);
        if (eventType == static_cast<int32_t>(ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE)) {
            GetInstance().Measure(*state, event);
        } else if (eventType == static_cast<int32_t>(ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT)) {
            GetInstance().Layout(*state, event);
        }
    }

    int32_t CreateState(ArkUI_NodeHandle content)
    {
        ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPI();
        if (api == nullptr || !HasRequiredNodeLifecycleApi(*api) || !HasRequiredCustomLayoutApi(*api)) {
            return ADAPTER_ERROR;
        }

        auto state = std::make_unique<ConstraintNodeState>();
        state->content = content;
        state->wrapper = api->createNode(ARKUI_NODE_CUSTOM);
        if (state->wrapper == nullptr) {
            return ADAPTER_ERROR;
        }
        int32_t result = RegisterEvents(api, *state);
        if (result == A2UI_ERROR_CODE_NO_ERROR) {
            CopyParentLayoutAttributes(api, state->content, state->wrapper);
            result = AttachContentToWrapper(api, *state);
        }
        if (result != A2UI_ERROR_CODE_NO_ERROR) {
            UnregisterEvents(api, *state);
            api->disposeNode(state->wrapper);
            return result;
        }
        ConstraintNodeState* statePointer = state.get();
        nodeStates_.emplace(content, std::move(state));
        wrapperStates_.emplace(statePointer->wrapper, statePointer);
        return A2UI_ERROR_CODE_NO_ERROR;
    }

    int32_t RegisterEvents(ArkUI_NativeNodeAPI_1* api, ConstraintNodeState& state)
    {
        int32_t result = api->addNodeCustomEventReceiver(state.wrapper, HandleCustomEvent);
        if (result != A2UI_ERROR_CODE_NO_ERROR) {
            return result;
        }
        result = api->registerNodeCustomEvent(
            state.wrapper, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE, CUSTOM_EVENT_TARGET_ID, &state);
        if (result != A2UI_ERROR_CODE_NO_ERROR) {
            static_cast<void>(api->removeNodeCustomEventReceiver(state.wrapper, HandleCustomEvent));
            return result;
        }
        result = api->registerNodeCustomEvent(
            state.wrapper, ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT, CUSTOM_EVENT_TARGET_ID, &state);
        if (result != A2UI_ERROR_CODE_NO_ERROR) {
            api->unregisterNodeCustomEvent(state.wrapper, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
            static_cast<void>(api->removeNodeCustomEventReceiver(state.wrapper, HandleCustomEvent));
        }
        return result;
    }

    void UnregisterEvents(ArkUI_NativeNodeAPI_1* api, const ConstraintNodeState& state)
    {
        api->unregisterNodeCustomEvent(state.wrapper, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
        api->unregisterNodeCustomEvent(state.wrapper, ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT);
        static_cast<void>(api->removeNodeCustomEventReceiver(state.wrapper, HandleCustomEvent));
    }

    void MarkMeasureDirty(const ConstraintNodeState& state)
    {
        ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPI();
        if (api != nullptr && api->markDirty != nullptr) {
            api->markDirty(state.wrapper, NODE_NEED_MEASURE);
        }
    }

    void ApplyConstraint(ConstraintNodeState& state, int32_t widthReferencePx, int32_t heightReferencePx)
    {
        if (!state.active || state.applying) {
            return;
        }
        const float densityPixels = GetDensityPixels();
        const float widthReferenceVp =
            widthReferencePx < 0 ? -1.0F : static_cast<float>(widthReferencePx) / densityPixels;
        const float heightReferenceVp =
            heightReferencePx < 0 ? -1.0F : static_cast<float>(heightReferencePx) / densityPixels;
        ResolvedConstraintSize resolved {
            .minWidth = ResolveConstraint(state.spec.minWidth, widthReferenceVp, 0.0F),
            .maxWidth = ResolveConstraint(state.spec.maxWidth, widthReferenceVp, FLT_MAX),
            .minHeight = ResolveConstraint(state.spec.minHeight, heightReferenceVp, 0.0F),
            .maxHeight = ResolveConstraint(state.spec.maxHeight, heightReferenceVp, FLT_MAX),
        };
        if (state.hasLastApplied && IsSameConstraint(state.lastApplied, resolved)) {
            return;
        }
        state.applying = true;
        int32_t result = ArkUINodeApiAdapter::SetNodeConstraintSize(
            state.content, resolved.minWidth, resolved.maxWidth, resolved.minHeight, resolved.maxHeight);
        state.applying = false;
        if (result == A2UI_ERROR_CODE_NO_ERROR) {
            state.lastApplied = resolved;
            state.hasLastApplied = true;
        }
    }

    void Measure(ConstraintNodeState& state, ArkUI_NodeCustomEvent* event)
    {
        ArkUI_LayoutConstraint* constraint = ArkUIOHApiAdapter::NodeCustomEventGetLayoutConstraintInMeasure(event);
        ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPI();
        if (constraint == nullptr || api == nullptr) {
            return;
        }
        ApplyConstraint(state, ArkUIOHApiAdapter::LayoutConstraintGetPercentReferenceWidth(constraint),
            ArkUIOHApiAdapter::LayoutConstraintGetPercentReferenceHeight(constraint));
        if (api->measureNode(state.content, constraint) != A2UI_ERROR_CODE_NO_ERROR) {
            return;
        }
        const ArkUI_IntSize contentSize = api->getMeasuredSize(state.content);
        static_cast<void>(api->setMeasuredSize(state.wrapper, contentSize.width, contentSize.height));
    }

    void Layout(ConstraintNodeState& state, ArkUI_NodeCustomEvent* event)
    {
        ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPI();
        if (api == nullptr) {
            return;
        }
        const ArkUI_IntOffset position = ArkUIOHApiAdapter::NodeCustomEventGetPositionInLayout(event);
        static_cast<void>(api->setLayoutPosition(state.wrapper, position.x, position.y));
        static_cast<void>(api->layoutNode(state.content, 0, 0));
    }

    std::unordered_map<ArkUI_NodeHandle, std::unique_ptr<ConstraintNodeState>> nodeStates_;
    std::unordered_map<ArkUI_NodeHandle, ConstraintNodeState*> wrapperStates_;
};

} // namespace

int32_t ArkUIConstraintSizeAdapter::SetPercentConstraintSize(ArkUI_NodeHandle node, const A2UIConstraintSizeSpec& spec)
{
    return ConstraintSizeManager::GetInstance().Set(node, spec);
}

void ArkUIConstraintSizeAdapter::Clear(ArkUI_NodeHandle node)
{
    ConstraintSizeManager::GetInstance().Clear(node);
}

void ArkUIConstraintSizeAdapter::Dispose(ArkUI_NodeHandle node)
{
    ConstraintSizeManager::GetInstance().Dispose(node);
}

ArkUI_NodeHandle ArkUIConstraintSizeAdapter::GetMountNode(ArkUI_NodeHandle node)
{
    return ConstraintSizeManager::GetInstance().GetMountNode(node);
}

ArkUI_NodeHandle ArkUIConstraintSizeAdapter::GetContentNode(ArkUI_NodeHandle node)
{
    return ConstraintSizeManager::GetInstance().GetContentNode(node);
}

} // namespace NativeModule
