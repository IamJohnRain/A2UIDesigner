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

#ifndef SURFACE_ERROR_CODES_H
#define SURFACE_ERROR_CODES_H

#include <cstdint>

namespace NativeModule {

// Keep cross-layer error codes in sync with
// genui/src/main/ets/interface/SurfaceTypes.ets.
constexpr int32_t SURFACE_RESULT_OK = 0;
constexpr int32_t SURFACE_RESULT_UNSUPPORTED_PROTOCOL_VERSION = 1003;

// 2002-2009: schema parse error codes
constexpr int32_t SURFACE_RESULT_SCHEMA_DSL_EMPTY = 2002;
constexpr int32_t SURFACE_RESULT_SCHEMA_JSON_PARSE_FAILED = 2003;
constexpr int32_t SURFACE_RESULT_SCHEMA_ROOT_NOT_OBJECT = 2004;

// 2101-2199: schema validation error codes
constexpr int32_t SURFACE_RESULT_SCHEMA_MESSAGE_OPERATION_INVALID = 2101;
constexpr int32_t SURFACE_RESULT_SCHEMA_MESSAGE_MULTIPLE_BODIES = 2102;
constexpr int32_t SURFACE_RESULT_SCHEMA_MESSAGE_BODY_INVALID = 2103;
constexpr int32_t SURFACE_RESULT_SCHEMA_SURFACE_ID_MISSING = 2104;
constexpr int32_t SURFACE_RESULT_SCHEMA_COMPONENTS_INVALID = 2105;
constexpr int32_t SURFACE_RESULT_SCHEMA_CATALOG_ID_MISSING = 2106;
constexpr int32_t SURFACE_RESULT_SCHEMA_VERSION_INVALID = 2107;

// 11001-11999: surface operation result codes
constexpr int32_t SURFACE_RESULT_MULTI_SURFACE_DISABLED = 11001;
constexpr int32_t SURFACE_RESULT_ONLY_ONE_SURFACE = 11002;
constexpr int32_t SURFACE_RESULT_EMPTY_STACK = 11003;
constexpr int32_t SURFACE_RESULT_MAX_SURFACE_LIMIT_REACHED = 11004;
constexpr int32_t SURFACE_RESULT_SURFACE_ALREADY_EXISTS = 11005;
constexpr int32_t SURFACE_RESULT_GESTURE_CONFLICT = 12001;
constexpr int32_t SURFACE_RESULT_ENGINE_ERROR = 13001;

// Runtime callback error codes are kept here as the single native source of
// truth for SurfaceErrorCode values.
constexpr int32_t SURFACE_ERROR_NO_SURFACE_MATCHED = 1001;
constexpr int32_t SURFACE_ERROR_NATIVE_PROCESS_FAILED = 1002;
constexpr int32_t SURFACE_ERROR_UNSUPPORTED_PROTOCOL_VERSION = 1003;
constexpr int32_t SURFACE_ERROR_COMPONENT_DROPPED_ON_INVALID_PARAMETER = 1004;
constexpr int32_t SURFACE_ERROR_FALLBACK_WARNING = 1101;
constexpr int32_t SURFACE_ERROR_SCHEMA_WARNING = 2001;
constexpr int32_t SURFACE_ERROR_ACTION_NOT_REGISTER = 3001;
constexpr int32_t SURFACE_ERROR_LOCAL_FUNCTION = 3101;
constexpr int32_t SURFACE_ERROR_ACTION_PARSE_FAILED = 3201;
constexpr int32_t SURFACE_ERROR_DYNAMIC_VALUE_RESOLVE_FAILED = 3202;
constexpr int32_t SURFACE_ERROR_GLOBAL_VARIABLE_NOT_FOUND = 3203;
constexpr int32_t SURFACE_ERROR_ILLEGAL_EXPRESSION = 3204;

} // namespace NativeModule

#endif // SURFACE_ERROR_CODES_H
