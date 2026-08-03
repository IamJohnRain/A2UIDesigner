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

#ifndef A2UI_ARKUI_COMPONENT_H
#define A2UI_ARKUI_COMPONENT_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include "../Component.h"
#include "ArkUINodeApiAdapter.h"

namespace NativeModule {

class TemplateAdapterNode;

class A2UIComponent : public Component {
public:
    using ClickHandler = std::function<void(const JsonValue&)>;

    explicit A2UIComponent(ArkUI_NodeHandle nativeView, bool ownsNativeView = true, bool isCompositeType = false);
    ~A2UIComponent() override;

    virtual std::shared_ptr<TemplateAdapterNode> GetLazyAdapter() const
    {
        return nullptr;
    }
    bool RefreshLazyAdapterFromDataModel();

    void SetWidth(float width);
    void SetHeight(float height);
    void SetWidthPercent(float percent);
    void SetHeightPercent(float percent);
    void SetBackgroundColor(uint32_t color);
    void SetPadding(float top, float right, float bottom, float left);
    void SetPaddingPercent(float top, float right, float bottom, float left);
    void SetMargin(float top, float right, float bottom, float left);
    void ResetCommonMargin();
    void SetMarginPercent(float top, float right, float bottom, float left);
    void SetBorderRadius(float radius);
    void SetBorderRadiusPercent(float topLeft, float topRight, float bottomLeft, float bottomRight);
    void SetBorderWidthPercent(float width);
    void RegisterOnClick(const std::function<void()>& onClick);
    void RegisterOnClickWithContext(const ClickHandler& onClick);
    void SetAuxiliaryOnClick(const std::string& key, const std::function<void()>& onClick);
    void RemoveAuxiliaryOnClick(const std::string& key);

protected:
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override;
    void ApplyCommonAttributes(const JsonValue& descriptor) override;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
    int32_t GetNativeNodeUniqueId() const;
    void RegisterNodeEventHandler(A2UINodeEventType eventType, const std::function<void()>& handler);

private:
    static void NodeEventReceiver(A2UINodeEvent* event);
    void HandleNodeEvent(A2UINodeEvent* event);
    static JsonValue BuildClickContext(A2UINodeEvent* event);
    bool HasClickHandler() const;
    void UpdateClickEventRegistration();

    ClickHandler onClick_;
    std::map<std::string, ClickHandler> auxiliaryOnClick_;
    std::map<A2UINodeEventType, std::function<void()>> nodeEventHandlers_;
    bool clickEventRegistered_ = false;
};

} // namespace NativeModule

#endif // A2UI_ARKUI_COMPONENT_H
