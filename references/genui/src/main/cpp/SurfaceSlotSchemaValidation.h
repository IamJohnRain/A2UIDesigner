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

#ifndef A2UI_SURFACE_SLOT_SCHEMA_VALIDATION_H
#define A2UI_SURFACE_SLOT_SCHEMA_VALIDATION_H

#include <string>

#include "utils/JsonAdapter.h"

namespace NativeModule {

enum class SurfaceProtocolMode;

bool ValidateDescriptorIdForPreparation(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId);

void DispatchComponentSchemaWarning(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const std::string& componentType, const std::string& code, const std::string& message,
    const std::string& propertyPath);

void ValidateRequiredCreationFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId);

void ValidateRequiredStructuralFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId,
    SurfaceProtocolMode surfaceProtocolMode);

void ValidateStructuralFieldShapes(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId,
    SurfaceProtocolMode surfaceProtocolMode);

void ValidateEventHandlerFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId);

} // namespace NativeModule

#endif // A2UI_SURFACE_SLOT_SCHEMA_VALIDATION_H
