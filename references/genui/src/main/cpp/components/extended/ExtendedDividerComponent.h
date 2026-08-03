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

#ifndef A2UI_EXTENDED_DIVIDER_COMPONENT_H
#define A2UI_EXTENDED_DIVIDER_COMPONENT_H

#include <cstdint>
#include <string>

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedDividerComponent : public ExtendedComponent {
public:
    ExtendedDividerComponent();
    ~ExtendedDividerComponent() override = default;

    std::string GetType() const override;
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void OnConfigChange(const ThemeContext& context) override;

#ifdef TDD_BUILD
    float GetStrokeWidthValueForTest() const
    {
        return strokeWidth_.value;
    }

    std::string GetStrokeWidthUnitForTest() const;

    bool GetVerticalForTest() const
    {
        return vertical_;
    }

    uint32_t GetColorForTest() const
    {
        return color_;
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter&) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;

private:
    enum class StrokeWidthUnit { VP = 0, FP, PX, PERCENT };

    struct StrokeWidthDimension {
        float value = 1.0F;
        StrokeWidthUnit unit = StrokeWidthUnit::PX;
    };

    void ReportStyleWarning(const std::string& code, const std::string& styleName, const std::string& message) const;
    void ApplyStrokeWidthPrivateValue(const JsonValue& value, bool reportWarning);
    void ApplyVerticalPrivateValue(const JsonValue& value, bool reportWarning);
    void SetStrokeWidth(const JsonValue& value);
    void SetVertical(bool vertical);
    void SetColor(const std::string& colorValue);
    uint32_t ResolveDefaultColor() const;
    void ApplyPrivateGeometryStyleState(const JsonValue& styles);
    void ApplyGeometryAfterCommonDimensions();
    void ApplyColor();
    void UpdateCommonDimensionState(const JsonValue& styles);
    void ApplyThickness(bool isWidth);
    void SetAbsoluteDimension(bool isWidth, float value);
    void SetPercentDimension(bool isWidth, float percent);
    void ResetDimension(A2UINodeAttributeType attribute);

    StrokeWidthDimension strokeWidth_;
    bool hasCommonWidth_ = false;
    bool hasCommonHeight_ = false;
    bool vertical_ = false;
    uint32_t color_ = 0x33000000u;
    bool useDefaultColor_ = true;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_DIVIDER_COMPONENT_H
