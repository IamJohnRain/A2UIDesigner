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

#include "SurfaceSlotSchemaValidation.h"

#include <vector>

#include "components/ChildListSchemaValidationUtils.h"
#include "functions/WarningDispatchBridge.h"
#include "utils/RequiredStringPropertyUtils.h"

#include "SchemaErrorCodes.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

constexpr char SCHEMA_ITEM_TYPE_COMPONENT[] = "component";

const std::vector<std::string> EVENT_HANDLER_PROPERTY_NAMES = { "onClick", "onAppear", "onChange", "onSelect",
    "onReachStart", "onReachEnd" };

std::string BuildComponentSchemaWarningPath(const std::string& componentId, const std::string& propertyPath)
{
    if (componentId.empty()) {
        return propertyPath;
    }
    if (propertyPath.empty()) {
        return componentId;
    }
    return componentId + "." + propertyPath;
}

std::string ResolveSchemaWarningItemName(const std::string& componentType)
{
    return componentType.empty() ? "unknown" : componentType;
}

std::vector<std::string> GetRequiredStructuralPropertyKeysForComponentType(
    const std::string& componentType, SurfaceProtocolMode surfaceProtocolMode)
{
    if (surfaceProtocolMode == SurfaceProtocolMode::EXTENDED_PROTOCOL) {
        return {};
    }
    if (componentType == "Button") {
        return { "child" };
    }
    if (componentType == "Card") {
        return { "child" };
    }
    if (componentType == "Column" || componentType == "Row" || componentType == "List") {
        return { "children" };
    }
    return {};
}

bool HasNonEmptyStringProperty(const JsonValue& nodeValue, const char* propertyName)
{
    if (!nodeValue.IsObject() || propertyName == nullptr || !nodeValue.Has(propertyName)) {
        return false;
    }

    JsonValue propertyValue = nodeValue.GetItem(propertyName);
    return propertyValue.IsString() && !propertyValue.GetStringValue("").empty();
}

} // namespace

bool ValidateDescriptorIdForPreparation(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId)
{
    if (!nodeValue.IsObject()) {
        return false;
    }

    const std::string componentType = nodeValue.GetString("component", "");
    const RequiredStringPropertyState idState = GetRequiredStringPropertyState(nodeValue, "id");
    if (idState == RequiredStringPropertyState::VALID) {
        return true;
    }

    if (idState == RequiredStringPropertyState::TYPE_MISMATCH) {
        DispatchComponentSchemaWarning(renderId, surfaceId, "", componentType, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property id expects string value, drop current item", "id");
    } else {
        DispatchComponentSchemaWarning(renderId, surfaceId, "", componentType, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property id is required, drop current item", "id");
    }
    return false;
}

void DispatchComponentSchemaWarning(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const std::string& componentType, const std::string& code, const std::string& message,
    const std::string& propertyPath)
{
    if (renderId < 0) {
        return;
    }
    WarningDispatchBridge::GetInstance().Dispatch(renderId, surfaceId, componentId, code, message,
        BuildComponentSchemaWarningPath(componentId, propertyPath), SCHEMA_ITEM_TYPE_COMPONENT,
        ResolveSchemaWarningItemName(componentType));
}

void ValidateRequiredCreationFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId)
{
    if (!nodeValue.IsObject()) {
        return;
    }

    const std::string nodeId = nodeValue.GetString("id", "");
    const std::string componentType = nodeValue.GetString("component", "");
    const RequiredStringPropertyState idState = GetRequiredStringPropertyState(nodeValue, "id");
    if (idState == RequiredStringPropertyState::TYPE_MISMATCH) {
        DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property id expects string value, drop current item", "id");
    } else if (idState != RequiredStringPropertyState::VALID) {
        DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property id is required, drop current item", "id");
    }

    const RequiredStringPropertyState componentState = GetRequiredStringPropertyState(nodeValue, "component");
    if (componentState == RequiredStringPropertyState::TYPE_MISMATCH) {
        DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property component expects string value, drop current item", "component");
    } else if (componentState != RequiredStringPropertyState::VALID) {
        DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property component is required, drop current item", "component");
    }
}

void ValidateRequiredStructuralFields(
    const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId, SurfaceProtocolMode surfaceProtocolMode)
{
    if (!nodeValue.IsObject()) {
        return;
    }

    const std::string nodeId = nodeValue.GetString("id", "");
    const std::string componentType = nodeValue.GetString("component", "");
    const std::vector<std::string> requiredKeys =
        GetRequiredStructuralPropertyKeysForComponentType(componentType, surfaceProtocolMode);
    for (const std::string& key : requiredKeys) {
        bool present = false;
        if (key == "child") {
            present = HasNonEmptyStringProperty(nodeValue, key.c_str());
        } else {
            present = nodeValue.Has(key.c_str());
        }
        if (present) {
            continue;
        }

        DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property " + key + " is required", key);
    }
}

void ValidateStructuralFieldShapes(
    const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId, SurfaceProtocolMode surfaceProtocolMode)
{
    if (!nodeValue.IsObject()) {
        return;
    }
    if (surfaceProtocolMode == SurfaceProtocolMode::EXTENDED_PROTOCOL) {
        return;
    }

    const std::string nodeId = nodeValue.GetString("id", "");
    const std::string componentType = nodeValue.GetString("component", "");
    if (componentType != "Column" && componentType != "Row" && componentType != "List") {
        return;
    }

    for (const auto& issue :
        ValidateChildListSchema(nodeValue, "children", ChildListEmptyArrayPolicy::WARN_INVALID_VALUE)) {
        DispatchComponentSchemaWarning(
            renderId, surfaceId, nodeId, componentType, issue.code, issue.message, issue.propertyPath);
    }
}

void ValidateEventHandlerFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId)
{
    if (!nodeValue.IsObject()) {
        return;
    }

    const std::string nodeId = nodeValue.GetString("id", "");
    const std::string componentType = nodeValue.GetString("component", "");

    for (const auto& eventName : EVENT_HANDLER_PROPERTY_NAMES) {
        if (!nodeValue.Has(eventName.c_str())) {
            continue;
        }

        JsonValue eventValue = nodeValue.GetItem(eventName.c_str());
        if (!eventValue.IsArray()) {
            DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + eventName + " expects array, got type '" + std::string(eventValue.GetTypeName()) + "'",
                eventName);
            continue;
        }

        const int handlerCount = eventValue.GetArraySize();
        for (int i = 0; i < handlerCount; ++i) {
            JsonValue handler = eventValue.GetArrayItem(i);
            const std::string handlerPath = eventName + "[" + std::to_string(i) + "]";

            if (!handler.IsObject()) {
                DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType,
                    SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                    "Property " + handlerPath + " expects object, got type '" + std::string(handler.GetTypeName()) +
                        "'",
                    handlerPath);
                continue;
            }

            JsonValue callValue = handler.GetItem("call");
            if (!callValue.IsString()) {
                if (handler.Has("condition")) {
                    DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType,
                        SCHEMA_ERROR_CODE_REQUIRED_MISS,
                        "Property " + handlerPath + " has 'condition' but missing valid 'call'", handlerPath + ".call");
                } else if (handler.Has("call")) {
                    DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType,
                        SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Property " + handlerPath + ".call expects string value",
                        handlerPath + ".call");
                } else {
                    DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType,
                        SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property " + handlerPath + ".call is required",
                        handlerPath + ".call");
                }
                continue;
            }

            if (callValue.GetStringValue("").empty()) {
                DispatchComponentSchemaWarning(renderId, surfaceId, nodeId, componentType,
                    SCHEMA_ERROR_CODE_INVALID_VALUE, "Property " + handlerPath + ".call cannot be empty string",
                    handlerPath + ".call");
            }
        }
    }
}

} // namespace NativeModule
