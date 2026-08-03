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

#ifndef A2UI_STYLE_ERRORS_H
#define A2UI_STYLE_ERRORS_H

#include <string>

namespace NativeModule {

enum class StyleErrorCode {
    OK = 0,
    INVALID_STYLES,
    UNSUPPORTED_PROPERTY,
    UNSUPPORTED_VALUE_TYPE,
    INVALID_COLOR,
    INVALID_SIZE,
    INVALID_EDGE,
    INVALID_RADIUS,
    INVALID_SHADOW,
    RESOLVE_FAILED,
    BINDING_FAILED
};

struct StyleError {
    StyleErrorCode code = StyleErrorCode::OK;
    std::string property;
    std::string message;
};

} // namespace NativeModule

#endif // A2UI_STYLE_ERRORS_H
