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

#ifndef A2UI_EXPRESSION_FUNCTIONS_H
#define A2UI_EXPRESSION_FUNCTIONS_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "EvalResult.h"
#include "EvaluationContext.h"

namespace NativeModule {

using ExpressionFunc = std::function<EvalResult(const std::vector<EvalResult>&, EvaluationContext&)>;
using LegacyExpressionFunc = std::function<EvalResult(const std::vector<EvalResult>&)>;

class ExpressionFunctions {
public:
    void Register(const std::string& name, ExpressionFunc func);
    void Register(const std::string& name, LegacyExpressionFunc func);
    bool Has(const std::string& name) const;
    EvalResult Call(const std::string& name, const std::vector<EvalResult>& args);
    EvalResult Call(const std::string& name, const std::vector<EvalResult>& args, EvaluationContext& context);

private:
    std::unordered_map<std::string, ExpressionFunc> functions_;
};

} // namespace NativeModule

#endif // A2UI_EXPRESSION_FUNCTIONS_H
