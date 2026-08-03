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

#ifndef A2UI_NATIVE_FUNCTION_REGISTRY_H
#define A2UI_NATIVE_FUNCTION_REGISTRY_H

#include <memory>
#include <string>
#include <unordered_map>

#include "data/DynamicValueResolver.h"

#include "../data/ResolvedValue.h"
#include "NativeFunctionBase.h"

namespace NativeModule {

class NativeFunctionRegistry {
public:
    static NativeFunctionRegistry& GetInstance();

    void Register(const std::string& name, std::shared_ptr<NativeFunctionBase> handler);
    bool HasFunction(const std::string& name) const;
    ResolvedValue Execute(const std::string& name, const JsonValue& resolvedArgs, const DynamicResolveContext& context,
        const std::string& expectedReturnType = "");

private:
    NativeFunctionRegistry();
    ~NativeFunctionRegistry() = default;
    NativeFunctionRegistry(const NativeFunctionRegistry&) = delete;
    NativeFunctionRegistry& operator=(const NativeFunctionRegistry&) = delete;

    std::unordered_map<std::string, std::shared_ptr<NativeFunctionBase>> handlers_;
};

} // namespace NativeModule

#endif // A2UI_NATIVE_FUNCTION_REGISTRY_H
