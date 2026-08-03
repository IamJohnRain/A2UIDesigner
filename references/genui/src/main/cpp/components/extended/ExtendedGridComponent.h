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

#ifndef A2UI_EXTENDED_GRID_COMPONENT_H
#define A2UI_EXTENDED_GRID_COMPONENT_H

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "components/extended/ExtendedComponent.h"
#include "composition/GridAdapterNode.h"

namespace NativeModule {

class ExtendedGridComponent : public ExtendedComponent {
public:
    ExtendedGridComponent();
    ~ExtendedGridComponent() override;

    std::string GetType() const override;
    void OnConfigChange(const ThemeContext& context) override;
    void RemoveAllChildren() override;
    void SetLazyMode(bool useLazy);
    void SetAdapterNode(const std::shared_ptr<GridAdapterNode>& adapterNode);
    bool IsLazyMode() const
    {
        return mode_ == Mode::LAZY;
    }
    std::shared_ptr<GridAdapterNode> GetAdapterNode() const
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
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds) override;

private:
    enum class Mode { EAGER, LAZY };
    enum class TemplateMode {
        THEME_DEFAULT, // Use the component's default template for the current theme context.
        FIXED,         // Use one explicit template string without breakpoint switching.
        RESPONSIVE,    // Resolve the template from xs/sm/md/lg/xl breakpoint values.
        RESET          // Clear explicit state and fall back to the component-specific default behavior.
    };

    struct GridTemplateConfig {
        TemplateMode mode = TemplateMode::RESET;
        std::string fixedValue;
        std::array<std::string, 5> responsiveValues {};
    };

    struct GridItemSlot {
        std::weak_ptr<Component> child;
        ArkUI_NodeHandle itemNode = nullptr;
    };

    void ApplyColumnsTemplateForContext(const ThemeContext& context);
    void ApplyRowsTemplateForContext(const ThemeContext& context);
    ThemeContext ResolveThemeContext() const;
    static bool ParseTemplateConfig(const JsonValue& value, GridTemplateConfig& config);
    static std::string ResolveResponsiveTemplate(const GridTemplateConfig& config, const ThemeContext& context);
    bool SetupLazyAdapter(const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot);
    void SetColumnsTemplate(const std::string& columnsTemplate);
    void SetRowsTemplate(const std::string& rowsTemplate);
    void SetColumnsGap(float columnsGap);
    void SetRowsGap(float rowsGap);
    void RefreshGridItemHeightPolicies();
    bool ShouldGridItemsWrapContentHeight() const;
    void DetachGridItemNode(ArkUI_NodeHandle itemNode);

    Mode mode_ = Mode::EAGER;
    std::shared_ptr<GridAdapterNode> adapterNode_;
    GridTemplateConfig columnsTemplateConfig_;
    GridTemplateConfig rowsTemplateConfig_;
    std::vector<GridItemSlot> gridItems_;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_GRID_COMPONENT_H
