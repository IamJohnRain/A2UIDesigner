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

#ifndef A2UI_IMAGE_COMPONENT_H
#define A2UI_IMAGE_COMPONENT_H

#include "../A2UIComponent.h"
#include "ImageTheme.h"

namespace NativeModule {

class ImageComponent : public A2UIComponent {
public:
    ImageComponent();

    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<ImageTheme> GetTheme();

    void SetSrc(const std::string& src);
    void SetObjectFit(A2UIObjectFit objectFit);
    void SetAlt(const std::string& alt);

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void OnConfigChange(const ThemeContext& context) override;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
    std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const override;

private:
    PropertyDeclaration CreateUrlPropertyDeclaration();
    PropertyDeclaration CreateDescriptionPropertyDeclaration();
    PropertyDeclaration CreateFitPropertyDeclaration();
    PropertyDeclaration CreateVariantPropertyDeclaration();
    A2UIObjectFit ParseObjectFit(const std::string& fit) const;
    void ApplyVariantPreset(const std::string& variant);

    // Theme cache
    std::weak_ptr<ImageTheme> cachedTheme_;
};

} // namespace NativeModule

#endif // A2UI_IMAGE_COMPONENT_H
