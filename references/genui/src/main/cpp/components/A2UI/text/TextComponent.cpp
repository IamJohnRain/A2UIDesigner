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

#include "TextComponent.h"

#include "utils/LogA2UI.h"

#include "TextTheme.h"

namespace NativeModule {
TextComponent::TextComponent() : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT)) {}

PropertyDeclaration TextComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(TextComponent&)>> declarations = {
        { "text",
            [](TextComponent& textComponent) {
                return PropertyDeclaration { .name = "text",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .fallbackString = "",
                    .applyValue = [&textComponent](const JsonValue& value) {
                        textComponent.SetTextContent(value.GetStringValue(""));
                    } };
            } },
        { "variant",
            [](TextComponent& textComponent) {
                return PropertyDeclaration { .name = "variant",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "body",
                    .enumAllowed = { "h1", "h2", "h3", "h4", "h5", "caption", "body" },
                    .enumFallback = "body",
                    .applyValue = [&textComponent](const JsonValue& value) {
                        textComponent.SetVariant(value.GetStringValue("body"));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

std::string TextComponent::GetType() const
{
    return "Text";
}

std::vector<std::string> TextComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return { "text" };
}

void TextComponent::SetTextContent(const std::string& content)
{
    textContent_ = content;
    ArkUINodeApiAdapter::SetNodeTextContent(nativeView_, textContent_);
}

void TextComponent::SetFontColor(uint32_t color)
{
    ArkUINodeApiAdapter::SetNodeFontColor(nativeView_, color);
}

void TextComponent::ResetFontColor()
{
    ArkUINodeApiAdapter::ResetNodeFontColor(nativeView_);
}

void TextComponent::SetFontSize(float fontSize)
{
    ArkUINodeApiAdapter::SetNodeFontSize(nativeView_, fontSize);
}

void TextComponent::SetFontWeight(A2UIFontWeight fontWeight)
{
    ArkUINodeApiAdapter::SetNodeFontWeight(nativeView_, fontWeight);
}

void TextComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("child"));
}

void TextComponent::SetVariant(const std::string& variant)
{
    SetFontSize(TextTheme::ResolveFontSize(variant));
}

void TextComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ApplySchemaProperty("text", descriptor);
    ApplySchemaProperty("variant", descriptor);
}

std::shared_ptr<TextTheme> TextComponent::GetTheme()
{
    // Try to get from cache first
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }

    // Cache miss, get from base class
    std::shared_ptr<ThemeBase> baseTheme = A2UIComponent::GetTheme();
    if (baseTheme == nullptr) {
        return nullptr;
    }

    // Cast to specific type and cache it
    theme = std::dynamic_pointer_cast<TextTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void TextComponent::OnConfigChange(const ThemeContext& context)
{
    // Get TextTheme for this component
    auto textTheme = GetTheme();
    if (textTheme == nullptr) {
        LOG_A2UI(LOG_WARN, "TextComponent::OnConfigChange: TextTheme is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    // TODO: Re-apply styles based on new theme context
    // This will use the updated theme values (colors, font sizes, etc.)
}

} // namespace NativeModule
