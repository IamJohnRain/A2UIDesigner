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

#include "CardTheme.h"

#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_CARD_BORDER_RADIUS = 8.0F;
constexpr int32_t DEFAULT_CARD_SHADOW_STYLE = static_cast<int32_t>(A2UIShadowStyle::OUTER_DEFAULT_LG);
constexpr float DEFAULT_CARD_PADDING = 16.0F;
constexpr float DEFAULT_CARD_BORDER_WIDTH = 1.0F;
constexpr uint32_t DEFAULT_CARD_BORDER_COLOR_LIGHT = 0xFFE0E0E0;
constexpr uint32_t DEFAULT_CARD_BORDER_COLOR_DARK = 0xFF333333;
constexpr uint32_t DEFAULT_CARD_BACKGROUND_COLOR_LIGHT = 0xFFFFFFFF;
constexpr uint32_t DEFAULT_CARD_BACKGROUND_COLOR_DARK = 0xFF1A1A1A;

} // namespace

CardTheme::CardTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void CardTheme::OnConfigChange(const ThemeContext& context)
{
    // Note: context is already updated by ThemeBase::UpdateContext
    // Initialize all properties with new context
    InitializeAllProperties();
}

void CardTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_INFO,
        "CardTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    InitializeValueMetrics();
    InitializeAppearanceMetrics();
    UpdateStyleMetrics();
}

void CardTheme::InitializeValueMetrics()
{
    valueMetrics_ = ResolveValueMetrics(currentContext_.breakpoint);
}

void CardTheme::InitializeAppearanceMetrics()
{
    appearanceMetrics_.shadowStyle = ResolveShadowStyle();
    appearanceMetrics_.borderColor = ResolveBorderColor();
    appearanceMetrics_.backgroundColor = ResolveBackgroundColor();
}

void CardTheme::UpdateStyleMetrics()
{
    styleMetrics_ = { .borderRadius = valueMetrics_.borderRadius,
        .shadowStyle = appearanceMetrics_.shadowStyle,
        .padding = valueMetrics_.padding,
        .borderWidth = valueMetrics_.borderWidth,
        .borderColor = appearanceMetrics_.borderColor,
        .backgroundColor = appearanceMetrics_.backgroundColor };
}

CardTheme::ValueMetrics CardTheme::ResolveValueMetrics(Breakpoint breakpoint) const
{
    return { .borderRadius = DEFAULT_CARD_BORDER_RADIUS,
        .padding = DEFAULT_CARD_PADDING,
        .borderWidth = DEFAULT_CARD_BORDER_WIDTH };
}

uint32_t CardTheme::ResolveBackgroundColor() const
{
    if (currentContext_.colorMode == ThemeMode::LIGHT) {
        return DEFAULT_CARD_BACKGROUND_COLOR_LIGHT;
    }
    return DEFAULT_CARD_BACKGROUND_COLOR_DARK;
}

uint32_t CardTheme::ResolveBorderColor() const
{
    if (currentContext_.colorMode == ThemeMode::LIGHT) {
        return DEFAULT_CARD_BORDER_COLOR_LIGHT;
    }
    return DEFAULT_CARD_BORDER_COLOR_DARK;
}

int32_t CardTheme::ResolveShadowStyle() const
{
    return DEFAULT_CARD_SHADOW_STYLE;
}

} // namespace NativeModule
