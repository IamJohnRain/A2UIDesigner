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

#ifndef A2UI_EXTENDED_ROW_COMPONENT_H
#define A2UI_EXTENDED_ROW_COMPONENT_H

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedRowComponent : public ExtendedComponent {
public:
    ExtendedRowComponent();
    ~ExtendedRowComponent() override = default;

    std::string GetType() const override;

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ValidateComponentDescriptorSchema(const JsonValue& descriptor) override;
    void ValidateComponentSpecificStylesSchema(const JsonValue& styles) override;
    void ValidateComponentSpecificDynamicStylesDfx(
        const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;

private:
    static constexpr float DEFAULT_ITEM_MARGIN = 16.0F;
    static constexpr A2UIItemAlignment DEFAULT_ALIGN_ITEMS = A2UIItemAlignment::CENTER;
    static constexpr A2UIFlexAlignment DEFAULT_JUSTIFY_CONTENT = A2UIFlexAlignment::START;
    static constexpr A2UIFlexWrap DEFAULT_WRAP = A2UIFlexWrap::NO_WRAP;

    void SetAlignItems(A2UIItemAlignment alignment);
    void SetJustifyContent(A2UIFlexAlignment alignment);
    void SetWrap(A2UIFlexWrap wrap);
    void SetItemMargin(float itemMargin);
    void ApplyItemMarginSpace();
    void SetSpace(float space);
    void ApplyFlexOptions();

    float itemMargin_ = DEFAULT_ITEM_MARGIN;
    bool itemMarginDisabledByJustify_ = false;
    A2UIItemAlignment alignItems_ = DEFAULT_ALIGN_ITEMS;
    A2UIFlexAlignment justifyContent_ = DEFAULT_JUSTIFY_CONTENT;
    A2UIFlexWrap wrap_ = DEFAULT_WRAP;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_ROW_COMPONENT_H
