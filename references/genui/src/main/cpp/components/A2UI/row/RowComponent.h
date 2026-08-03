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

#ifndef A2UI_ROW_COMPONENT_H
#define A2UI_ROW_COMPONENT_H

#include "../A2UIComponent.h"
#include "RowTheme.h"

namespace NativeModule {

class RowComponent : public A2UIComponent {
public:
    // Lifecycle / type
    RowComponent();
    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<RowTheme> GetTheme();

    // Row layout API
    void SetAlignItems(A2UIVerticalAlignment alignment);
    void SetJustifyContent(A2UIFlexAlignment alignment);

protected:
    // Descriptor handling
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void OnConfigChange(const ThemeContext& context) override;

private:
    // Theme cache
    std::weak_ptr<RowTheme> cachedTheme_;
    bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds) override;
    void ApplyMarginToChild(const std::shared_ptr<Component>& child, float startMargin, float endMargin) override;
};

} // namespace NativeModule

#endif // A2UI_ROW_COMPONENT_H
