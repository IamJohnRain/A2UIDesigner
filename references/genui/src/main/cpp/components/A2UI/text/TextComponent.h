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

#ifndef A2UI_TEXT_COMPONENT_H
#define A2UI_TEXT_COMPONENT_H

#include <string>

#include "../A2UIComponent.h"
#include "TextTheme.h"

namespace NativeModule {

class TextComponent : public A2UIComponent {
public:
    // Lifecycle / type
    TextComponent();
    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<TextTheme> GetTheme();

    // Text content and style API
    void SetTextContent(const std::string& content);
    void SetFontColor(uint32_t color);
    void ResetFontColor();
    void SetFontSize(float fontSize);
    void SetFontWeight(A2UIFontWeight fontWeight);
    void SetVariant(const std::string& variant);

protected:
    // Descriptor handling
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const override;

    // Theme configuration change hook
    void OnConfigChange(const ThemeContext& context) override;

private:
    // Cached content
    std::string textContent_;

    // Theme cache
    std::weak_ptr<TextTheme> cachedTheme_;
};

} // namespace NativeModule

#endif // A2UI_TEXT_COMPONENT_H
