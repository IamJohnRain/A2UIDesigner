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

#include "EvaluationContext.h"

#include "data/DataModel.h"
#include "theme/ThemeBase.h"
#include "utils/LogA2UI.h"

#include "ThemeContextUtils.h"

namespace NativeModule {

EvalResult EvaluationContext::ResolveVariable(const std::string& name)
{
    if (name == "__widthBreakpoint") {
        if (themeContext_ != nullptr) {
            return EvalResult::FromString(BreakpointToString(themeContext_->breakpoint));
        }
        return EvalResult::FromString("sm");
    }
    if (name == "__colorMode") {
        if (themeContext_ != nullptr) {
            return EvalResult::FromString(ColorModeToString(themeContext_->colorMode));
        }
        return EvalResult::FromString("light");
    }
    if (name == "__dataModel") {
        if (dataModel_ != nullptr && dataModel_->GetRoot() != nullptr) {
            if (allowContainerResults) {
                return EvalResult::FromJson(*dataModel_->GetRoot());
            }
            return EvalResult::FromString(dataModel_->GetRoot()->ToString());
        }
        return EvalResult::FromString("");
    }

    if (name.size() >= 2 && name[0] == '_' && name[1] == '_') {
        if (lastError == ExpressionError::NONE) {
            SetError(ExpressionError::EVAL_NO_GLOBAL_VARIABLE, "no global variables: " + name);
        }
        EvalResult result = EvalResult::FromString("");
        result.hasEvaluationError = true;
        return result;
    }

    auto it = globalVariables_.find(name);
    if (it != globalVariables_.end()) {
        return it->second;
    }

    for (auto scopeIt = scopeStack_.rbegin(); scopeIt != scopeStack_.rend(); ++scopeIt) {
        auto varIt = scopeIt->find(name);
        if (varIt != scopeIt->end()) {
            return varIt->second;
        }
    }

    return EvalResult::Undefined();
}

} // namespace NativeModule
