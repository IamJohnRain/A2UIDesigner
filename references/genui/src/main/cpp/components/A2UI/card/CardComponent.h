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

#ifndef A2UI_CARD_COMPONENT_H
#define A2UI_CARD_COMPONENT_H

#include "../A2UIComponent.h"
#include "CardTheme.h"

namespace NativeModule {

class CardComponent : public A2UIComponent {
public:
    CardComponent();

    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<CardTheme> GetTheme();

    void SetShadow(float radius, uint32_t color, float offsetX, float offsetY);
    void SetShadow(int32_t shadowStyle);
    void SetBorderWidth(float top, float right, float bottom, float left);
    void SetBorderColor(uint32_t color);

protected:
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void OnConfigChange(const ThemeContext& context) override;

private:
    void ApplyThemeDefaults(const CardTheme::StyleMetrics& styleMetrics);

    // Theme cache
    std::weak_ptr<CardTheme> cachedTheme_;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
};

} // namespace NativeModule

#endif // A2UI_CARD_COMPONENT_H
