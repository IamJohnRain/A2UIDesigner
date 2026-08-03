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

#ifndef SCHEMA_ERROR_CODES_H
#define SCHEMA_ERROR_CODES_H

namespace NativeModule {

// Keep schema warning code strings in one place so protocol parsing and
// component validation report the same canonical values.
constexpr char SCHEMA_ERROR_CODE_SCHEMA_PARSE_FAILED[] = "ERROR_CODE_SCHEMA_PARSE_FAILED";
constexpr char SCHEMA_ERROR_CODE_REQUIRED_MISS[] = "ERROR_CODE_REQUIRED_MISS";
constexpr char SCHEMA_ERROR_CODE_INVALID_VALUE[] = "ERROR_CODE_INVALID_VALUE";
constexpr char SCHEMA_ERROR_CODE_TYPE_MISMATCH[] = "ERROR_CODE_TYPE_MISMATCH";
constexpr char SCHEMA_ERROR_CODE_UNDEFINED_FIELD[] = "ERROR_CODE_UNDEFINED_FIELD";

} // namespace NativeModule

#endif // SCHEMA_ERROR_CODES_H
