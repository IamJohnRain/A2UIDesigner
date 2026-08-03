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

#ifndef A2UI_EXTENDED_IMAGE_COMPONENT_H
#define A2UI_EXTENDED_IMAGE_COMPONENT_H

#include <cstdint>
#include <string>

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedImageComponent : public ExtendedComponent {
public:
    ExtendedImageComponent();
    ~ExtendedImageComponent() override = default;

    std::string GetType() const override;

#ifdef TDD_BUILD
    const std::string& GetSrcValueForTest() const
    {
        return srcValue_;
    }

    const std::string& GetAltValueForTest() const
    {
        return altValue_;
    }

    float GetAspectRatioForTest() const
    {
        return aspectRatio_;
    }

    int32_t GetObjectFitForTest() const
    {
        return static_cast<int32_t>(objectFit_);
    }

    bool HasFillColorForTest() const
    {
        return hasFillColor_;
    }

    uint32_t GetFillColorForTest() const
    {
        return fillColor_;
    }

    PropertyDeclaration GetPrivatePropertyDeclarationForTest(const std::string& propertyName)
    {
        return GetPrivatePropertyDeclaration(propertyName);
    }

    void ApplyPrivateAttributesForTest(const JsonValue& descriptor)
    {
        ApplyPrivateAttributes(descriptor);
    }

    void ApplyComponentSpecificStylesForTest(const JsonValue& styles, ArkUINodeApiAdapter& applier)
    {
        ApplyComponentSpecificStyles(styles, applier);
    }

    void SetAltForTest(const std::string& alt)
    {
        SetAlt(alt);
    }

    void* GetNativeNodeApiForTest() const
    {
        return ArkUINodeApiAdapter::GetNativeNodeAPI();
    }

    void OverrideNativeStateForTest(void* nativeNodeApi, ArkUI_NodeHandle nativeView)
    {
        static_cast<void>(nativeNodeApi);
        nativeView_ = nativeView;
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;

private:
    void ReportStyleWarning(const std::string& code, const std::string& styleName, const std::string& message) const;
    void SetSrc(const std::string& src);
    void SetAlt(const std::string& alt);
    void SetAspectRatio(float ratio);
    void SetObjectFit(A2UIObjectFit objectFit);
    void SetFillColor(uint32_t color);
    void ResetFillColor();

    std::string srcValue_;
    std::string altValue_;
    float aspectRatio_ = 1.0F;
    A2UIObjectFit objectFit_ = A2UIObjectFit::COVER;
    bool hasFillColor_ = false;
    uint32_t fillColor_ = 0;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_IMAGE_COMPONENT_H
