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

#include "utils/LogA2UI.h"

#include "NativeEntry.h"
#include "napi/native_api.h"

namespace {

napi_property_descriptor NAPI_PROPERTY_DESCRIPTORS[] = {
    { "initRenderSlot", nullptr, NativeModule::InitRenderSlot, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "destroyRenderSlot", nullptr, NativeModule::DestroyRenderSlot, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "processMessage", nullptr, NativeModule::ProcessMessage, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "bindSurfaceToRender", nullptr, NativeModule::BindSurfaceToRender, nullptr, nullptr, nullptr, napi_default,
        nullptr },
    { "unbindSurfaceFromRender", nullptr, NativeModule::UnbindSurfaceFromRender, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "setRootFillMode", nullptr, NativeModule::SetRootFillMode, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "popSurface", nullptr, NativeModule::PopSurface, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getSurfaceIds", nullptr, NativeModule::GetSurfaceIds, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getLatestSurfaceId", nullptr, NativeModule::GetLatestSurfaceId, nullptr, nullptr, nullptr, napi_default,
        nullptr },
    { "registerInvokeLocalFunction", nullptr, NativeModule::RegisterInvokeLocalFunction, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "registerDispatchAction", nullptr, NativeModule::RegisterDispatchAction, nullptr, nullptr, nullptr, napi_default,
        nullptr },
    { "registerDispatchRuntimeError", nullptr, NativeModule::RegisterDispatchRuntimeError, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "registerDispatchSchemaWarning", nullptr, NativeModule::RegisterDispatchSchemaWarning, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "registerCreateCustomComponent", nullptr, NativeModule::RegisterCreateCustomComponent, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "registerUpdateCustomComponent", nullptr, NativeModule::RegisterUpdateCustomComponent, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "validateCustomComponentChecks", nullptr, NativeModule::ValidateCustomComponentChecks, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "dispatchCustomComponentAction", nullptr, NativeModule::DispatchCustomComponentAction, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "resolveCustomComponentDynamicValue", nullptr, NativeModule::ResolveCustomComponentDynamicValue, nullptr, nullptr,
        nullptr, napi_default, nullptr },
    { "clearCustomComponentDynamicValue", nullptr, NativeModule::ClearCustomComponentDynamicValue, nullptr, nullptr,
        nullptr, napi_default, nullptr },
    { "evaluateDynamicValue", nullptr, NativeModule::EvaluateDynamicValue, nullptr, nullptr, nullptr, napi_default,
        nullptr },
    { "setFontSizeScale", nullptr, NativeModule::SetFontSizeScale, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "setApiVersion", nullptr, NativeModule::SetApiVersion, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "registerCrossLanguageAttributeCallback", nullptr, NativeModule::RegisterCrossLanguageAttributeCallback, nullptr,
        nullptr, nullptr, napi_default, nullptr },
    { "syncComponentBoundDataModel", nullptr, NativeModule::SyncComponentBoundDataModel, nullptr, nullptr, nullptr,
        napi_default, nullptr },
    { "registerLocale", nullptr, NativeModule::RegisterLocale, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "updateThemeMode", nullptr, NativeModule::UpdateThemeMode, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "updateBreakpoint", nullptr, NativeModule::UpdateBreakpoint, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "setDisplayDensity", nullptr, NativeModule::SetDisplayDensity, nullptr, nullptr, nullptr, napi_default, nullptr },
#ifdef ENABLE_EXPRESSION_ENGINE
    { "evaluateExpression", nullptr, NativeModule::EvaluateExpression, nullptr, nullptr, nullptr, napi_default,
        nullptr }
#endif
};

} // namespace

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    LOG_A2UI(LOG_INFO, "NAPI Init - Registering functions");
    napi_define_properties(env, exports, sizeof(NAPI_PROPERTY_DESCRIPTORS) / sizeof(NAPI_PROPERTY_DESCRIPTORS[0]),
        NAPI_PROPERTY_DESCRIPTORS);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "a2ui_native",
    .nm_priv = nullptr,
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
