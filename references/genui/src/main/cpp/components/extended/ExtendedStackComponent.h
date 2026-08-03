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

#ifndef A2UI_EXTENDED_STACK_COMPONENT_H
#define A2UI_EXTENDED_STACK_COMPONENT_H

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedStackComponent : public ExtendedComponent {
public:
    ExtendedStackComponent();
    ~ExtendedStackComponent() override = default;

    std::string GetType() const override;

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void ValidateComponentDescriptorSchema(const JsonValue& descriptor) override;
    void ValidateComponentSpecificStylesSchema(const JsonValue& styles) override;
    void ValidateComponentSpecificDynamicStylesDfx(
        const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds) override;

private:
    void SetAlignContent(A2UIAlignment alignment);
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_STACK_COMPONENT_H
