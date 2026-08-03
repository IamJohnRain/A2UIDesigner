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

#ifndef A2UI_EXTENDED_NAV_CONTAINER_COMPONENT_H
#define A2UI_EXTENDED_NAV_CONTAINER_COMPONENT_H

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class NavContainerComponent : public ExtendedComponent {
public:
    NavContainerComponent();
    ~NavContainerComponent() override = default;

    std::string GetType() const override;

    bool NavigateToTargetComponent(const std::string& targetComponentId);

#ifdef TDD_BUILD
    int32_t GetCurrentIndexForTest() const
    {
        return currentIndex_;
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ValidateComponentDescriptorSchema(const JsonValue& descriptor) override;
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;
    std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const override;

private:
    void SetCurrentIndex(int32_t currentIndex);
    void RefreshChildVisibility();
    int32_t ResolveVisibleIndex(size_t childCount) const;

    int32_t currentIndex_ = 0;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_NAV_CONTAINER_COMPONENT_H
