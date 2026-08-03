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

#include "ExtendedCheckboxComponent.h"

#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "components/extended/ExtendedStyleResolver.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "theme/ThemeBase.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"
#include "RenderManager.h"
#include "SchemaErrorCodes.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_CHECKBOX_MARK_STROKE_WIDTH = 2.0f;
constexpr float DEFAULT_CHECKBOX_MARK_SIZE = 20.0f;

std::string BuildNativeCheckboxGroupName(int32_t renderId, const std::string& surfaceId, const std::string& group)
{
    if (group.empty() || renderId < 0 || surfaceId.empty()) {
        return group;
    }
    return "a2ui:" + std::to_string(renderId) + ":" + surfaceId + ":" + group;
}

constexpr const char* CHECKBOX_SELECT_RUNTIME_STATE_SCOPE = "ExtendedCheckbox.select";
constexpr const char* RUNTIME_STATE_GROUP_KEY = "group";
constexpr const char* RUNTIME_STATE_KEY_KEY = "key";
constexpr const char* RUNTIME_STATE_VALUE_KEY = "value";
constexpr const char* RUNTIME_STATE_SELECT_KEY = "select";

JsonValue BuildChangeEventContext(bool value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }
    JsonValue root = adapter->GetRoot();
    root.PutBool("value", value);
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

ExtendedCheckboxComponent::ExtendedCheckboxComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::ROW))
{
    if (nativeView_ == nullptr) {
        return;
    }

    textNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT);
    checkboxNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::CHECKBOX);

    if (checkboxNode_ == nullptr || textNode_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::AddChild(nativeView_, checkboxNode_);
    ArkUINodeApiAdapter::AddChild(nativeView_, textNode_);

    ArkUINodeApiAdapter::SetNodeWidth(checkboxNode_, 20.0f);
    ArkUINodeApiAdapter::SetNodeHeight(checkboxNode_, 20.0f);
    ArkUINodeApiAdapter::SetNodeMargin(checkboxNode_, 2.0f, 2.0f, 2.0f, 2.0f);

    ArkUINodeApiAdapter::SetNodeHeight(nativeView_, 48.0f);

    ArkUINodeApiAdapter::SetNodeLayoutWeight(textNode_, 1U);

    ArkUINodeApiAdapter::SetNodeTextMaxLines(textNode_, 1);

    ArkUINodeApiAdapter::SetNodeTextOverflow(textNode_, 2);

    ArkUINodeApiAdapter::SetNodeMargin(textNode_, 0.0f, 0.0f, 0.0f, 12.0f);

    ArkUINodeApiAdapter::AddNodeEventReceiver(checkboxNode_, ExtendedCheckboxComponent::NodeEventReceiver);
    ArkUINodeApiAdapter::SetUserData(checkboxNode_, this);

    ArkUINodeApiAdapter::SetNodeRowAlignItems(nativeView_, A2UIVerticalAlignment::CENTER);

    auto theme = GetTheme();
    if (theme != nullptr) {
        selectedColor_ = theme->GetSelectedColor();
        unselectedColor_ = theme->GetUnselectedColor();
        markStrokeColor_ = theme->GetMarkStrokeColor();
    }

    SetSelect(select_);
    SetSelectedColor(selectedColor_);
    SetUnselectedColor(unselectedColor_);
    SetMark(markStrokeColor_, markSize_, markStrokeWidth_);
    SetShape(shape_);
    SetLabel(label_);
    SetGroup(group_);
}

ExtendedCheckboxComponent::~ExtendedCheckboxComponent()
{
    if (checkboxNode_ != nullptr) {
        if (changeEventRegistered_) {
            ArkUINodeApiAdapter::UnregisterNodeEvent(checkboxNode_, A2UINodeEventType::CHECKBOX_ON_CHANGE);
        }
        if (HasEventHandler("onClick")) {
            ArkUINodeApiAdapter::UnregisterNodeEvent(checkboxNode_, A2UINodeEventType::ON_CLICK);
        }
        ArkUINodeApiAdapter::RemoveNodeEventReceiver(checkboxNode_, ExtendedCheckboxComponent::NodeEventReceiver);
        ArkUINodeApiAdapter::ResetNodeCheckboxGroup(checkboxNode_);
        ArkUINodeApiAdapter::ResetNodeCheckboxName(checkboxNode_);
        ArkUINodeApiAdapter::RemoveChild(nativeView_, checkboxNode_);
        ArkUINodeApiAdapter::DisposeNode(checkboxNode_);
        checkboxNode_ = nullptr;
    }
    if (textNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, textNode_);
        ArkUINodeApiAdapter::DisposeNode(textNode_);
        textNode_ = nullptr;
    }
}

std::string ExtendedCheckboxComponent::GetType() const
{
    return "Checkbox";
}

bool ExtendedCheckboxComponent::GetSelect() const
{
    return select_;
}

std::string ExtendedCheckboxComponent::GetValue() const
{
    return value_;
}

std::string ExtendedCheckboxComponent::GetLabel() const
{
    return label_;
}

std::string ExtendedCheckboxComponent::GetGroup() const
{
    return group_;
}

std::string ExtendedCheckboxComponent::GetRuntimeStateScope() const
{
    return CHECKBOX_SELECT_RUNTIME_STATE_SCOPE;
}

std::string ExtendedCheckboxComponent::GetRuntimeStateKey() const
{
    std::string stateValue = ResolveRuntimeStateValue();
    if (stateValue.empty()) {
        return "";
    }
    return group_ + "\n" + stateValue;
}

JsonValue ExtendedCheckboxComponent::CaptureRuntimeState() const
{
    std::string stateValue = ResolveRuntimeStateValue();
    if (stateValue.empty()) {
        return JsonValue();
    }
    std::unique_ptr<JsonAdapter> stateAdapter = JsonAdapter::CreateObject();
    if (stateAdapter == nullptr) {
        return JsonValue();
    }
    JsonValue root = stateAdapter->GetRoot();
    root.PutString(RUNTIME_STATE_GROUP_KEY, group_);
    root.PutString(RUNTIME_STATE_KEY_KEY, stateValue);
    root.PutString(RUNTIME_STATE_VALUE_KEY, value_);
    root.PutBool(RUNTIME_STATE_SELECT_KEY, select_);
    return root;
}

void ExtendedCheckboxComponent::RestoreRuntimeState(const JsonValue& state)
{
    if (hasExplicitSelect_) {
        return;
    }
    if (!state.IsObject() || !state.GetItem(RUNTIME_STATE_SELECT_KEY).IsBool()) {
        return;
    }
    std::string stateGroup = state.GetString(RUNTIME_STATE_GROUP_KEY, "");
    std::string stateValue = state.GetString(RUNTIME_STATE_KEY_KEY, "");
    if (stateGroup != group_ || stateValue != ResolveRuntimeStateValue()) {
        return;
    }
    SetSelect(state.GetBool(RUNTIME_STATE_SELECT_KEY, false));
}

bool ExtendedCheckboxComponent::HasExplicitSelect() const
{
    return hasExplicitSelect_;
}

bool ExtendedCheckboxComponent::HasExplicitShape() const
{
    return hasExplicitShape_;
}

void ExtendedCheckboxComponent::ApplyInheritedSelect(bool select)
{
    if (hasExplicitSelect_) {
        return;
    }
    if (TryRestoreSelectFromRuntimeState()) {
        return;
    }
    SetSelect(select);
}

void ExtendedCheckboxComponent::ApplyInheritedShape(int32_t shape)
{
    if (hasExplicitShape_) {
        return;
    }
    SetShape(static_cast<A2UICheckboxShape>(shape));
}

void ExtendedCheckboxComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    hasExplicitSelect_ = descriptor.IsObject() && descriptor.Has("select");
    ApplyDeclaredPropertyOrFallback(descriptor, "label");
    ApplyDeclaredPropertyOrFallback(descriptor, "select");
    ApplyDeclaredPropertyOrFallback(descriptor, "value");
    ApplyDeclaredPropertyOrFallback(descriptor, "group");
}

PropertyDeclaration ExtendedCheckboxComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ExtendedCheckboxComponent&)>> declarations = {
        { "label",
            [](ExtendedCheckboxComponent& component) {
                return PropertyDeclaration { .name = "label",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetLabel(value.GetStringValue("")); } };
            } },
        { "select",
            [](ExtendedCheckboxComponent& component) {
                return PropertyDeclaration { .name = "select",
                    .type = PropertyValueType::BOOLEAN,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackBool = false,
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetSelect(value.GetBoolValue(false)); } };
            } },
        { "value",
            [](ExtendedCheckboxComponent& component) {
                return PropertyDeclaration { .name = "value",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](const JsonValue& val) { component.SetValue(val.GetStringValue("")); } };
            } },
        { "group",
            [](ExtendedCheckboxComponent& component) {
                return PropertyDeclaration { .name = "group",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetGroup(value.GetStringValue("")); } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

void ExtendedCheckboxComponent::ValidateStylesSchema(const JsonValue& styles)
{
    if (IsApplyingStyleDeltaUpdate() || !styles.IsObject()) {
        return;
    }

    const char* colorKeys[] = { "selectedColor", "unselectedColor" };
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

    if (styles.Has("shape")) {
        JsonValue shapeValue = styles.GetItem("shape");
        if (IsDynamicValueDescriptor(shapeValue)) {
            // Dynamic styles are handled by DFX fallback/reset after resolution.
        } else if (!shapeValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.shape expects string enum, fallback/reset has been applied", "styles.shape");
        } else {
            std::string shape = shapeValue.GetStringValue("");
            if (shape != "circle" && shape != "rounded_square") {
                ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles.shape expects circle/rounded_square, fallback/reset has been applied",
                    "styles.shape");
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

std::unique_ptr<JsonAdapter> ExtendedCheckboxComponent::ResolveMarkDynamicMembers(const JsonValue& markValue) const
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

void ExtendedCheckboxComponent::ValidateResolvedMarkDfx(const JsonValue& markValue)
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

void ExtendedCheckboxComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStylesSchema(styles);
}

void ExtendedCheckboxComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (!isDeltaUpdate) {
        hasExplicitShape_ = styles.IsObject() && styles.Has("shape");
    } else if (styles.IsObject() && styles.Has("shape")) {
        hasExplicitShape_ = true;
    }

    auto theme = GetTheme();
    uint32_t defaultSelectedColor = (theme != nullptr) ? theme->GetSelectedColor() : 0xFF317AF7;

    uint32_t defaultUnselectedColor = (theme != nullptr) ? theme->GetUnselectedColor() : 0x66000000;
    ApplyStyleColor(styles, "selectedColor", nullptr, defaultSelectedColor, selectedColorOverridden_,
        &ExtendedCheckboxComponent::SetSelectedColor);
    ApplyStyleColor(styles, "unselectedColor", nullptr, defaultUnselectedColor, unselectedColorOverridden_,
        &ExtendedCheckboxComponent::SetUnselectedColor);

    if (styles.IsObject() && styles.Has("shape")) {
        JsonValue shapeValue = styles.GetItem("shape");
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
            float size = ResolveMarkNumber(markValue, "size", DEFAULT_CHECKBOX_MARK_SIZE);
            float strokeWidth = ResolveMarkNumber(markValue, "strokeWidth", DEFAULT_CHECKBOX_MARK_STROKE_WIDTH);
            markOverridden_ = true;
            SetMark(strokeColor, size, strokeWidth);
        } else {
            markOverridden_ = false;
            SetMark(defaultMarkColor, DEFAULT_CHECKBOX_MARK_SIZE, DEFAULT_CHECKBOX_MARK_STROKE_WIDTH);
        }
    } else if (!isDeltaUpdate) {
        markOverridden_ = false;
        SetMark(defaultMarkColor, DEFAULT_CHECKBOX_MARK_SIZE, DEFAULT_CHECKBOX_MARK_STROKE_WIDTH);
    }
}

void ExtendedCheckboxComponent::RegisterComponentSpecificListeners()
{
    UpdateChangeEventRegistration();
    if (HasEventHandler("onClick") && checkboxNode_ != nullptr) {
        ArkUINodeApiAdapter::RegisterNodeEvent(checkboxNode_, A2UINodeEventType::ON_CLICK, 0, this);
    }
}

void ExtendedCheckboxComponent::OnPropertyRemoved(const std::string& propertyName)
{
    if (propertyName == "select") {
        SetSelect(false);
        UpdateChangeEventRegistration();
        return;
    }
    if (propertyName == "label") {
        SetLabel("");
        return;
    }
    if (propertyName == "value") {
        SetValue("");
        return;
    }
    if (propertyName == "group") {
        SetGroup("");
        return;
    }
}

void ExtendedCheckboxComponent::NodeEventReceiver(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    auto* component = reinterpret_cast<ExtendedCheckboxComponent*>(ArkUINodeApiAdapter::GetUserData(nodeHandle));
    if (component != nullptr) {
        component->HandleNodeEvent(event);
    }
}

void ExtendedCheckboxComponent::ApplyStyleColor(const JsonValue& styles, const char* propertyName,
    const char* aliasName, uint32_t fallbackColor, bool& overridden,
    void (ExtendedCheckboxComponent::*setter)(uint32_t))
{
    JsonValue colorValue;
    bool hasColorValue = false;
    if (styles.IsObject() && propertyName != nullptr && styles.Has(propertyName)) {
        colorValue = styles.GetItem(propertyName);
        hasColorValue = true;
    } else if (styles.IsObject() && aliasName != nullptr && styles.Has(aliasName)) {
        colorValue = styles.GetItem(aliasName);
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

void ExtendedCheckboxComponent::HandleNodeEvent(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto eventType = ArkUIOHApiAdapter::NodeEventGetEventType(event);

    if (eventType == A2UINodeEventType::ON_CLICK) {
        DispatchEvent("onClick");
        return;
    }

    if (eventType == A2UINodeEventType::CHECKBOX_ON_CHANGE) {
        A2UINodeComponentEvent* componentEvent = ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
        if (componentEvent == nullptr) {
            return;
        }
        HandleCheckboxChange(componentEvent->data[0].i32 == 1);
    }
}

void ExtendedCheckboxComponent::HandleCheckboxChange(bool isChecked)
{
    LOG_A2UI(LOG_DEBUG,
        "ExtendedCheckboxComponent::HandleCheckboxChange: componentId=%{public}s, isChecked=%{public}s, "
        "currentSelect=%{public}s",
        GetComponentId().c_str(), isChecked ? "true" : "false", select_ ? "true" : "false");
    if (isChecked == select_) {
        return;
    }

    SetSelect(isChecked);
    SyncRuntimeStateToSurface();
    SyncSelectToBoundDataModel(isChecked);
    DispatchEvent("onChange", BuildChangeEventContext(isChecked));
}

void ExtendedCheckboxComponent::UpdateChangeEventRegistration()
{
    if (checkboxNode_ == nullptr) {
        changeEventRegistered_ = false;
        return;
    }
    if (!changeEventRegistered_) {
        ArkUINodeApiAdapter::RegisterNodeEvent(checkboxNode_, A2UINodeEventType::CHECKBOX_ON_CHANGE, 0, this);
        changeEventRegistered_ = true;
    }
}

void ExtendedCheckboxComponent::SetSelect(bool select)
{
    select_ = select;
    if (checkboxNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxSelect(checkboxNode_, select_);
}

void ExtendedCheckboxComponent::SetValue(const std::string& value)
{
    value_ = value;
}

void ExtendedCheckboxComponent::SetSelectedColor(uint32_t color)
{
    selectedColor_ = color;
    if (checkboxNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxSelectColor(checkboxNode_, selectedColor_);
}

void ExtendedCheckboxComponent::SetUnselectedColor(uint32_t color)
{
    unselectedColor_ = color;
    if (checkboxNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxUnselectColor(checkboxNode_, unselectedColor_);
}

void ExtendedCheckboxComponent::SetMark(uint32_t strokeColor, float size, float strokeWidth)
{
    markStrokeColor_ = strokeColor;
    markSize_ = size;
    markStrokeWidth_ = strokeWidth;
    if (checkboxNode_ == nullptr) {
        return;
    }
    if (markSize_ > 0.0f) {
        ArkUINodeApiAdapter::SetNodeCheckboxMark(checkboxNode_, markStrokeColor_, markSize_, markStrokeWidth_);
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxMark(checkboxNode_, markStrokeColor_);
}

void ExtendedCheckboxComponent::SetShape(A2UICheckboxShape shape)
{
    shape_ = shape;
    if (checkboxNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxShape(checkboxNode_, shape_);
}

void ExtendedCheckboxComponent::SetLabel(const std::string& label)
{
    label_ = label;
    if (textNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeTextContent(textNode_, label_);
}

void ExtendedCheckboxComponent::SetGroup(const std::string& group)
{
    group_ = group;
    if (checkboxNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeCheckboxGroup(
        checkboxNode_, BuildNativeCheckboxGroupName(GetRenderId(), GetSurfaceId(), group_));

    const std::string& checkboxName = value_.empty() ? componentId_ : value_;
    ArkUINodeApiAdapter::SetNodeCheckboxName(checkboxNode_, checkboxName);
}

std::string ExtendedCheckboxComponent::ResolveRuntimeStateValue() const
{
    return componentId_;
}

bool ExtendedCheckboxComponent::TryRestoreSelectFromRuntimeState()
{
    std::string scope = GetRuntimeStateScope();
    std::string key = GetRuntimeStateKey();
    if (scope.empty() || key.empty()) {
        return false;
    }

    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(GetRenderId(), GetSurfaceId());
    if (surfaceSlot == nullptr) {
        return false;
    }

    JsonValue state;
    if (!surfaceSlot->GetRuntimeState(scope, key, state)) {
        return false;
    }
    RestoreRuntimeState(state);
    return true;
}

void ExtendedCheckboxComponent::SyncRuntimeStateToSurface()
{
    std::string scope = GetRuntimeStateScope();
    std::string key = GetRuntimeStateKey();
    if (scope.empty() || key.empty()) {
        return;
    }

    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(GetRenderId(), GetSurfaceId());
    if (surfaceSlot == nullptr) {
        return;
    }
    surfaceSlot->StoreRuntimeState(scope, key, CaptureRuntimeState());
}

std::string ExtendedCheckboxComponent::ResolveSelectBindingPath() const
{
    const auto& bindings = GetDataBindings();
    for (auto iter = bindings.rbegin(); iter != bindings.rend(); ++iter) {
        if (iter->propertyName_ == "select" && !iter->dataPath_.empty()) {
            return iter->dataPath_;
        }
    }
    return "";
}

void ExtendedCheckboxComponent::SyncSelectToBoundDataModel(bool select)
{
    std::string bindingPath = ResolveSelectBindingPath();
    if (bindingPath.empty()) {
        return;
    }

    const RenderContext& renderContext = GetRenderContext();
    std::shared_ptr<BindingEngine> bindingEngine = renderContext.bindingEngine;
    if (bindingEngine == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedCheckboxComponent::SyncSelectToBoundDataModel: binding engine is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    if (renderContext.surfaceId.empty()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedCheckboxComponent::SyncSelectToBoundDataModel: surfaceId is empty, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    std::unique_ptr<JsonAdapter> selectAdapter = JsonAdapter::CreateBool(select);
    if (selectAdapter == nullptr) {
        return;
    }
    bindingEngine->UpdateDataModelByPath(renderContext.surfaceId, bindingPath, selectAdapter->GetRoot());
}

std::shared_ptr<CheckboxTheme> ExtendedCheckboxComponent::GetTheme()
{
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }
    std::shared_ptr<ThemeBase> baseTheme = Component::GetTheme();
    if (baseTheme == nullptr) {
        return nullptr;
    }
    theme = std::dynamic_pointer_cast<CheckboxTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }
    return theme;
}

void ExtendedCheckboxComponent::OnConfigChange(const ThemeContext& context)
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
