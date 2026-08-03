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

#include "components/actions/BuiltInActions.h"

#include <memory>

#include "components/actions/NativeActionRegistry.h"
#include "data/DataModel.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/EventContextResolver.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "RenderManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

JsonValue CloneJsonValue(const JsonValue& value)
{
    if (!value.IsValid()) {
        return JsonValue();
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(value);
    if (adapter == nullptr) {
        return JsonValue();
    }
    return adapter->GetRoot();
}

JsonValue CreateEmptyObjectValue()
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

bool JsonObjectHasEntries(const JsonValue& value)
{
    if (!value.IsObject()) {
        return false;
    }

    for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
        if (!child.GetKey().empty()) {
            return true;
        }
    }
    return false;
}

JsonValue MergeEventContext(const JsonValue& baseContext, const JsonValue& extraContext)
{
    if (!extraContext.IsValid()) {
        if (baseContext.IsObject()) {
            return CloneJsonValue(baseContext);
        }
        return CreateEmptyObjectValue();
    }

    if (!extraContext.IsObject()) {
        if (!baseContext.IsObject() || !JsonObjectHasEntries(baseContext)) {
            return CloneJsonValue(extraContext);
        }

        std::unique_ptr<JsonAdapter> contextAdapter = JsonAdapter::Clone(baseContext);
        if (contextAdapter == nullptr) {
            return JsonValue();
        }

        JsonValue contextRoot = contextAdapter->GetRoot();
        if (contextRoot.Has("value")) {
            contextRoot.Replace("value", extraContext);
        } else {
            contextRoot.Put("value", extraContext);
        }
        return contextRoot;
    }

    std::unique_ptr<JsonAdapter> contextAdapter =
        baseContext.IsObject() ? JsonAdapter::Clone(baseContext) : JsonAdapter::CreateObject();
    if (contextAdapter == nullptr) {
        return JsonValue();
    }

    JsonValue contextRoot = contextAdapter->GetRoot();
    for (JsonValue child = extraContext.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }
        if (contextRoot.Has(key.c_str())) {
            contextRoot.Replace(key.c_str(), child);
        } else {
            contextRoot.Put(key.c_str(), child);
        }
    }

    return contextRoot;
}

} // namespace

void RegisterBuiltInActions(NativeActionRegistry& registry)
{
    registry.Register("dispatchEvent", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        std::string eventName = args.GetString("eventName", "");
        if (eventName.empty()) {
            LOG_A2UI(LOG_WARN, "NativeActionRegistry::dispatchEvent - eventName is empty, componentId=%{public}s",
                ctx.componentId.c_str());
            return JsonValue();
        }

        EventResolveContext resolveContext = {
            .renderId = ctx.renderId, .surfaceId = ctx.surfaceId, .componentId = ctx.componentId
        };
        JsonValue resolvedContext = EventContextResolver::Resolve(args.GetItem("context"), resolveContext);
        const JsonValue& externalContext = ctx.hasExternalEventContext ? ctx.externalEventContext : ctx.eventContext;
        JsonValue dispatchContext = MergeEventContext(resolvedContext, externalContext);
        ActionDispatchBridge::GetInstance().Dispatch(
            ctx.renderId, ctx.surfaceId, ctx.componentId, eventName, dispatchContext);
        return JsonValue();
    });

    registry.Register("setDataModel", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        if (ctx.dataModel == nullptr) {
            ctx.dataModel = std::make_shared<DataModel>(ctx.surfaceId);
        }

        JsonValue pathValue = args.GetItem("path");
        std::string resolvedPath;
        if (pathValue.IsString()) {
            resolvedPath = pathValue.GetStringValue();
        }

        if (resolvedPath.empty()) {
            LOG_A2UI(LOG_WARN, "NativeActionRegistry::setDataModel - path is empty, componentId=%{public}s",
                ctx.componentId.c_str());
            return JsonValue();
        }

        JsonValue value = args.GetItem("value");
        ctx.dataModel->UpdateByPath(resolvedPath, value);
        ctx.dataModel->NotifyPathUpdate(resolvedPath);

        return JsonValue();
    });

    registry.Register("setAttributes", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        std::string componentId = args.GetString("componentId", "");
        if (componentId.empty()) {
            LOG_A2UI(LOG_WARN,
                "NativeActionRegistry::setAttributes - componentId is empty, "
                "sourceComponentId=%{public}s",
                ctx.componentId.c_str());
            return JsonValue();
        }

        JsonValue value = args.GetItem("value");
        if (!value.IsObject()) {
            LOG_A2UI(LOG_WARN,
                "NativeActionRegistry::setAttributes - value is not object, "
                "componentId=%{public}s",
                componentId.c_str());
            return JsonValue();
        }

        SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(ctx.renderId, ctx.surfaceId);
        if (surface == nullptr) {
            LOG_A2UI(LOG_WARN,
                "NativeActionRegistry::setAttributes - surface not found, "
                "surfaceId=%{public}s, componentId=%{public}s",
                ctx.surfaceId.c_str(), componentId.c_str());
            return JsonValue();
        }

        std::shared_ptr<Component> component = surface->FindComponentById(componentId);
        if (component == nullptr) {
            LOG_A2UI(LOG_WARN,
                "NativeActionRegistry::setAttributes - component not found, "
                "componentId=%{public}s",
                componentId.c_str());
            return JsonValue();
        }

        component->ApplyDescriptor(value);
        return JsonValue();
    });

    registry.Register("navigate", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        LOG_A2UI(LOG_WARN, "NativeActionRegistry::navigate - STUB: not implemented, componentId=%{public}s",
            ctx.componentId.c_str());
        return JsonValue();
    });
}

} // namespace NativeModule
