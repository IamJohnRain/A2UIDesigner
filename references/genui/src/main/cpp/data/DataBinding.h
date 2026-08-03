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

//
// Created on 2026/3/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef A2UIRENDER_DATABINDING_H
#define A2UIRENDER_DATABINDING_H

#include <string>
#include <vector>

#include "../utils/JsonAdapter.h"

namespace NativeModule {

enum class BindingType { PATH, FUNCTION_CALL, EXPRESSION };

struct DataBinding {
    std::string propertyName_;
    std::string dataPath_;
    BindingType type_ = BindingType::PATH;
    JsonValue functionCallDescriptor_;

    std::string expression_;
    std::vector<std::string> globalVarDeps_;

    DataBinding(std::string prop, std::string path) : propertyName_(std::move(prop)), dataPath_(std::move(path)) {}

    DataBinding(std::string prop, std::string path, BindingType type, const JsonValue& descriptor)
        : propertyName_(std::move(prop)), dataPath_(std::move(path)), type_(type), functionCallDescriptor_(descriptor)
    {}

    DataBinding(std::string prop, std::string expr, std::vector<std::string> deps)
        : propertyName_(std::move(prop)), type_(BindingType::EXPRESSION), expression_(std::move(expr)),
          globalVarDeps_(std::move(deps))
    {}
};

} // namespace NativeModule

#endif // A2UIRENDER_DATABINDING_H
