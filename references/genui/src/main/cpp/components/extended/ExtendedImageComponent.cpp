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

#include "ExtendedImageComponent.h"

#include <cmath>
#include <unordered_set>

#include "styles/StyleApplyUtils.h"
#include "utils/LogA2UI.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_ASPECT_RATIO = 1.0F;
constexpr A2UIObjectFit DEFAULT_OBJECT_FIT = A2UIObjectFit::COVER;
constexpr char DEFAULT_IMAGE_PLACEHOLDER_ALT[] = "resources/base/media/placeHolder_E5E5EA.png";
enum class ImageDfxDecision { APPLIED, FALLBACK, PRESERVED };

// GCOVR_EXCL_BR_START
bool IsSupportedObjectFitToken(const std::string& value)
{
    static const std::unordered_set<std::string> supportedTokens = { "contain", "cover", "auto", "fill", "scaleDown",
        "none", "topStart", "top", "topEnd", "start", "center", "end", "bottomStart", "bottom", "bottomEnd", "matrix" };
    return supportedTokens.find(StyleApplyUtils::TrimToken(value)) != supportedTokens.end();
}
// GCOVR_EXCL_BR_STOP

bool HasNonObjectStylePayload(const JsonValue& styles)
{
    return styles.IsValid() && !styles.IsObject();
}

bool IsBindingDescriptorValue(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

bool TryParsePositiveAspectRatio(const JsonValue& value, float& ratio)
{
    if (!value.IsNumber()) {
        return false;
    }

    double rawRatio = value.GetNumberValue(0.0);
    if (!std::isfinite(rawRatio)) {
        return false;
    }

    ratio = static_cast<float>(rawRatio);
    return ratio > 0.0F;
}

const char* ToUpdateTypeLabel(bool isDeltaUpdate)
{
    static constexpr const char* UPDATE_TYPE_LABELS[] = { "full", "delta" };
    return UPDATE_TYPE_LABELS[static_cast<size_t>(isDeltaUpdate)];
}

const char* ToDecisionLabel(ImageDfxDecision decision)
{
    static constexpr const char* DECISION_LABELS[] = { "applied", "fallback", "preserved" };
    return DECISION_LABELS[static_cast<size_t>(decision)];
}

const char* DescribeStyleResolution(bool isDeltaUpdate)
{
    static_cast<void>(isDeltaUpdate);
    return "fallback to default value";
}

std::string DescribeJsonValueForLog(const JsonValue& value)
{
    return value.IsValid() ? value.GetTypeName() : "missing";
}

void LogImageDfxEvent(const std::string& componentId, const char* event, const char* field, const JsonValue& value,
    bool isDeltaUpdate, ImageDfxDecision decision, const std::string& extra = "")
{
    LOG_A2UI(LOG_INFO,
        "ExtendedImageComponent::%{public}s - componentId=%{public}s, updateType=%{public}s, field=%{public}s, "
        "input=%{public}s, result=%{public}s, extra=%{public}s",
        event, componentId.c_str(), ToUpdateTypeLabel(isDeltaUpdate), field, DescribeJsonValueForLog(value).c_str(),
        ToDecisionLabel(decision), extra.c_str());
}

} // namespace

ExtendedImageComponent::ExtendedImageComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::IMAGE))
{
    SetAspectRatio(DEFAULT_ASPECT_RATIO);
    SetObjectFit(DEFAULT_OBJECT_FIT);
}

std::string ExtendedImageComponent::GetType() const
{
    return "Image";
}

void ExtendedImageComponent::ReportStyleWarning(
    const std::string& code, const std::string& styleName, const std::string& message) const
{
    ReportSchemaWarning(code, message, "styles." + styleName);
}

void ExtendedImageComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    JsonValue srcValue = descriptor.GetItem("src");
    bool hasSrc = descriptor.IsObject() && descriptor.Has("src");
    bool isSupportedSrcValue = srcValue.IsString() || IsBindingDescriptorValue(srcValue);
    ImageDfxDecision srcDecision = ImageDfxDecision::FALLBACK;
    if (hasSrc && isSupportedSrcValue) {
        srcDecision = ImageDfxDecision::APPLIED;
    }
    if (!hasSrc) {
        ReportSchemaWarning(
            SCHEMA_ERROR_CODE_INVALID_VALUE, "Property src is missing, fallback to empty string", "src");
        ApplyRuntimeProperty("src", JsonValue(), false);
    } else if (isSupportedSrcValue) {
        ApplyDeclaredPropertyOrFallback(descriptor, "src");
    } else {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            std::string("Property src expects string value, got type '") + srcValue.GetTypeName() +
                "', fallback to empty string",
            "src");
        RemoveBindingsForProperty("src");
        ApplyRuntimeProperty("src", JsonValue(), false);
    }
    LogImageDfxEvent(GetComponentId(), "ApplyPrivateAttributes", "src", srcValue, false, srcDecision,
        "srcLength=" + std::to_string(srcValue_.length()));
    SetAlt(DEFAULT_IMAGE_PLACEHOLDER_ALT);
}

PropertyDeclaration ExtendedImageComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    // GCOVR_EXCL_BR_START
    if (propertyName == "src") {
        PropertyDeclaration declaration;
        declaration.name = "src";
        declaration.type = PropertyValueType::STRING;
        declaration.allowDynamic = true;
        declaration.allowExpression = true;
        declaration.fallbackString = "";
        declaration.applyValue = [this](const JsonValue& value) { SetSrc(value.GetStringValue("")); };
        return declaration;
    }
    // GCOVR_EXCL_BR_STOP
    return {};
}

void ExtendedImageComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    const std::string componentId = GetComponentId();

    if (HasNonObjectStylePayload(styles)) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles expects object value, got type '" + std::string(styles.GetTypeName()) + "', " +
                DescribeStyleResolution(isDeltaUpdate),
            "styles");
        SetAspectRatio(DEFAULT_ASPECT_RATIO);
        SetObjectFit(DEFAULT_OBJECT_FIT);
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "aspectRatio", styles, isDeltaUpdate,
            ImageDfxDecision::FALLBACK, "aspectRatio=" + std::to_string(aspectRatio_));
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "objectFit", styles, isDeltaUpdate,
            ImageDfxDecision::FALLBACK, "objectFit=" + std::to_string(static_cast<int32_t>(objectFit_)));
        return;
    }

    float ratio = 0.0F;
    JsonValue aspectRatioValue = styles.GetItem("aspectRatio");
    if (TryParsePositiveAspectRatio(aspectRatioValue, ratio)) {
        SetAspectRatio(ratio);
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "aspectRatio", aspectRatioValue, isDeltaUpdate,
            ImageDfxDecision::APPLIED, "aspectRatio=" + std::to_string(aspectRatio_));
    } else if (aspectRatioValue.IsValid()) {
        if (aspectRatioValue.IsNumber()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "aspectRatio",
                "Property aspectRatio got invalid number value, " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "aspectRatio",
                std::string("Property aspectRatio expects number value, got type '") + aspectRatioValue.GetTypeName() +
                    "', " + DescribeStyleResolution(isDeltaUpdate));
        }
        SetAspectRatio(DEFAULT_ASPECT_RATIO);
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "aspectRatio", aspectRatioValue, isDeltaUpdate,
            ImageDfxDecision::FALLBACK, "aspectRatio=" + std::to_string(aspectRatio_));
    } else if (!isDeltaUpdate) {
        SetAspectRatio(DEFAULT_ASPECT_RATIO);
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "aspectRatio", aspectRatioValue, isDeltaUpdate,
            ImageDfxDecision::FALLBACK, "aspectRatio=" + std::to_string(aspectRatio_));
    }

    JsonValue objectFitValue = styles.GetItem("objectFit");
    A2UIObjectFit objectFit = DEFAULT_OBJECT_FIT;
    if (objectFitValue.IsString() &&
        IsSupportedObjectFitToken(objectFitValue.GetStringValue(""))) { // GCOVR_EXCL_BR_LINE
        objectFit = ArkUINodeApiAdapter::ParseImageObjectFit(
            objectFitValue.GetStringValue(""), DEFAULT_OBJECT_FIT, GetRenderContext().apiVersion);
        SetObjectFit(objectFit);
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "objectFit", objectFitValue, isDeltaUpdate,
            ImageDfxDecision::APPLIED, "objectFit=" + std::to_string(static_cast<int32_t>(objectFit_)));
    } else if (objectFitValue.IsValid()) {
        if (objectFitValue.IsString()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "objectFit",
                "Property objectFit got invalid enum value '" +
                    StyleApplyUtils::TrimToken(objectFitValue.GetStringValue("")) + "', " +
                    DescribeStyleResolution(isDeltaUpdate));
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "objectFit",
                std::string("Property objectFit expects string enum value, got type '") + objectFitValue.GetTypeName() +
                    "', " + DescribeStyleResolution(isDeltaUpdate));
        }
        SetObjectFit(DEFAULT_OBJECT_FIT);
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "objectFit", objectFitValue, isDeltaUpdate,
            ImageDfxDecision::FALLBACK, "objectFit=" + std::to_string(static_cast<int32_t>(objectFit_)));
    } else if (!isDeltaUpdate) {
        SetObjectFit(DEFAULT_OBJECT_FIT);
        LogImageDfxEvent(componentId, "ApplyComponentSpecificStyles", "objectFit", objectFitValue, isDeltaUpdate,
            ImageDfxDecision::FALLBACK, "objectFit=" + std::to_string(static_cast<int32_t>(objectFit_)));
    }
}

void ExtendedImageComponent::SetSrc(const std::string& src)
{
    srcValue_ = src;
    if (nativeView_ == nullptr) {
        return;
    }
    if (srcValue_.empty()) {
        ArkUINodeApiAdapter::ResetNodeImageSrc(nativeView_);
        return;
    }

    ArkUINodeApiAdapter::SetNodeImageSrc(nativeView_, srcValue_);
}

void ExtendedImageComponent::SetAlt(const std::string& alt)
{
    altValue_ = alt;
    if (nativeView_ == nullptr) {
        return;
    }

    if (altValue_.empty()) {
        ArkUINodeApiAdapter::ResetNodeImageAlt(nativeView_);
        return;
    }

    ArkUINodeApiAdapter::SetNodeImageAlt(nativeView_, altValue_);
}

void ExtendedImageComponent::SetAspectRatio(float ratio)
{
    aspectRatio_ = ratio;
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeAspectRatio(nativeView_, aspectRatio_);
}

void ExtendedImageComponent::SetObjectFit(A2UIObjectFit objectFit)
{
    objectFit_ = objectFit;
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeImageObjectFit(nativeView_, objectFit_);
}

} // namespace NativeModule
