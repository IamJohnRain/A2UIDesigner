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

#ifndef A2UI_CARD_THEME_H
#define A2UI_CARD_THEME_H

#include "theme/ThemeBase.h"

namespace NativeModule {

/**
 * @brief Card component theme configuration
 */
class CardTheme : public ThemeBase {
public:
    struct ValueMetrics {
        float borderRadius = 0.0F;
        float padding = 0.0F;
        float borderWidth = 0.0F;
    };

    struct AppearanceMetrics {
        int32_t shadowStyle = 0;
        uint32_t borderColor = 0;
        uint32_t backgroundColor = 0;
    };

    struct StyleMetrics {
        float borderRadius = 0.0F;
        int32_t shadowStyle = 0;
        float padding = 0.0F;
        float borderWidth = 0.0F;
        uint32_t borderColor = 0;
        uint32_t backgroundColor = 0;
    };

    explicit CardTheme(const ThemeContext& context);
    ~CardTheme() override = default;

    void OnConfigChange(const ThemeContext& context) override;

    const ValueMetrics& GetValueMetrics() const
    {
        return valueMetrics_;
    }
    const AppearanceMetrics& GetAppearanceMetrics() const
    {
        return appearanceMetrics_;
    }
    const StyleMetrics& GetStyleMetrics() const
    {
        return styleMetrics_;
    }

private:
    void InitializeAllProperties();
    void InitializeValueMetrics();
    void InitializeAppearanceMetrics();
    void UpdateStyleMetrics();
    ValueMetrics ResolveValueMetrics(Breakpoint breakpoint) const;
    uint32_t ResolveBackgroundColor() const;
    uint32_t ResolveBorderColor() const;
    int32_t ResolveShadowStyle() const;

    ValueMetrics valueMetrics_;
    AppearanceMetrics appearanceMetrics_;
    StyleMetrics styleMetrics_;
};

} // namespace NativeModule

#endif // A2UI_CARD_THEME_H
