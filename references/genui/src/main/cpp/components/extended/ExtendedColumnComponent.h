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

#ifndef A2UI_EXTENDED_COLUMN_COMPONENT_H
#define A2UI_EXTENDED_COLUMN_COMPONENT_H

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedColumnComponent : public ExtendedComponent {
public:
    ExtendedColumnComponent();
    ~ExtendedColumnComponent() override = default;

    std::string GetType() const override;
    void RemoveAllChildren() override;

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ValidateComponentDescriptorSchema(const JsonValue& descriptor) override;
    void ValidateComponentSpecificStylesSchema(const JsonValue& styles) override;
    void ValidateComponentSpecificDynamicStylesDfx(
        const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;

private:
    static constexpr float DEFAULT_ITEM_MARGIN = 8.0F;

    void SetAlignItems(A2UIHorizontalAlignment alignment);
    void SetJustifyContent(A2UIFlexAlignment alignment);
    void SetItemMargin(float itemMargin);
    void ApplyItemMarginToChildren(const std::shared_ptr<Component>& excludedChild = nullptr);
    void RestoreCommonMarginForChild(const std::shared_ptr<Component>& child);
    void RestoreCommonMarginForChildren();

    float itemMargin_ = DEFAULT_ITEM_MARGIN;
    bool itemMarginDisabledByJustify_ = false;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_COLUMN_COMPONENT_H
