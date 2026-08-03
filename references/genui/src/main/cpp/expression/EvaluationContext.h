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

#ifndef A2UI_EVALUATION_CONTEXT_H
#define A2UI_EVALUATION_CONTEXT_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "EvalResult.h"
#include "ExpressionErrors.h"

namespace NativeModule {

class DataModel;
struct ThemeContext;

class EvaluationContext {
public:
    void SetRenderId(int32_t renderId)
    {
        renderId_ = renderId;
    }

    void SetSurfaceId(const std::string& surfaceId)
    {
        surfaceId_ = surfaceId;
    }

    void SetComponentId(const std::string& componentId)
    {
        componentId_ = componentId;
    }

    void SetDataModel(DataModel* dataModel)
    {
        dataModel_ = dataModel;
    }

    void SetThemeContext(const ThemeContext* themeContext)
    {
        themeContext_ = themeContext;
    }

    const ThemeContext* GetThemeContext() const
    {
        return themeContext_;
    }

    void SetGlobalVariable(const std::string& name, const EvalResult& value)
    {
        globalVariables_[name] = value;
    }

    void SetLocalVariable(const std::string& name, const EvalResult& value)
    {
        if (name.size() >= 2 && name[0] == '_' && name[1] == '_') {
            return;
        }
        if (!scopeStack_.empty()) {
            scopeStack_.back()[name] = value;
        }
    }

    EvalResult ResolveVariable(const std::string& name);

    void PushScope()
    {
        scopeStack_.emplace_back();
    }

    void PopScope()
    {
        if (!scopeStack_.empty()) {
            scopeStack_.pop_back();
        }
    }

    DataModel* GetDataModel() const
    {
        return dataModel_;
    }

    int32_t GetRenderId() const
    {
        return renderId_;
    }

    const std::string& GetSurfaceId() const
    {
        return surfaceId_;
    }

    const std::string& GetComponentId() const
    {
        return componentId_;
    }

    ExpressionError lastError = ExpressionError::NONE;
    std::string errorMessage;
    size_t errorPosition = 0;

    void SetError(ExpressionError code, const std::string& message, size_t position = 0)
    {
        lastError = code;
        errorMessage = message;
        errorPosition = position;
    }

    void ClearError()
    {
        lastError = ExpressionError::NONE;
        errorMessage.clear();
        errorPosition = 0;
    }

    size_t maxExprLength = 2048;
    size_t maxTokenCount = 100;
    size_t maxNestingDepth = 20;
    size_t maxAstNodes = 100;
    bool allowContainerResults = false;

private:
    int32_t renderId_ = -1;
    std::string surfaceId_;
    std::string componentId_;
    DataModel* dataModel_ = nullptr;
    const ThemeContext* themeContext_ = nullptr;
    std::vector<std::map<std::string, EvalResult>> scopeStack_;
    std::map<std::string, EvalResult> globalVariables_;
};

} // namespace NativeModule

#endif // A2UI_EVALUATION_CONTEXT_H
