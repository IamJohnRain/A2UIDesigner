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

#ifndef HWCLAW_NATIVE_ENTRY_H
#define HWCLAW_NATIVE_ENTRY_H

#include <arkui/native_node_napi.h>

#include <js_native_api.h>

namespace NativeModule {

napi_value InitRenderSlot(napi_env env, napi_callback_info info);
napi_value DestroyRenderSlot(napi_env env, napi_callback_info info);
napi_value ProcessMessage(napi_env env, napi_callback_info info);
napi_value BindSurfaceToRender(napi_env env, napi_callback_info info);
napi_value UnbindSurfaceFromRender(napi_env env, napi_callback_info info);
napi_value SetRootFillMode(napi_env env, napi_callback_info info);
napi_value PopSurface(napi_env env, napi_callback_info info);
napi_value GetSurfaceIds(napi_env env, napi_callback_info info);
napi_value GetLatestSurfaceId(napi_env env, napi_callback_info info);
napi_value RegisterInvokeLocalFunction(napi_env env, napi_callback_info info);
napi_value RegisterDispatchAction(napi_env env, napi_callback_info info);
napi_value RegisterDispatchRuntimeError(napi_env env, napi_callback_info info);
napi_value RegisterDispatchSchemaWarning(napi_env env, napi_callback_info info);
napi_value RegisterCreateCustomComponent(napi_env env, napi_callback_info info);
napi_value RegisterUpdateCustomComponent(napi_env env, napi_callback_info info);
napi_value ValidateCustomComponentChecks(napi_env env, napi_callback_info info);
napi_value DispatchCustomComponentAction(napi_env env, napi_callback_info info);
napi_value ResolveCustomComponentDynamicValue(napi_env env, napi_callback_info info);
napi_value ClearCustomComponentDynamicValue(napi_env env, napi_callback_info info);
napi_value EvaluateDynamicValue(napi_env env, napi_callback_info info);
napi_value SetFontSizeScale(napi_env env, napi_callback_info info);
napi_value SetApiVersion(napi_env env, napi_callback_info info);
napi_value RegisterCrossLanguageAttributeCallback(napi_env env, napi_callback_info info);
napi_value SyncComponentBoundDataModel(napi_env env, napi_callback_info info);
napi_value RegisterLocale(napi_env env, napi_callback_info info);
napi_value UpdateThemeMode(napi_env env, napi_callback_info info);
napi_value UpdateBreakpoint(napi_env env, napi_callback_info info);
napi_value SetDisplayDensity(napi_env env, napi_callback_info info);
#ifdef ENABLE_EXPRESSION_ENGINE
napi_value EvaluateExpression(napi_env env, napi_callback_info info);
#endif

} // namespace NativeModule

#endif
