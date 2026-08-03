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

#ifndef A2UI_SURFACE_CONTEXT_H
#define A2UI_SURFACE_CONTEXT_H

#include <string>

namespace NativeModule {

// Keep in sync with genui/src/main/ets/core/base/CapabilitiesCore.ets.
constexpr const char* DEFAULT_A2UI_PROTOCOL_VERSION = "v0.9";

struct SurfaceContext {
    std::string a2UIProtocolVersion = DEFAULT_A2UI_PROTOCOL_VERSION;
    std::string catalogId;
};

} // namespace NativeModule

#endif // A2UI_SURFACE_CONTEXT_H
