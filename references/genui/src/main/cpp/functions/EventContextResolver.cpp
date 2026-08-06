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

#include "EventContextResolver.h"

#include <memory>

#include "../SchemaErrorCodes.h"
#include "../data/DynamicValueResolver.h"
#include "../utils/LogA2UI.h"
#include "WarningDispatchBridge.h"
namespace NativeModule {

namespace {

constexpr const char* FUNCTION_ITEM_TYPE = "function";
constexpr const char* UNKNOWN_FUNCTION_NAME = "unknown";

bool HasOnlyFunctionDescriptorKeys(const JsonValue& value)
{
    if (!value.IsObject()) {
        return false;
    }
    for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key != "call" && key != "args" && key != "returnType") {
            return false;
        }
    }
    return true;
}

void DispatchFunctionSchemaWarning(
    const EventResolveContext& context, const std::string& path, const std::string& code, const std::string& message)
{
    WarningDispatchBridge::GetInstance().Dispatch(context.renderId, context.surfaceId, context.componentId, code,
        message, path, FUNCTION_ITEM_TYPE, UNKNOWN_FUNCTION_NAME);
}

bool HandleMalformedFunctionDescriptor(
    const JsonValue& value, const std::string& key, const EventResolveContext& context)
{
    if (!value.IsObject() || value.Has("path")) {
        return false;
    }

    const std::string callPath = "function." + key + ".call";
    if (value.Has("call")) {
        JsonValue callValue = value.GetItem("call");
        if (!callValue.IsString()) {
            DispatchFunctionSchemaWarning(
                context, callPath, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Property call expects string value");
            return true;
        }

        if (callValue.GetStringValue("").empty()) {
            DispatchFunctionSchemaWarning(
                context, callPath, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property call is required");
            return true;
        }
        return false;
    }

    if ((value.Has("args") || value.Has("returnType")) && HasOnlyFunctionDescriptorKeys(value)) {
        DispatchFunctionSchemaWarning(context, callPath, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property call is required");
        return true;
    }

    return false;
}

bool CloneJsonValue(const JsonValue& input, JsonValue& output)
{
    if (!input.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsValid();
}

JsonValue CreateNullJsonValue()
{
    std::unique_ptr<JsonAdapter> nullAdapter = JsonAdapter::CreateNull();
    if (nullAdapter == nullptr) {
        return {};
    }
    return nullAdapter->GetRoot();
}

bool ResolveObjectContextNode(const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue);

bool ResolveContextNodeRecursively(
    const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue)
{
    if (!value.IsValid()) {
        resolvedValue = CreateNullJsonValue();
        return resolvedValue.IsValid();
    }

    if (value.IsObject()) {
        return ResolveObjectContextNode(value, context, resolvedValue);
    }

    if (value.IsArray()) {
        std::unique_ptr<JsonAdapter> arrayAdapter = JsonAdapter::CreateArray();
        if (arrayAdapter == nullptr) {
            return false;
        }
        JsonValue arrayValue = arrayAdapter->GetRoot();

        int itemCount = value.GetArraySize();
        for (int index = 0; index < itemCount; ++index) {
            JsonValue itemResolved;
            if (!ResolveContextNodeRecursively(value.GetArrayItem(index), context, itemResolved)) {
                itemResolved = CreateNullJsonValue();
            }
            if (!itemResolved.IsValid()) {
                return false;
            }
            if (!arrayValue.Append(itemResolved)) {
                return false;
            }
        }

        resolvedValue = arrayValue;
        return true;
    }

    ResolvedValue resolved = DynamicValueResolver::ResolveRecursively(value, context);
    if (!resolved.success || !resolved.value.IsValid()) {
        return false;
    }
    return CloneJsonValue(resolved.value, resolvedValue);
}

bool ResolveObjectContextNode(const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue)
{
    bool hasCall = value.Has("call");
    bool hasPath = value.Has("path");
    if (hasCall || hasPath) {
        ResolvedValue resolved = DynamicValueResolver::ResolveRecursively(value, context);
        if (!resolved.success || !resolved.value.IsValid()) {
            return false;
        }
        return CloneJsonValue(resolved.value, resolvedValue);
    }

    std::unique_ptr<JsonAdapter> objectAdapter = JsonAdapter::CreateObject();
    if (objectAdapter == nullptr) {
        return false;
    }
    JsonValue objectValue = objectAdapter->GetRoot();

    for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }

        JsonValue childResolved;
        if (!ResolveContextNodeRecursively(child, context, childResolved)) {
            continue;
        }
        if (!objectValue.Put(key.c_str(), childResolved)) {
            return false;
        }
    }

    resolvedValue = objectValue;
    return true;
}

} // namespace

JsonValue EventContextResolver::Resolve(const JsonValue& rawContextDescriptor, const EventResolveContext& context)
{
    std::unique_ptr<JsonAdapter> contextAdapter = JsonAdapter::CreateObject();
    if (contextAdapter == nullptr) {
        return {};
    }
    JsonValue contextObject = contextAdapter->GetRoot();

    if (!rawContextDescriptor.IsValid()) {
        return contextObject;
    }

    if (!rawContextDescriptor.IsObject()) {
        LOG_A2UI(LOG_WARN, "EventContextResolver: context is not an object, fallback to {}");
        return contextObject;
    }

    for (JsonValue child = rawContextDescriptor.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }

        if (HandleMalformedFunctionDescriptor(child, key, context)) {
            LOG_A2UI(LOG_WARN, "EventContextResolver: malformed function descriptor, key=%{public}s", key.c_str());
            continue;
        }

        DynamicResolveContext resolveContext = { .renderId = context.renderId,
            .surfaceId = context.surfaceId,
            .componentId = context.componentId,
            .allowExpression = true };
        JsonValue resolvedValue;
        if (!ResolveContextNodeRecursively(child, resolveContext, resolvedValue)) {
            LOG_A2UI(LOG_WARN, "EventContextResolver: resolve failed, key=%{public}s", key.c_str());
            continue;
        }

        contextObject.Put(key.c_str(), resolvedValue);
    }

    return contextObject;
}

} // namespace NativeModule
