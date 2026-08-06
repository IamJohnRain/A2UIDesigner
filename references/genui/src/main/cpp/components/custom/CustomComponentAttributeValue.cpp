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

#include "components/custom/CustomComponent.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "theme/ThemeManager.h"
#include "utils/NapiUtils.h"

#include "NapiBridge.h"
#include "RenderManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

constexpr const char* TYPE_KEY = "type";
constexpr const char* ID_KEY = "id";
constexpr const char* CUSTOM_PROPS_KEY = "customProps";
constexpr const char* PROPERTIES_KEY = "properties";
constexpr const char* DATA_MODEL_JSON_KEY = "dataModelJson";
constexpr const char* SIZE_KEY = "size";
constexpr const char* PADDING_KEY = "padding";
constexpr const char* MARGIN_KEY = "margin";
constexpr const char* WIDTH_KEY = "width";
constexpr const char* HEIGHT_KEY = "height";
constexpr const char* FLEX_SHRINK_PARENT_DEFAULT_KEY = "flexShrinkParentDefault";
constexpr const char* RESET_FLEX_SHRINK_TO_PARENT_DEFAULT_KEY = "resetFlexShrinkToParentDefault";

bool HasCommonStyleProps(const CommonStyleProps& properties)
{
    return properties.hasWidth || properties.hasHeight || properties.hasWeight || !properties.size.empty() ||
           !properties.padding.empty() || properties.hasMargin || properties.hasAccessibilityLabel ||
           properties.hasAccessibilityDescription || properties.hasFlexShrinkParentDefault ||
           properties.resetFlexShrinkToParentDefault;
}

void SetStringProperty(napi_env env, napi_value object, const char* key, const std::string& value)
{
    napi_value propertyValue = nullptr;
    auto& napi = NapiBridge::GetInstance().Provider();
    napi.CreateStringUtf8(env, value.c_str(), NAPI_AUTO_LENGTH, &propertyValue);
    napi.SetNamedProperty(env, object, key, propertyValue);
}

void SetDoubleProperty(napi_env env, napi_value object, const char* key, double value)
{
    napi_value propertyValue = nullptr;
    auto& napi = NapiBridge::GetInstance().Provider();
    napi.CreateDouble(env, value, &propertyValue);
    napi.SetNamedProperty(env, object, key, propertyValue);
}

void SetBoolProperty(napi_env env, napi_value object, const char* key, bool value)
{
    napi_value propertyValue = nullptr;
    auto& napi = NapiBridge::GetInstance().Provider();
    napi.CreateBoolean(env, value, &propertyValue);
    napi.SetNamedProperty(env, object, key, propertyValue);
}

} // namespace

void CustomComponent::PopulateAttributeIdentity(napi_value attributeValue) const
{
    auto& napi = NapiBridge::GetInstance().Provider();
    SetStringProperty(env_, attributeValue, TYPE_KEY, descriptor_.type);
    SetStringProperty(env_, attributeValue, ID_KEY, descriptor_.id);
    if (!descriptor_.surfaceId.empty()) {
        SetStringProperty(env_, attributeValue, "surfaceId", descriptor_.surfaceId);
    }

    napi_value renderIdValue = nullptr;
    napi.CreateInt32(env_, renderId_, &renderIdValue);
    napi.SetNamedProperty(env_, attributeValue, "renderId", renderIdValue);
    napi_value handleValue = nullptr;
    napi.CreateDouble(env_, static_cast<double>(GetCustomComponentHandle()), &handleValue);
    napi.SetNamedProperty(env_, attributeValue, "customComponentHandle", handleValue);
    if (!GetSurfaceContext().a2UIProtocolVersion.empty()) {
        SetStringProperty(env_, attributeValue, "protocolVersion", GetSurfaceContext().a2UIProtocolVersion);
    }
    if (!GetSurfaceContext().catalogId.empty()) {
        SetStringProperty(env_, attributeValue, "catalogId", GetSurfaceContext().catalogId);
    }
}

void CustomComponent::PopulateAttributeThemeAndCustomProps(napi_value attributeValue) const
{
    ThemeContext themeContext;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        themeContext = themeManager->GetContext();
    }
    SetComponentThemeProperty(env_, attributeValue, themeContext);

    if (descriptor_.customProps.IsValid()) {
        NapiBridge::GetInstance().Provider().SetNamedProperty(
            env_, attributeValue, CUSTOM_PROPS_KEY, JsonValueToNapiValue(env_, descriptor_.customProps));
    }
}

void CustomComponent::PopulateAttributeCommonProperties(napi_value attributeValue) const
{
    if (!HasCommonStyleProps(descriptor_.properties)) {
        return;
    }

    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value propertiesValue = nullptr;
    napi.CreateObject(env_, &propertiesValue);
    if (!descriptor_.properties.size.empty()) {
        SetStringProperty(env_, propertiesValue, SIZE_KEY, descriptor_.properties.size);
    }
    if (!descriptor_.properties.padding.empty()) {
        SetStringProperty(env_, propertiesValue, PADDING_KEY, descriptor_.properties.padding);
    }
    if (descriptor_.properties.hasMargin) {
        SetStringProperty(env_, propertiesValue, MARGIN_KEY, descriptor_.properties.margin);
    }
    if (descriptor_.properties.hasWidth) {
        SetDoubleProperty(env_, propertiesValue, WIDTH_KEY, descriptor_.properties.width);
    }
    if (descriptor_.properties.hasHeight) {
        SetDoubleProperty(env_, propertiesValue, HEIGHT_KEY, descriptor_.properties.height);
    }
    if (descriptor_.properties.hasWeight) {
        SetDoubleProperty(env_, propertiesValue, "weight", descriptor_.properties.weight);
    }
    if (descriptor_.properties.hasAccessibilityLabel) {
        SetStringProperty(env_, propertiesValue, "accessibilityLabel", descriptor_.properties.accessibilityLabel);
    }
    if (descriptor_.properties.hasAccessibilityDescription) {
        SetStringProperty(
            env_, propertiesValue, "accessibilityDescription", descriptor_.properties.accessibilityDescription);
    }
    if (descriptor_.properties.hasFlexShrinkParentDefault) {
        SetDoubleProperty(
            env_, propertiesValue, FLEX_SHRINK_PARENT_DEFAULT_KEY, descriptor_.properties.flexShrinkParentDefault);
    }
    if (descriptor_.properties.resetFlexShrinkToParentDefault) {
        SetBoolProperty(env_, propertiesValue, RESET_FLEX_SHRINK_TO_PARENT_DEFAULT_KEY, true);
    }
    napi.SetNamedProperty(env_, attributeValue, PROPERTIES_KEY, propertiesValue);
}

void CustomComponent::PopulateAttributeDataModel(napi_value attributeValue) const
{
    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot == nullptr) {
        return;
    }
    std::shared_ptr<BindingEngine> bindingEngine = surfaceSlot->GetBindingEngine();
    if (bindingEngine == nullptr) {
        return;
    }
    std::shared_ptr<DataModel> dataModel = bindingEngine->GetOrCreateDataModel(surfaceId);
    if (dataModel == nullptr || dataModel->GetRoot() == nullptr || !dataModel->GetRoot()->IsValid()) {
        return;
    }

    std::string dataModelJson = dataModel->GetRoot()->ToJsonLiteral();
    if (dataModelJson.empty() || dataModelJson == "null") {
        return;
    }
    SetStringProperty(env_, attributeValue, DATA_MODEL_JSON_KEY, dataModelJson);
}

napi_value CustomComponent::CreateAttributeValue() const
{
    napi_value attributeValue = nullptr;
    NapiBridge::GetInstance().Provider().CreateObject(env_, &attributeValue);
    PopulateAttributeIdentity(attributeValue);
    PopulateAttributeThemeAndCustomProps(attributeValue);
    PopulateAttributeCommonProperties(attributeValue);
    PopulateAttributeDataModel(attributeValue);
    return attributeValue;
}

} // namespace NativeModule
