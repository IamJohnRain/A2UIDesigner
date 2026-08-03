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

#ifndef A2UI_TEXT_THEME_H
#define A2UI_TEXT_THEME_H

#include <string>

#include "theme/ThemeBase.h"

namespace NativeModule {

/**
 * @brief Text component theme configuration
 *
 * This class defines the visual style constants for Text component.
 * It supports both light/dark modes and different breakpoints.
 */
class TextTheme : public ThemeBase {
public:
    explicit TextTheme(const ThemeContext& context);
    ~TextTheme() override = default;

    /**
     * @brief Called when theme configuration changes
     * @param context The new theme context
     */
    void OnConfigChange(const ThemeContext& context) override;

    /**
     * @brief Resolve font size by text variant token
     * @param variant text variant, such as h1/h2/body/caption
     * @return Corresponding font size value
     */
    static float ResolveFontSize(const std::string& variant);

private:
    /**
     * @brief Initialize all theme properties based on context
     */
    void InitializeAllProperties();
};

} // namespace NativeModule

#endif // A2UI_TEXT_THEME_H
