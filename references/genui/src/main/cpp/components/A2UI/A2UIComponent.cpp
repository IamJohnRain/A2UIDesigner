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

#include "A2UIComponent.h"

#include "composition/TemplateAdapterNode.h"
#include "data/DataModel.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

A2UIComponent::A2UIComponent(ArkUI_NodeHandle nativeView, bool ownsNativeView, bool isCompositeType)
    : Component(nativeView, ownsNativeView, isCompositeType)
{
    ArkUINodeApiAdapter::SetUserData(nativeView_, this);
    ArkUINodeApiAdapter::AddNodeEventReceiver(nativeView_, A2UIComponent::NodeEventReceiver);
}

A2UIComponent::~A2UIComponent()
{
    if (clickEventRegistered_) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(nativeView_, A2UINodeEventType::ON_CLICK);
    }
    for (const auto& [eventType, _] : nodeEventHandlers_) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(nativeView_, eventType);
    }
    ArkUINodeApiAdapter::RemoveNodeEventReceiver(nativeView_, A2UIComponent::NodeEventReceiver);
}

void A2UIComponent::SetWidth(float width)
{
    ArkUINodeApiAdapter::SetNodeWidth(nativeView_, width);
}

void A2UIComponent::SetHeight(float height)
{
    ArkUINodeApiAdapter::SetNodeHeight(nativeView_, height);
}

void A2UIComponent::SetWidthPercent(float percent)
{
    ArkUINodeApiAdapter::SetNodeWidthPercent(nativeView_, percent);
}

void A2UIComponent::SetHeightPercent(float percent)
{
    ArkUINodeApiAdapter::SetNodeHeightPercent(nativeView_, percent);
}

void A2UIComponent::SetBackgroundColor(uint32_t color)
{
    ArkUINodeApiAdapter::SetNodeBackgroundColor(nativeView_, color);
}

void A2UIComponent::SetPadding(float top, float right, float bottom, float left)
{
    ArkUINodeApiAdapter::SetNodePadding(nativeView_, top, right, bottom, left);
}

void A2UIComponent::SetPaddingPercent(float top, float right, float bottom, float left)
{
    ArkUINodeApiAdapter::SetNodePaddingPercent(nativeView_, top, right, bottom, left);
}

void A2UIComponent::SetMargin(float top, float right, float bottom, float left)
{
    SetCommonMargin(top, right, bottom, left);
    ArkUINodeApiAdapter::SetNodeMargin(nativeView_, top, right, bottom, left);
}

void A2UIComponent::ResetCommonMargin()
{
    SetCommonMargin(0.0F, 0.0F, 0.0F, 0.0F);
}

void A2UIComponent::SetMarginPercent(float top, float right, float bottom, float left)
{
    ArkUINodeApiAdapter::SetNodeMarginPercent(nativeView_, top, right, bottom, left);
}

void A2UIComponent::SetBorderRadius(float radius)
{
    ArkUINodeApiAdapter::SetNodeBorderRadius(nativeView_, radius);
}

void A2UIComponent::SetBorderRadiusPercent(float topLeft, float topRight, float bottomLeft, float bottomRight)
{
    ArkUINodeApiAdapter::SetNodeBorderRadiusPercent(nativeView_, topLeft, topRight, bottomLeft, bottomRight);
}

void A2UIComponent::SetBorderWidthPercent(float width)
{
    ArkUINodeApiAdapter::SetNodeBorderWidthPercent(nativeView_, width);
}

int32_t A2UIComponent::GetNativeNodeUniqueId() const
{
    int32_t uniqueId = -1;
    if (ArkUIOHApiAdapter::GetNodeUniqueId(nativeView_, &uniqueId) != A2UI_ERROR_CODE_NO_ERROR) {
        return -1;
    }
    return uniqueId;
}

void A2UIComponent::RegisterOnClick(const std::function<void()>& onClick)
{
    if (onClick == nullptr) {
        RegisterOnClickWithContext(nullptr);
        return;
    }
    RegisterOnClickWithContext([onClick](const JsonValue&) { onClick(); });
}

void A2UIComponent::RegisterOnClickWithContext(const ClickHandler& onClick)
{
    onClick_ = onClick;
    UpdateClickEventRegistration();
}

void A2UIComponent::SetAuxiliaryOnClick(const std::string& key, const std::function<void()>& onClick)
{
    if (key.empty()) {
        return;
    }

    if (onClick == nullptr) {
        auxiliaryOnClick_.erase(key);
    } else {
        auxiliaryOnClick_[key] = [onClick](const JsonValue&) { onClick(); };
    }
    UpdateClickEventRegistration();
}

void A2UIComponent::RemoveAuxiliaryOnClick(const std::string& key)
{
    if (key.empty()) {
        return;
    }

    auxiliaryOnClick_.erase(key);
    UpdateClickEventRegistration();
}

void A2UIComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    if (nativeView_ == nullptr || child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::InsertChildAt(nativeView_, child->GetNativeView(), static_cast<int32_t>(index));
}

void A2UIComponent::OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    if (nativeView_ == nullptr || child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }
    static_cast<void>(currentIndex);
    ArkUINodeApiAdapter::InsertChildAt(nativeView_, child->GetNativeView(), static_cast<int32_t>(targetIndex));
}

void A2UIComponent::ApplyCommonAttributes(const JsonValue& descriptor)
{
    Component::ApplyCommonAttributes(descriptor);

    if (descriptor.Has("widthPercent")) {
        SetWidthPercent(static_cast<float>(descriptor.GetNumber("widthPercent", 0.0)));
    }
    if (descriptor.Has("heightPercent")) {
        SetHeightPercent(static_cast<float>(descriptor.GetNumber("heightPercent", 0.0)));
    }

    uint32_t backgroundColor = descriptor.GetUint32("backgroundColor", 0);
    if (backgroundColor != 0) {
        SetBackgroundColor(backgroundColor);
    }

    bool hasPadding = descriptor.Has("paddingTop") || descriptor.Has("paddingRight") ||
                      descriptor.Has("paddingBottom") || descriptor.Has("paddingLeft");
    if (hasPadding) {
        SetPadding(static_cast<float>(descriptor.GetNumber("paddingTop", 0.0)),
            static_cast<float>(descriptor.GetNumber("paddingRight", 0.0)),
            static_cast<float>(descriptor.GetNumber("paddingBottom", 0.0)),
            static_cast<float>(descriptor.GetNumber("paddingLeft", 0.0)));
    }

    bool hasMargin = descriptor.Has("marginTop") || descriptor.Has("marginRight") || descriptor.Has("marginBottom") ||
                     descriptor.Has("marginLeft");
    if (hasMargin) {
        SetMargin(static_cast<float>(descriptor.GetNumber("marginTop", 0.0)),
            static_cast<float>(descriptor.GetNumber("marginRight", 0.0)),
            static_cast<float>(descriptor.GetNumber("marginBottom", 0.0)),
            static_cast<float>(descriptor.GetNumber("marginLeft", 0.0)));
    }

    if (descriptor.Has("borderRadius")) {
        SetBorderRadius(static_cast<float>(descriptor.GetNumber("borderRadius", 0.0)));
    }
}

bool A2UIComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    return false;
}

void A2UIComponent::NodeEventReceiver(A2UINodeEvent* event)
{
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    auto* node = reinterpret_cast<A2UIComponent*>(ArkUINodeApiAdapter::GetUserData(nodeHandle));
    if (node != nullptr) {
        node->HandleNodeEvent(event);
    }
}

void A2UIComponent::HandleNodeEvent(A2UINodeEvent* event)
{
    auto eventType = ArkUIOHApiAdapter::NodeEventGetEventType(event);
    switch (eventType) {
        case A2UINodeEventType::ON_CLICK:
        case A2UINodeEventType::ON_CLICK_EVENT: {
            JsonValue clickContext = BuildClickContext(event);
            if (onClick_ != nullptr) {
                onClick_(clickContext);
            }
            for (const auto& [_, onClick] : auxiliaryOnClick_) {
                if (onClick != nullptr) {
                    onClick(clickContext);
                }
            }
            return;
        }
        default: {
            auto handlerIt = nodeEventHandlers_.find(eventType);
            if (handlerIt != nodeEventHandlers_.end() && handlerIt->second != nullptr) {
                handlerIt->second();
            }
            return;
        }
    }
}

JsonValue A2UIComponent::BuildClickContext(A2UINodeEvent* event)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }

    double x = 0.0;
    double y = 0.0;
    A2UINodeComponentEvent* componentEvent = ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
    if (componentEvent != nullptr) {
        x = static_cast<double>(componentEvent->data[0].f32);
        y = static_cast<double>(componentEvent->data[1].f32);
    }

    JsonValue root = adapter->GetRoot();
    root.PutNumber("x", x);
    root.PutNumber("y", y);
    return root;
}

bool A2UIComponent::HasClickHandler() const
{
    return onClick_ != nullptr || !auxiliaryOnClick_.empty();
}

void A2UIComponent::UpdateClickEventRegistration()
{
    if (nativeView_ == nullptr) {
        return;
    }

    bool shouldRegister = HasClickHandler();
    if (shouldRegister && !clickEventRegistered_) {
        ArkUINodeApiAdapter::RegisterNodeEvent(nativeView_, A2UINodeEventType::ON_CLICK, 0, this);
        clickEventRegistered_ = true;
        return;
    }

    if (!shouldRegister && clickEventRegistered_) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(nativeView_, A2UINodeEventType::ON_CLICK);
        clickEventRegistered_ = false;
    }
}

void A2UIComponent::RegisterNodeEventHandler(A2UINodeEventType eventType, const std::function<void()>& handler)
{
    if (nativeView_ == nullptr) {
        return;
    }

    auto handlerIt = nodeEventHandlers_.find(eventType);
    bool hasRegistered = handlerIt != nodeEventHandlers_.end();
    if (handler == nullptr) {
        if (hasRegistered) {
            ArkUINodeApiAdapter::UnregisterNodeEvent(nativeView_, eventType);
            nodeEventHandlers_.erase(handlerIt);
        }
        return;
    }

    if (!hasRegistered) {
        ArkUINodeApiAdapter::RegisterNodeEvent(nativeView_, eventType, 0, this);
    }
    nodeEventHandlers_[eventType] = handler;
}

bool A2UIComponent::RefreshLazyAdapterFromDataModel()
{
    auto adapter = GetLazyAdapter();
    if (adapter == nullptr) {
        return false;
    }

    auto dataModel = adapter->GetDataModel();
    const std::string& dataPath = adapter->GetDataPath();
    if (dataModel == nullptr || dataPath.empty() || dataPath[0] != '/') {
        LOG_A2UI(LOG_WARN, "RefreshLazyAdapterFromDataModel: invalid data model or path: %{public}s", dataPath.c_str());
        return false;
    }

    int32_t itemCount = 0;
    auto arrayOpt = dataModel->GetNode(dataPath);
    if (arrayOpt.has_value()) {
        JsonValue arrayValue = arrayOpt.value();
        if (arrayValue.IsArray()) {
            itemCount = static_cast<int32_t>(arrayValue.GetArraySize());
        } else {
            LOG_A2UI(LOG_WARN, "RefreshLazyAdapterFromDataModel: data path is not array: %{public}s", dataPath.c_str());
        }
    }

    adapter->UpdateItemCount(itemCount);
    adapter->ReloadAllItems();
    LOG_A2UI(LOG_DEBUG, "RefreshLazyAdapterFromDataModel: refreshed itemCount=%{public}d, path=%{public}s", itemCount,
        dataPath.c_str());
    return true;
}

} // namespace NativeModule
