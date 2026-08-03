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

#include "CardComponent.h"

#include "utils/LogA2UI.h"

#include "CardTheme.h"

namespace NativeModule {

CardComponent::CardComponent() : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN)) {}

void CardComponent::SetShadow(float radius, uint32_t color, float offsetX, float offsetY)
{
    ArkUINodeApiAdapter::SetNodeShadow(nativeView_, radius, color, offsetX, offsetY);
}

void CardComponent::SetShadow(int32_t shadowStyle)
{
    ArkUINodeApiAdapter::SetNodeShadow(nativeView_, shadowStyle);
}

void CardComponent::SetBorderWidth(float top, float right, float bottom, float left)
{
    ArkUINodeApiAdapter::SetNodeBorderWidth(nativeView_, top, right, bottom, left);
}

void CardComponent::SetBorderColor(uint32_t color)
{
    ArkUINodeApiAdapter::SetNodeBorderColor(nativeView_, color);
}

void CardComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChild(descriptor.GetItem("child"));
}

void CardComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    auto cardTheme = GetTheme();
    if (cardTheme == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "CardComponent::ApplyPrivateAttributes: CardTheme is nullptr, renderId=%{public}d, "
            "surfaceId=%{public}s, componentId=%{public}s",
            renderId_, surfaceId_.c_str(), componentId_.c_str());
        return;
    }
    ApplyThemeDefaults(cardTheme->GetStyleMetrics());

    if (descriptor.Has("width")) {
        SetWidth(static_cast<float>(descriptor.GetNumber("width", 10.0)));
    }
    if (descriptor.Has("height")) {
        SetHeight(static_cast<float>(descriptor.GetNumber("height", 10.0)));
    }
}

bool CardComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    return A2UIComponent::IsKnownAdditionalDescriptorKey(propertyName) || propertyName == "width" ||
           propertyName == "height";
}

std::string CardComponent::GetType() const
{
    return "Card";
}

std::shared_ptr<CardTheme> CardComponent::GetTheme()
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
    theme = std::dynamic_pointer_cast<CardTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void CardComponent::ApplyThemeDefaults(const CardTheme::StyleMetrics& styleMetrics)
{
    SetBorderRadius(styleMetrics.borderRadius);
    SetBackgroundColor(styleMetrics.backgroundColor);
    SetShadow(styleMetrics.shadowStyle);
    SetPadding(styleMetrics.padding, styleMetrics.padding, styleMetrics.padding, styleMetrics.padding);
    SetBorderWidth(
        styleMetrics.borderWidth, styleMetrics.borderWidth, styleMetrics.borderWidth, styleMetrics.borderWidth);
    SetBorderColor(styleMetrics.borderColor);
}

void CardComponent::OnConfigChange(const ThemeContext& context)
{
    auto cardTheme = GetTheme();
    if (cardTheme == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "CardComponent::OnConfigChange: CardTheme is nullptr, renderId=%{public}d, "
            "surfaceId=%{public}s, componentId=%{public}s",
            renderId_, surfaceId_.c_str(), componentId_.c_str());
        return;
    }

    ApplyThemeDefaults(cardTheme->GetStyleMetrics());
}

} // namespace NativeModule
