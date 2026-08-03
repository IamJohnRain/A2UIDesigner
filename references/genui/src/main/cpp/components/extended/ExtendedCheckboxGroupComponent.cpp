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

#include "ExtendedCheckboxGroupComponent.h"

#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "components/A2UI/checkbox/CheckboxGroupTheme.h"
#include "components/extended/ExtendedStyleResolver.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "theme/ThemeBase.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_CHECKBOX_GROUP_MARK_STROKE_WIDTH = 2.0f;
constexpr float DEFAULT_CHECKBOX_GROUP_MARK_SIZE = 20.0f;

std::string BuildNativeCheckboxGroupName(int32_t renderId, const std::string& surfaceId, const std::string& group)
{
    if (group.empty() || renderId < 0 || surfaceId.empty()) {
        return group;
    }
    return "a2ui:" + std::to_string(renderId) + ":" + surfaceId + ":" + group;
}

std::string SelectAllStatusToString(int32_t status)
{
    switch (status) {
        case 0:
            return "All";
        case 1:
            return "Part";
        case 2:
            return "None";
        default:
            return "None";
    }
}

JsonValue BuildGroupChangeEventContext(const std::vector<std::string>& names, int32_t status)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }
    JsonValue root = adapter->GetRoot();
    JsonValue namesArr = root.PutArray("value");
    for (const auto& n : names) {
        auto strAdapter = JsonAdapter::CreateString(n);
        if (strAdapter != nullptr) {
            namesArr.Append(strAdapter->GetRoot());
        }
    }
    root.PutString("status", SelectAllStatusToString(status));
    return root;
}

bool HasSupportedMarkMember(const JsonValue& markValue)
{
    return markValue.IsObject() &&
           (markValue.Has("strokeColor") || markValue.Has("size") || markValue.Has("strokeWidth"));
}

bool IsSupportedMarkMember(const std::string& key)
{
    return key == "strokeColor" || key == "size" || key == "strokeWidth";
}

float ResolveMarkNumber(const JsonValue& markValue, const char* key, float fallback)
{
    if (key == nullptr || !markValue.Has(key)) {
        return fallback;
    }
    JsonValue value = markValue.GetItem(key);
    if (!value.IsNumber()) {
        return fallback;
    }
    double number = value.GetNumberValue(static_cast<double>(fallback));
    if (!std::isfinite(number)) {
        return fallback;
    }
    return static_cast<float>(number);
}

} // namespace

ExtendedCheckboxGroupComponent::ExtendedCheckboxGroupComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::CHECKBOX_GROUP))
{
    if (nativeView_ == nullptr) {
        return;
    }

    checkboxGroupNode_ = nativeView_;

    ArkUINodeApiAdapter::AddNodeEventReceiver(checkboxGroupNode_, ExtendedCheckboxGroupComponent::NodeEventReceiver);

    auto theme = GetTheme();
    if (theme != nullptr) {
        selectedColor_ = theme->GetSelectedColor();
        unselectedColor_ = theme->GetUnselectedColor();
        markStrokeColor_ = theme->GetMarkStrokeColor();
    }

    SetSelectAll(selectAll_);
    SetSelectedColor(selectedColor_);
    SetUnselectedColor(unselectedColor_);
    SetMark(markStrokeColor_, markSize_, markStrokeWidth_);
    SetShape(shape_);
    SetGroup(group_);
}

ExtendedCheckboxGroupComponent::~ExtendedCheckboxGroupComponent()
{
    if (checkboxGroupNode_ != nullptr) {
        if (changeEventRegistered_) {
            ArkUINodeApiAdapter::UnregisterNodeEvent(checkboxGroupNode_, A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE);
        }
        ArkUINodeApiAdapter::RemoveNodeEventReceiver(
            checkboxGroupNode_, ExtendedCheckboxGroupComponent::NodeEventReceiver);
    }
}

std::string ExtendedCheckboxGroupComponent::GetType() const
{
    return "CheckboxGroup";
}

void ExtendedCheckboxGroupComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ApplyDeclaredPropertyOrFallback(descriptor, "group");
    ApplyDeclaredPropertyOrFallback(descriptor, "selectAll");
}

PropertyDeclaration ExtendedCheckboxGroupComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ExtendedCheckboxGroupComponent&)>>
        declarations = { { "group",
                             [](ExtendedCheckboxGroupComponent& component) {
                                 return PropertyDeclaration { .name = "group",
                                     .type = PropertyValueType::STRING,
                                     .allowDynamic = true,
                                     .allowExpression = true,
                                     .fallbackString = "",
                                     .applyValue = [&component](const JsonValue& value) {
                                         component.SetGroup(value.GetStringValue(""));
                                     } };
                             } },
            { "selectAll", [](ExtendedCheckboxGroupComponent& component) {
                 return PropertyDeclaration { .name = "selectAll",
                     .type = PropertyValueType::BOOLEAN,
                     .allowDynamic = true,
                     .allowExpression = true,
                     .fallbackBool = false,
                     .applyValue = [&component](
                                       const JsonValue& value) { component.SetSelectAll(value.GetBoolValue(false)); } };
             } } };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

void ExtendedCheckboxGroupComponent::ValidateStylesSchema(const JsonValue& styles)
{
    if (IsApplyingStyleDeltaUpdate() || !styles.IsObject()) {
        return;
    }

    if (styles.Has("unselectedColor")) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property styles.unselectedColor is undefined for CheckboxGroup and has been ignored",
            "styles.unselectedColor");
    }
    if (styles.Has("shape")) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property styles.shape is undefined for CheckboxGroup and has been ignored", "styles.shape");
    }

    const char* colorKeys[] = { "selectedColor", "unSelectedColor" };
    for (const char* key : colorKeys) {
        if (key == nullptr || !styles.Has(key)) {
            continue;
        }
        uint32_t color = 0;
        JsonValue colorValue = styles.GetItem(key);
        if (IsDynamicValueDescriptor(colorValue)) {
            continue;
        }
        if (!colorValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles." + std::string(key) + " expects string color, fallback/reset has been applied",
                "styles." + std::string(key));
            continue;
        }
        if (!ExtendedStyleResolver::ParseColor(colorValue, color)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles." + std::string(key) + " expects color value, fallback/reset has been applied",
                "styles." + std::string(key));
        }
    }

    if (styles.Has("checkboxShape")) {
        JsonValue shapeValue = styles.GetItem("checkboxShape");
        if (IsDynamicValueDescriptor(shapeValue)) {
            // Dynamic styles are handled by DFX fallback/reset after resolution.
        } else if (!shapeValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.checkboxShape expects string enum, fallback/reset has been applied",
                "styles.checkboxShape");
        } else {
            std::string shape = shapeValue.GetStringValue("");
            if (shape != "circle" && shape != "rounded_square") {
                ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles.checkboxShape expects circle/rounded_square, fallback/reset has been applied",
                    "styles.checkboxShape");
            }
        }
    }

    if (!styles.Has("mark")) {
        return;
    }
    JsonValue markValue = styles.GetItem("mark");
    if (IsDynamicValueDescriptor(markValue)) {
        return;
    }
    if (!markValue.IsObject()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.mark expects object, fallback/reset has been applied", "styles.mark");
        return;
    }
    for (JsonValue child = markValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (!key.empty() && !IsSupportedMarkMember(key)) {
            std::string path = "styles.mark." + key;
            ReportExtendedSchemaWarning(
                SCHEMA_ERROR_CODE_UNDEFINED_FIELD, "Property " + path + " is undefined and has been ignored", path);
        }
    }
    if (!HasSupportedMarkMember(markValue)) {
        return;
    }
    std::unique_ptr<JsonAdapter> resolvedMarkAdapter = ResolveMarkDynamicMembers(markValue);
    if (resolvedMarkAdapter != nullptr) {
        ValidateResolvedMarkDfx(resolvedMarkAdapter->GetRoot());
    }
    if (markValue.Has("strokeColor")) {
        uint32_t color = 0;
        JsonValue strokeColorValue = markValue.GetItem("strokeColor");
        if (IsDynamicValueDescriptor(strokeColorValue)) {
            // Dynamic mark values are handled by DFX fallback/reset after resolution.
        } else if (!strokeColorValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.mark.strokeColor expects string color, fallback/reset has been applied",
                "styles.mark.strokeColor");
        } else if (!ExtendedStyleResolver::ParseColor(strokeColorValue, color)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.mark.strokeColor expects color value, fallback/reset has been applied",
                "styles.mark.strokeColor");
        }
    }
    auto reportMarkNumber = [this, &markValue](const char* key) {
        if (key == nullptr || !markValue.Has(key)) {
            return;
        }
        JsonValue value = markValue.GetItem(key);
        if (IsDynamicValueDescriptor(value)) {
            return;
        }
        if (!value.IsNumber()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.mark." + std::string(key) + " expects number, fallback/reset has been applied",
                "styles.mark." + std::string(key));
            return;
        }
        if (value.GetNumberValue(0.0) <= 0.0) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.mark." + std::string(key) +
                    " expects positive number, fallback/reset has been applied",
                "styles.mark." + std::string(key));
        }
    };
    reportMarkNumber("size");
    reportMarkNumber("strokeWidth");
}

std::unique_ptr<JsonAdapter> ExtendedCheckboxGroupComponent::ResolveMarkDynamicMembers(const JsonValue& markValue) const
{
    if (!markValue.IsObject() || IsDynamicValueDescriptor(markValue)) {
        return std::unique_ptr<JsonAdapter>();
    }

    std::unique_ptr<JsonAdapter> resolvedAdapter = JsonAdapter::CreateObject();
    if (resolvedAdapter == nullptr) {
        return std::unique_ptr<JsonAdapter>();
    }
    JsonValue resolvedRoot = resolvedAdapter->GetRoot();
    bool hasDynamicMember = false;
    const RenderContext& renderContext = GetRenderContext();
    DynamicResolveContext context = { .renderId = renderContext.renderId,
        .surfaceId = renderContext.surfaceId,
        .componentId = GetComponentId(),
        .dataModel = renderContext.dataModel,
        .allowExpression = true };
    for (JsonValue child = markValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }
        if (IsDynamicValueDescriptor(child)) {
            hasDynamicMember = true;
            if (child.Has("path") && renderContext.bindingEngine != nullptr) {
                std::string path = child.GetString("path", "");
                std::shared_ptr<DataModel> dataModel =
                    renderContext.bindingEngine->GetOrCreateDataModel(renderContext.surfaceId);
                if (dataModel != nullptr) {
                    std::optional<JsonValue> value = dataModel->GetNode(path);
                    if (value.has_value()) {
                        resolvedRoot.Put(key.c_str(), value.value());
                        continue;
                    }
                }
            }
            ResolvedValue resolvedValue = DynamicValueResolver::Resolve(child, context);
            if (resolvedValue.success && resolvedValue.value.IsValid()) {
                resolvedRoot.Put(key.c_str(), resolvedValue.value);
                continue;
            }
        }
        resolvedRoot.Put(key.c_str(), child);
    }
    if (!hasDynamicMember) {
        return std::unique_ptr<JsonAdapter>();
    }
    return resolvedAdapter;
}

void ExtendedCheckboxGroupComponent::ValidateResolvedMarkDfx(const JsonValue& markValue)
{
    if (!markValue.IsObject()) {
        return;
    }
    if (markValue.Has("strokeColor")) {
        uint32_t color = 0;
        JsonValue strokeColorValue = markValue.GetItem("strokeColor");
        if (!strokeColorValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.mark.strokeColor expects string color, fallback/reset has been applied",
                "styles.mark.strokeColor");
        } else if (!ExtendedStyleResolver::ParseColor(strokeColorValue, color)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.mark.strokeColor expects color value, fallback/reset has been applied",
                "styles.mark.strokeColor");
        }
    }
    auto validateMarkNumber = [this, &markValue](const char* key) {
        if (key == nullptr || !markValue.Has(key)) {
            return;
        }
        JsonValue value = markValue.GetItem(key);
        if (!value.IsNumber()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.mark." + std::string(key) + " expects number, fallback/reset has been applied",
                "styles.mark." + std::string(key));
            return;
        }
        double number = value.GetNumberValue(0.0);
        if (!std::isfinite(number) || number <= 0.0) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.mark." + std::string(key) +
                    " expects positive number, fallback/reset has been applied",
                "styles.mark." + std::string(key));
        }
    };
    validateMarkNumber("size");
    validateMarkNumber("strokeWidth");
}

void ExtendedCheckboxGroupComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStylesSchema(styles);
}

void ExtendedCheckboxGroupComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();

    auto theme = GetTheme();
    uint32_t defaultSelectedColor = (theme != nullptr) ? theme->GetSelectedColor() : 0xFF007DFF;

    uint32_t defaultUnselectedColor = (theme != nullptr) ? theme->GetUnselectedColor() : 0x66182431;
    ApplyStyleColor(styles, "selectedColor", defaultSelectedColor, selectedColorOverridden_,
        &ExtendedCheckboxGroupComponent::SetSelectedColor);
    ApplyStyleColor(styles, "unSelectedColor", defaultUnselectedColor, unselectedColorOverridden_,
        &ExtendedCheckboxGroupComponent::SetUnselectedColor);

    if (styles.IsObject() && styles.Has("checkboxShape")) {
        JsonValue shapeValue = styles.GetItem("checkboxShape");
        std::string shapeStr = shapeValue.GetStringValue("circle");
        A2UICheckboxShape shape =
            (shapeStr == "rounded_square") ? A2UICheckboxShape::ROUNDED_SQUARE : A2UICheckboxShape::CIRCLE;
        SetShape(shape);
    } else if (!isDeltaUpdate) {
        SetShape(A2UICheckboxShape::CIRCLE);
    }

    uint32_t defaultMarkColor = (theme != nullptr) ? theme->GetMarkStrokeColor() : 0xFFFFFFFF;
    if (styles.IsObject() && styles.Has("mark")) {
        JsonValue markValue = styles.GetItem("mark");
        std::unique_ptr<JsonAdapter> resolvedMarkAdapter = ResolveMarkDynamicMembers(markValue);
        if (resolvedMarkAdapter != nullptr) {
            markValue = resolvedMarkAdapter->GetRoot();
        }
        if (markValue.IsObject()) {
            uint32_t strokeColor = defaultMarkColor;
            if (markValue.Has("strokeColor")) {
                JsonValue sc = markValue.GetItem("strokeColor");
                ExtendedStyleResolver::ParseColor(sc, strokeColor);
            }
            float size = ResolveMarkNumber(markValue, "size", DEFAULT_CHECKBOX_GROUP_MARK_SIZE);
            float strokeWidth = ResolveMarkNumber(markValue, "strokeWidth", DEFAULT_CHECKBOX_GROUP_MARK_STROKE_WIDTH);
            markOverridden_ = true;
            SetMark(strokeColor, size, strokeWidth);
        } else {
            markOverridden_ = false;
            SetMark(defaultMarkColor, DEFAULT_CHECKBOX_GROUP_MARK_SIZE, DEFAULT_CHECKBOX_GROUP_MARK_STROKE_WIDTH);
        }
    } else if (!isDeltaUpdate) {
        markOverridden_ = false;
        SetMark(defaultMarkColor, DEFAULT_CHECKBOX_GROUP_MARK_SIZE, DEFAULT_CHECKBOX_GROUP_MARK_STROKE_WIDTH);
    }
}

void ExtendedCheckboxGroupComponent::RegisterComponentSpecificListeners()
{
    UpdateChangeEventRegistration();
}

void ExtendedCheckboxGroupComponent::OnPropertyRemoved(const std::string& propertyName)
{
    if (propertyName == "selectAll") {
        SetSelectAll(false);
        UpdateChangeEventRegistration();
        return;
    }
    if (propertyName == "group") {
        SetGroup("");
        return;
    }
}

void ExtendedCheckboxGroupComponent::NodeEventReceiver(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    auto* component = reinterpret_cast<ExtendedCheckboxGroupComponent*>(ArkUINodeApiAdapter::GetUserData(nodeHandle));
    if (component != nullptr) {
        component->HandleNodeEvent(event);
    }
}

void ExtendedCheckboxGroupComponent::ApplyStyleColor(const JsonValue& styles, const char* propertyName,
    uint32_t fallbackColor, bool& overridden, void (ExtendedCheckboxGroupComponent::*setter)(uint32_t))
{
    JsonValue colorValue;
    bool hasColorValue = false;
    if (styles.IsObject() && propertyName != nullptr && styles.Has(propertyName)) {
        colorValue = styles.GetItem(propertyName);
        hasColorValue = true;
    }

    if (!hasColorValue && IsApplyingStyleDeltaUpdate()) {
        return;
    }

    uint32_t color = fallbackColor;
    if (hasColorValue) {
        overridden = ExtendedStyleResolver::ParseColor(colorValue, color);
    } else {
        overridden = false;
    }
    (this->*setter)(color);
}

void ExtendedCheckboxGroupComponent::HandleNodeEvent(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto eventType = ArkUIOHApiAdapter::NodeEventGetEventType(event);
    if (eventType != A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE) {
        return;
    }

    A2UIStringAsyncEvent* stringEvent = ArkUIOHApiAdapter::NodeEventGetStringAsyncEvent(event);
    if (stringEvent != nullptr && stringEvent->pStr != nullptr) {
        std::string eventStr(stringEvent->pStr);
        LOG_A2UI(LOG_INFO, "CheckboxGroup onChange stringEvent: [%{public}s]", eventStr.c_str());
        selectedNames_.clear();
        ParseNamesFromEventString(eventStr);
        ParseStatusFromEventString(eventStr);
    } else {
        LOG_A2UI(LOG_WARN, "CheckboxGroup onChange stringEvent is null, fallback to componentEvent");
        A2UINodeComponentEvent* componentEvent = ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
        if (componentEvent != nullptr) {
            selectedNames_.clear();
            selectAllStatus_ = componentEvent->data[0].i32;
            LOG_A2UI(LOG_INFO, "CheckboxGroup onChange componentEvent status=%{public}d", selectAllStatus_);
        } else {
            LOG_A2UI(LOG_WARN, "CheckboxGroup onChange componentEvent is also null");
        }
    }

    HandleCheckboxGroupChange();
}

void ExtendedCheckboxGroupComponent::ParseNamesFromEventString(const std::string& eventStr)
{
    auto namePos = eventStr.find("Name:");
    if (namePos == std::string::npos) {
        return;
    }
    size_t nameStart = namePos + 5;
    auto statusPos = eventStr.find("Status:", nameStart);
    std::string namesPart = (statusPos != std::string::npos) ? eventStr.substr(nameStart, statusPos - nameStart)
                                                             : eventStr.substr(nameStart);
    while (!namesPart.empty() && (namesPart.back() == ';' || namesPart.back() == ' ')) {
        namesPart.pop_back();
    }
    if (namesPart.empty()) {
        return;
    }
    std::istringstream iss(namesPart);
    std::string name;
    while (std::getline(iss, name, ',')) {
        if (!name.empty()) {
            selectedNames_.push_back(name);
        }
    }
}

void ExtendedCheckboxGroupComponent::ParseStatusFromEventString(const std::string& eventStr)
{
    auto pos = eventStr.find("Status:");
    if (pos != std::string::npos && pos + 7 < eventStr.length()) {
        int32_t status = eventStr[pos + 7] - '0';
        if (status >= 0 && status <= 2) {
            selectAllStatus_ = status;
        }
    }
}

void ExtendedCheckboxGroupComponent::HandleCheckboxGroupChange()
{
    DispatchEvent("onChange", BuildGroupChangeEventContext(selectedNames_, selectAllStatus_));
}

void ExtendedCheckboxGroupComponent::UpdateChangeEventRegistration()
{
    bool shouldRegister = HasEventHandler("onChange");
    if (checkboxGroupNode_ == nullptr) {
        changeEventRegistered_ = false;
        return;
    }
    if (shouldRegister && !changeEventRegistered_) {
        ArkUINodeApiAdapter::RegisterNodeEvent(
            checkboxGroupNode_, A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE, 0, this);
        changeEventRegistered_ = true;
        return;
    }

    if (!shouldRegister && changeEventRegistered_) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(checkboxGroupNode_, A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE);
        changeEventRegistered_ = false;
    }
}

void ExtendedCheckboxGroupComponent::SetSelectAll(bool selectAll)
{
    selectAll_ = selectAll;
    if (checkboxGroupNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxGroupSelectAll(checkboxGroupNode_, selectAll_);
}

void ExtendedCheckboxGroupComponent::SetSelectedColor(uint32_t color)
{
    selectedColor_ = color;
    if (checkboxGroupNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxGroupSelectedColor(checkboxGroupNode_, selectedColor_);
}

void ExtendedCheckboxGroupComponent::SetUnselectedColor(uint32_t color)
{
    unselectedColor_ = color;
    if (checkboxGroupNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxGroupUnselectedColor(checkboxGroupNode_, unselectedColor_);
}

void ExtendedCheckboxGroupComponent::SetMark(uint32_t strokeColor, float size, float strokeWidth)
{
    markStrokeColor_ = strokeColor;
    markSize_ = size;
    markStrokeWidth_ = strokeWidth;
    if (checkboxGroupNode_ == nullptr) {
        return;
    }
    if (markSize_ > 0.0f) {
        ArkUINodeApiAdapter::SetNodeCheckboxGroupMark(
            checkboxGroupNode_, markStrokeColor_, markSize_, markStrokeWidth_);
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxGroupMark(checkboxGroupNode_, markStrokeColor_);
}

void ExtendedCheckboxGroupComponent::SetShape(A2UICheckboxShape shape)
{
    shape_ = shape;
    if (checkboxGroupNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxGroupShape(checkboxGroupNode_, shape_);
}

void ExtendedCheckboxGroupComponent::SetGroup(const std::string& group)
{
    group_ = group;
    if (checkboxGroupNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxGroupName(
        checkboxGroupNode_, BuildNativeCheckboxGroupName(GetRenderId(), GetSurfaceId(), group_));
}

std::shared_ptr<CheckboxGroupTheme> ExtendedCheckboxGroupComponent::GetTheme()
{
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }
    std::shared_ptr<ThemeBase> baseTheme = Component::GetTheme();
    if (baseTheme == nullptr) {
        return nullptr;
    }
    theme = std::dynamic_pointer_cast<CheckboxGroupTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }
    return theme;
}

void ExtendedCheckboxGroupComponent::OnConfigChange(const ThemeContext& context)
{
    static_cast<void>(context);
    auto theme = GetTheme();
    if (theme == nullptr) {
        return;
    }
    if (!selectedColorOverridden_) {
        SetSelectedColor(theme->GetSelectedColor());
    }
    if (!unselectedColorOverridden_) {
        SetUnselectedColor(theme->GetUnselectedColor());
    }
    if (!markOverridden_) {
        SetMark(theme->GetMarkStrokeColor(), markSize_, markStrokeWidth_);
    }
}

} // namespace NativeModule
