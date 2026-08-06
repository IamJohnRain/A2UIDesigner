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

#ifndef A2UI_IF_COMPONENT_H
#define A2UI_IF_COMPONENT_H

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/extended/ExtendedComponent.h"

#include "expression/DependencyCollector.h"

namespace NativeModule {

class SurfaceSlot;
class EvaluationContext;

class IfComponent : public ExtendedComponent {
public:
    IfComponent();
    ~IfComponent() override;

    std::string GetType() const override;

    void OnConfigChange(const ThemeContext& context) override;
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;
    void RemoveAllChildren() override;

    bool ReevaluateAndSwitch();
    void BuildBranchChildren(SurfaceSlot& surfaceSlot);

protected:
    bool CreateArkUINode() override;
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    void ApplyComponentSpecificAttributes(const JsonValue& normalizedDescriptor) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;
    void RegisterClickHandler() override;
    void RegisterComponentSpecificListeners() override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
    void SelectBranch(bool conditionResult);
    void ReconcileBranchChildren(std::map<std::string, std::shared_ptr<Component>>& allComponents);

private:
    std::string conditionExpression_;
    bool currentBranch_ = true;
    std::vector<Dependency> dependencies_;
    std::list<std::string> childrenIfIds_;
    std::list<std::string> childrenElseIds_;
    bool initialized_ = false;
    ThemeContext lastThemeContext_;
    bool themeContextValid_ = false;

    std::list<std::string> ParseStringArray(const JsonValue& value, const std::string& propertyName);
    bool EvaluateCondition(const std::string& expression, bool& evalSucceeded, bool isInitialEvaluation);
    void SyncConditionExpressionBinding(const std::string& expression);
    void InjectGlobalVariables(EvaluationContext& context);
#ifdef ENABLE_EXPRESSION_ENGINE
    void SetupConditionEvaluationContext(EvaluationContext& context);
    bool EvaluateConditionWithExpressionEngine(
        const std::string& expression, bool& evalSucceeded, bool isInitialEvaluation);
#endif
    SurfaceSlot* GetRuntimeSurfaceSlot() const;
    Component* FindAncestorWithNativeView();
    void ReportMissingBranchChildren(const std::map<std::string, std::shared_ptr<Component>>& allComponents);
    void ReportIfSchemaWarning(
        const std::string& code, const std::string& message, const std::string& propertyPath) const;
};

} // namespace NativeModule

#endif // A2UI_IF_COMPONENT_H
