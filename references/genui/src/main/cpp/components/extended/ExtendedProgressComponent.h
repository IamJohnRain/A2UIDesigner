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

#ifndef A2UI_EXTENDED_PROGRESS_COMPONENT_H
#define A2UI_EXTENDED_PROGRESS_COMPONENT_H

#include <cstdint>
#include <string>

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedProgressComponent : public ExtendedComponent {
public:
    ExtendedProgressComponent();
    ~ExtendedProgressComponent() override = default;

    std::string GetType() const override;
    void OnConfigChange(const ThemeContext& context) override;

#ifdef TDD_BUILD
    float GetValueForTest() const
    {
        return value_;
    }

    float GetTotalForTest() const
    {
        return total_;
    }

    uint32_t GetColorForTest() const
    {
        return color_;
    }

    int32_t GetProgressTypeForTest() const
    {
        return progressType_;
    }

    float GetStrokeWidthForTest() const
    {
        return strokeWidth_;
    }

    void ApplyStrokeWidthValueForTest(const JsonValue& value)
    {
        ApplyStrokeWidthValue(value);
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;

private:
    void ReportStyleWarning(const std::string& code, const std::string& styleName, const std::string& message) const;
    void ApplyTotalPrivateAttribute(const JsonValue& descriptor);
    void SetValue(float value);
    void SetTotal(float total);
    void SetColor(uint32_t color);
    void SetProgressType(int32_t progressType);
    uint32_t ResolveDefaultColorByType(int32_t progressType) const;
    uint32_t ResolveLinearDefaultColor() const;
    void ApplyColorValue(const JsonValue& value);
    void ApplyProgressTypeValue(const JsonValue& value);
    void ApplyStrokeWidthValue(const JsonValue& value);
    void SetStrokeWidth(float strokeWidth);

    float value_ = 0.0F;
    float total_ = 100.0F;
    float strokeWidth_ = 4.0F;
    uint32_t color_ = 0xFF0A59F7u;
    int32_t progressType_ = 0;
    bool useDefaultColor_ = true;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_PROGRESS_COMPONENT_H
