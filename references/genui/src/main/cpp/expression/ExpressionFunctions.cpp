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

#include "ExpressionFunctions.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

void ExpressionFunctions::Register(const std::string& name, ExpressionFunc func)
{
    functions_[name] = std::move(func);
}

void ExpressionFunctions::Register(const std::string& name, LegacyExpressionFunc func)
{
    Register(name, [func = std::move(func)](const std::vector<EvalResult>& args, EvaluationContext&) -> EvalResult {
        return func(args);
    });
}

bool ExpressionFunctions::Has(const std::string& name) const
{
    return functions_.find(name) != functions_.end();
}

EvalResult ExpressionFunctions::Call(const std::string& name, const std::vector<EvalResult>& args)
{
    EvaluationContext context;
    return Call(name, args, context);
}

EvalResult ExpressionFunctions::Call(
    const std::string& name, const std::vector<EvalResult>& args, EvaluationContext& context)
{
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        LOG_A2UI(LOG_WARN, "ExpressionFunctions: unknown function '%{public}s'", name.c_str());
        return EvalResult::Undefined();
    }
    return it->second(args, context);
}

} // namespace NativeModule
