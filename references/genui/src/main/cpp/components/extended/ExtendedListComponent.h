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

#ifndef A2UI_EXTENDED_LIST_COMPONENT_H
#define A2UI_EXTENDED_LIST_COMPONENT_H

#include <memory>
#include <optional>
#include <vector>

#include "components/extended/ExtendedComponent.h"
#include "composition/ListAdapterNode.h"

namespace NativeModule {

class ExtendedListComponent : public ExtendedComponent {
public:
    ExtendedListComponent();
    ~ExtendedListComponent() override;

    std::string GetType() const override;
    void OnConfigChange(const ThemeContext& context) override;
    void RemoveAllChildren() override;
    void SetLazyMode(bool useLazy);
    void SetAdapterNode(const std::shared_ptr<ListAdapterNode>& adapterNode);
    bool IsLazyMode() const
    {
        return mode_ == Mode::LAZY;
    }
    std::shared_ptr<ListAdapterNode> GetAdapterNode() const
    {
        return adapterNode_;
    }
    std::shared_ptr<TemplateAdapterNode> GetLazyAdapter() const override
    {
        return mode_ == Mode::LAZY ? adapterNode_ : nullptr;
    }

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void ValidateComponentDescriptorSchema(const JsonValue& descriptor) override;
    void ValidateComponentSpecificStylesSchema(const JsonValue& styles) override;
    void ValidateComponentSpecificDynamicStylesDfx(
        const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;
    void RegisterComponentSpecificListeners() override;
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds) override;

private:
    enum class Mode { EAGER, LAZY };

    struct ListItemSlot {
        std::weak_ptr<Component> child;
        ArkUI_NodeHandle listItemNode = nullptr;
    };

    void SetupLazyAdapter(const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot);
    void HandleSizeChange(A2UINodeEvent* event, bool isAreaChange = false);
    void ApplyNestedScrollValueOrDefault(const JsonValue& value);
    void ApplyDefaultLanes(const ThemeContext& context);
    ThemeContext ResolveThemeContext() const;
    void SetSpace(float space);
    void SetLanes(int32_t lanes);
    void SetListDirection(A2UIAxis direction);
    void SetScrollBar(A2UIScrollBarDisplayMode displayMode);
    void SetNestedScroll(A2UIScrollNestedMode scrollForward, A2UIScrollNestedMode scrollBackward);
    void RemoveListItemNode(ArkUI_NodeHandle listItemNode);

    Mode mode_ = Mode::EAGER;
    std::shared_ptr<ListAdapterNode> adapterNode_;
    std::vector<ListItemSlot> listItems_;
    std::optional<Breakpoint> componentBreakpoint_;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_LIST_COMPONENT_H
