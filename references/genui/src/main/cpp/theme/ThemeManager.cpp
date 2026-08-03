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

#include "theme/ThemeManager.h"

#include "theme/ThemeFactory.h"
#include "utils/ThemeColorUtils.h"

#include "SurfaceSlot.h"

namespace NativeModule {

ThemeManager::ThemeManager(const std::string& surfaceId, int32_t renderId, const ThemeContext& context)
    : surfaceId_(surfaceId), renderId_(renderId), context_(context)
{}

void ThemeManager::UpdateThemeMode(ThemeMode mode)
{
    context_.colorMode = mode;
    RefreshBrandColorFromContext(false);
}

void ThemeManager::UpdateBreakpoint(Breakpoint breakpoint)
{
    context_.breakpoint = breakpoint;
}

void ThemeManager::UpdateBrandColor(uint32_t color)
{
    context_.brandColor = color;
    context_.hasBrandColor = true;
}

void ThemeManager::ClearBrandColor()
{
    context_.hasBrandColor = false;
    context_.brandColor = 0;
}

void ThemeManager::SetComponentTheme(const ThemeContext& themeContext)
{
    context_.hasPrimaryColor = themeContext.hasPrimaryColor;
    context_.primaryColorArgb = themeContext.primaryColorArgb;
    context_.hasDarkPrimaryColor = themeContext.hasDarkPrimaryColor;
    context_.darkPrimaryColorArgb = themeContext.darkPrimaryColorArgb;
    context_.iconUrl = themeContext.iconUrl;
    context_.agentDisplayName = themeContext.agentDisplayName;

    RefreshBrandColorFromContext(true);
}

void ThemeManager::RefreshBrandColorFromContext(bool forceClearWhenUnresolved)
{
    if (context_.colorMode == ThemeMode::DARK) {
        if (context_.hasDarkPrimaryColor) {
            UpdateBrandColor(context_.darkPrimaryColorArgb);
            return;
        }
        if (context_.hasPrimaryColor) {
            UpdateBrandColor(ThemeColorUtils::InvertRgbKeepAlpha(context_.primaryColorArgb));
            return;
        }
    } else if (context_.hasPrimaryColor) {
        UpdateBrandColor(context_.primaryColorArgb);
        return;
    }

    if (forceClearWhenUnresolved || context_.HasComponentThemeValue()) {
        ClearBrandColor();
    }
}

std::shared_ptr<ThemeBase> ThemeManager::GetTheme(const std::string& componentType)
{
    // Try to find in cache first
    auto it = themes_.find(componentType);
    if (it != themes_.end()) {
        return it->second;
    }

    // Not found in cache, create and cache it
    auto theme = CreateTheme(componentType, context_);
    if (theme != nullptr) {
        themes_[componentType] = theme;
    }

    return theme;
}

std::shared_ptr<ThemeBase> ThemeManager::CreateTheme(const std::string& componentType, const ThemeContext& context)
{
    // Use ThemeFactory to create theme with context
    return ThemeFactory::CreateTheme(componentType, context);
}

void ThemeManager::NotifyThemeChange(SurfaceSlot* surfaceSlot)
{
    if (surfaceSlot == nullptr) {
        return;
    }

    // Also update all cached themes
    for (auto& themeEntry : themes_) {
        std::shared_ptr<ThemeBase>& theme = themeEntry.second;
        if (theme != nullptr) {
            theme->UpdateContext(context_);
        }
    }

    // Notify all components in this surface
    for (const auto& componentEntry : surfaceSlot->GetAllComponents()) {
        const std::shared_ptr<Component>& component = componentEntry.second;
        if (component != nullptr) {
            component->OnConfigChange(context_);
        }
    }
}

} // namespace NativeModule
