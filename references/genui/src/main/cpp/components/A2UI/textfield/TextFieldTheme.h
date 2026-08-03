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

#ifndef A2UI_TEXTFIELD_THEME_H
#define A2UI_TEXTFIELD_THEME_H

#include <array>
#include <string>

#include "theme/ThemeBase.h"

#include "ArkUINodeApiAdapter.h"

namespace NativeModule {

/**
 * @brief TextField component theme configuration
 */
class TextFieldTheme : public ThemeBase {
public:
    explicit TextFieldTheme(const ThemeContext& context);
    ~TextFieldTheme() override = default;

    void OnConfigChange(const ThemeContext& context) override;

    static float GetLabelFontSize();
    static std::array<float, 4> GetLabelPadding();
    static float GetErrorFontSize();
    static std::array<float, 4> GetErrorPadding();
    static A2UIFontWeight GetLabelFontWeight();
    static float GetLabelHeight();
    uint32_t GetLabelFontColor();
    uint32_t GetErrorFontColor();

private:
    /**
     * @brief Initialize all theme properties based on context
     */
    void InitializeAllProperties();
};

} // namespace NativeModule

#endif // A2UI_TEXTFIELD_THEME_H
