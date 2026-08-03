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

#ifndef A2UIRENDER_NATIVECOMPONENTFACTORY_H
#define A2UIRENDER_NATIVECOMPONENTFACTORY_H

#include <memory>
#include <string>
#include <unordered_map>

#include "utils/JsonAdapter.h"

#include "Component.h"

namespace NativeModule {

class NativeComponentFactory final {
public:
    static std::shared_ptr<Component> CreateComponent(const std::string& type);
    static std::string ResolveComponentType(const JsonValue& descriptor);

private:
    NativeComponentFactory() = default;
    ~NativeComponentFactory() = default;
    NativeComponentFactory(const NativeComponentFactory&) = delete;
    NativeComponentFactory& operator=(const NativeComponentFactory&) = delete;

    static std::unordered_map<std::string, std::function<std::shared_ptr<Component>()>> builders_;
};

} // namespace NativeModule
#endif // A2UIRENDER_NATIVECOMPONENTFACTORY_H
