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

#ifndef A2UI_BUTTON_THEME_H
#define A2UI_BUTTON_THEME_H

#include <array>
#include <string>

#include "theme/ThemeBase.h"

#include "ArkUINodeApiAdapter.h"

namespace NativeModule {

/**
 * @brief Button theme configuration
 *
 * This class defines the visual style constants for Button component.
 * It supports both light/dark modes and different breakpoints.
 */
class ButtonTheme : public ThemeBase {
public:
    explicit ButtonTheme(const ThemeContext& context);
    ~ButtonTheme() override = default;

    /**
     * @brief Called when theme configuration changes
     * @param context The new theme context
     */
    void OnConfigChange(const ThemeContext& context) override;

    /**
     * @brief Get the font color
     * @param variant button variant
     * @return The font color value
     */
    uint32_t GetFontColor(const std::string& variant) const;

    /**
     * @brief Get the background color
     * @param variant button variant
     * @return The background color value
     */
    uint32_t GetBackgroundColor(const std::string& variant) const;

    /**
     * @brief Whether the current context has a brand color
     * @return True if the current context has a brand color
     */
    bool HasBrandColor() const
    {
        return currentContext_.hasBrandColor;
    }

    /**
     * @brief Get the font weight
     * @return The font weight value
     */
    static A2UIFontWeight GetFontWeight();

    /**
     * @brief Get the height
     * @return The height value
     */
    static int32_t GetHeight();

    /**
     * @brief Get the padding
     * @return The padding value
     */
    static std::array<float, 4> GetPadding();

    /**
     * @brief Get the icon padding
     * @return The icon padding value
     */
    static std::array<float, 4> GetIconPadding();

    /**
     * @brief Get icon-mode button square size
     * @return The size in vp
     */
    static int32_t GetIconButtonSize();

    /**
     * @brief Get icon square size
     * @return The size in vp
     */
    static int32_t GetIconSize();

    /**
     * @brief Get icon color for variant
     * @param variant button variant
     * @return The icon color value
     */
    uint32_t GetIconColor(const std::string& variant);

    // TODO: Add getter methods for colors and values
    // These will return values based on current context (mode, breakpoint, brand color)

protected:
    /**
     * @brief Initialize background color based on theme context
     */
    void InitializeBackgroundColor();

private:
    /**
     * @brief Initialize all theme properties based on context
     */
    void InitializeAllProperties();
};

} // namespace NativeModule

#endif // A2UI_BUTTON_THEME_H
