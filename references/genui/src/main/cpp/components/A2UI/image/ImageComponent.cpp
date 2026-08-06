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

#include "ImageComponent.h"

#include "ImageTheme.h"

namespace {

constexpr float VARIANT_ICON_WIDTH = 32.0F;
constexpr float VARIANT_ICON_HEIGHT = 32.0F;

constexpr float VARIANT_AVATAR_WIDTH = 32.0F;
constexpr float VARIANT_AVATAR_HEIGHT = 32.0F;
constexpr float VARIANT_AVATAR_BORDER_RADIUS = 16.0F;

constexpr float VARIANT_SMALL_FEATURE_WIDTH = 50.0F;
constexpr float VARIANT_SMALL_FEATURE_HEIGHT = 50.0F;

constexpr float VARIANT_MEDIUM_FEATURE_WIDTH = 150.0F;
constexpr float VARIANT_MEDIUM_FEATURE_HEIGHT = 150.0F;

constexpr float VARIANT_LARGE_FEATURE_WIDTH = 400.0F;
constexpr float VARIANT_LARGE_FEATURE_HEIGHT = 400.0F;

constexpr float FULL_WIDTH_PERCENT = 1.0F;
constexpr char DEFAULT_IMAGE_PLACEHOLDER_ALT[] = "resources/base/media/placeHolder_E5E5EA.png";
constexpr char DEFAULT_IMAGE_VARIANT[] = "mediumFeature";
constexpr char HEADER_IMAGE_VARIANT[] = "header";
constexpr char DEFAULT_IMAGE_FIT[] = "fill";
constexpr char HEADER_IMAGE_FIT[] = "contain";

thread_local std::string g_imageFitFallbackVariant = DEFAULT_IMAGE_VARIANT;

std::string ResolveDefaultFitValueForVariant(const std::string& variant)
{
    return variant == HEADER_IMAGE_VARIANT ? HEADER_IMAGE_FIT : DEFAULT_IMAGE_FIT;
}

bool IsSupportedImageVariant(const std::string& variant)
{
    return variant == "icon" || variant == "avatar" || variant == "smallFeature" || variant == DEFAULT_IMAGE_VARIANT ||
           variant == "largeFeature" || variant == HEADER_IMAGE_VARIANT;
}

std::string ResolveDescriptorVariantForFitFallback(const NativeModule::JsonValue& descriptor)
{
    if (!descriptor.IsObject() || !descriptor.Has("variant")) {
        return DEFAULT_IMAGE_VARIANT;
    }

    NativeModule::JsonValue variantValue = descriptor.GetObjectItem("variant");
    if (!variantValue.IsString()) {
        return DEFAULT_IMAGE_VARIANT;
    }

    std::string variant = variantValue.GetStringValue(DEFAULT_IMAGE_VARIANT);
    return IsSupportedImageVariant(variant) ? variant : DEFAULT_IMAGE_VARIANT;
}

class ScopedImageFitFallbackVariant {
public:
    explicit ScopedImageFitFallbackVariant(const std::string& variant) : previousVariant_(g_imageFitFallbackVariant)
    {
        g_imageFitFallbackVariant = variant;
    }

    ~ScopedImageFitFallbackVariant()
    {
        g_imageFitFallbackVariant = previousVariant_;
    }

private:
    std::string previousVariant_;
};

} // namespace

namespace NativeModule {

ImageComponent::ImageComponent() : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::IMAGE)) {}

PropertyDeclaration ImageComponent::CreateUrlPropertyDeclaration()
{
    return PropertyDeclaration { .name = "url",
        .type = PropertyValueType::STRING,
        .allowDynamic = true,
        .fallbackString = "",
        .applyValue = [this](const JsonValue& value) { SetSrc(value.GetStringValue("")); } };
}

PropertyDeclaration ImageComponent::CreateDescriptionPropertyDeclaration()
{
    return PropertyDeclaration { .name = "description",
        .type = PropertyValueType::STRING,
        .allowDynamic = true,
        .fallbackString = "",
        .applyValue = [this](const JsonValue& value) { SetAlt(value.GetStringValue("")); } };
}

PropertyDeclaration ImageComponent::CreateFitPropertyDeclaration()
{
    std::string defaultFit = ResolveDefaultFitValueForVariant(g_imageFitFallbackVariant);
    return PropertyDeclaration { .name = "fit",
        .type = PropertyValueType::ENUM_STRING,
        .allowDynamic = false,
        .fallbackString = defaultFit,
        .enumAllowed = { "contain", "cover", "fill", "scaleDown", "none" },
        .enumFallback = defaultFit,
        .applyValue = [this, defaultFit](
                          const JsonValue& value) { SetObjectFit(ParseObjectFit(value.GetStringValue(defaultFit))); } };
}

PropertyDeclaration ImageComponent::CreateVariantPropertyDeclaration()
{
    return PropertyDeclaration { .name = "variant",
        .type = PropertyValueType::ENUM_STRING,
        .allowDynamic = false,
        .fallbackString = "mediumFeature",
        .enumAllowed = { "icon", "avatar", "smallFeature", "mediumFeature", "largeFeature", "header" },
        .enumFallback = "mediumFeature",
        .applyValue = [this](const JsonValue& value) { ApplyVariantPreset(value.GetStringValue("mediumFeature")); } };
}

PropertyDeclaration ImageComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    if (propertyName == "url") {
        return CreateUrlPropertyDeclaration();
    }
    if (propertyName == "description") {
        return CreateDescriptionPropertyDeclaration();
    }
    if (propertyName == "fit") {
        return CreateFitPropertyDeclaration();
    }
    if (propertyName == "variant") {
        return CreateVariantPropertyDeclaration();
    }
    return {};
}

std::string ImageComponent::GetType() const
{
    return "Image";
}

std::vector<std::string> ImageComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return { "url" };
}

bool ImageComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    return A2UIComponent::IsKnownAdditionalDescriptorKey(propertyName) || propertyName == "width" ||
           propertyName == "height";
}

void ImageComponent::SetSrc(const std::string& src)
{
    ArkUINodeApiAdapter::ResetNodeImageSrc(nativeView_);
    if (!src.empty()) {
        ArkUINodeApiAdapter::SetNodeImageSrc(nativeView_, src);
    }
}

void ImageComponent::SetObjectFit(A2UIObjectFit objectFit)
{
    ArkUINodeApiAdapter::SetNodeImageObjectFit(nativeView_, objectFit);
}

void ImageComponent::SetAlt(const std::string& alt)
{
    ArkUINodeApiAdapter::ResetNodeImageAlt(nativeView_);
    if (alt.empty()) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeImageAlt(nativeView_, alt);
}

A2UIObjectFit ImageComponent::ParseObjectFit(const std::string& fit) const
{
    if (fit == "contain") {
        return A2UIObjectFit::CONTAIN;
    } else if (fit == "cover") {
        return A2UIObjectFit::COVER;
    } else if (fit == "fill") {
        return A2UIObjectFit::FILL;
    } else if (fit == "scaleDown") {
        return A2UIObjectFit::SCALE_DOWN;
    } else if (fit == "none") {
        return A2UIObjectFit::NONE;
    }
    return ResolveDefaultFitValueForVariant(g_imageFitFallbackVariant) == HEADER_IMAGE_FIT ? A2UIObjectFit::CONTAIN
                                                                                           : A2UIObjectFit::FILL;
}

void ImageComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    A2UIComponent::OnDataUpdate(property, value);
}

void ImageComponent::ApplyVariantPreset(const std::string& variant)
{
    SetBorderRadius(0.0F);
    if (variant == "icon") {
        SetWidth(VARIANT_ICON_WIDTH);
        SetHeight(VARIANT_ICON_HEIGHT);
        return;
    }
    if (variant == "avatar") {
        SetWidth(VARIANT_AVATAR_WIDTH);
        SetHeight(VARIANT_AVATAR_HEIGHT);
        SetBorderRadius(VARIANT_AVATAR_BORDER_RADIUS);
        return;
    }
    if (variant == "smallFeature") {
        SetWidth(VARIANT_SMALL_FEATURE_WIDTH);
        SetHeight(VARIANT_SMALL_FEATURE_HEIGHT);
        return;
    }
    if (variant == "largeFeature") {
        SetWidth(VARIANT_LARGE_FEATURE_WIDTH);
        SetHeight(VARIANT_LARGE_FEATURE_HEIGHT);
        return;
    }
    if (variant == "header") {
        SetWidthPercent(FULL_WIDTH_PERCENT);
        return;
    }

    SetWidth(VARIANT_MEDIUM_FEATURE_WIDTH);
    SetHeight(VARIANT_MEDIUM_FEATURE_HEIGHT);
}

void ImageComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ApplyCommonAttributes(descriptor);

    ApplySchemaProperty("url", descriptor);
    ApplySchemaProperty("description", descriptor);
    if (!descriptor.Has("description")) {
        SetAlt(DEFAULT_IMAGE_PLACEHOLDER_ALT);
    }
    ApplySchemaProperty("variant", descriptor);
    ScopedImageFitFallbackVariant scopedFitFallbackVariant(ResolveDescriptorVariantForFitFallback(descriptor));
    ApplySchemaProperty("fit", descriptor);

    if (descriptor.Has("weight") && descriptor.GetNumber("weight", 0.0) > 0.0) {
        ArkUINodeApiAdapter::ResetNodeWidth(nativeView_);
        ArkUINodeApiAdapter::ResetNodeHeight(nativeView_);
    }
}

std::shared_ptr<ImageTheme> ImageComponent::GetTheme()
{
    // Try to get from cache first
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }

    // Cache miss, get from base class
    std::shared_ptr<ThemeBase> baseTheme = A2UIComponent::GetTheme();
    if (baseTheme == nullptr) {
        return nullptr;
    }

    // Cast to specific type and cache it
    theme = std::dynamic_pointer_cast<ImageTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void ImageComponent::OnConfigChange(const ThemeContext& context)
{
    auto imageTheme = GetTheme();
    if (imageTheme == nullptr) {
        return;
    }

    // TODO: Re-apply styles based on new theme context
}

} // namespace NativeModule
