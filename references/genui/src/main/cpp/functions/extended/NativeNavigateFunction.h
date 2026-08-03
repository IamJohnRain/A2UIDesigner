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

#ifndef A2UI_EXTENDED_NATIVE_NAVIGATE_FUNCTION_H
#define A2UI_EXTENDED_NATIVE_NAVIGATE_FUNCTION_H

#include "functions/NativeFunctionBase.h"

namespace NativeModule {

class NativeNavigateFunction : public NativeFunctionBase {
public:
    std::string GetName() const override;
    FunctionResult Execute(const JsonValue& resolvedArgs) override;
    FunctionResult ExecuteWithContext(const JsonValue& resolvedArgs, const DynamicResolveContext& context) override;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_NATIVE_NAVIGATE_FUNCTION_H
