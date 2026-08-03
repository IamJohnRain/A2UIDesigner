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

#include <gtest/gtest.h>
#include <memory>
#include <set>
#include <string>

#include "functions/RuntimeErrorDispatchBridge.h"

#include "TestFixture.h"

// Expose private/protected members of CustomComponent (and the Component base it pulls in)
// so the expression helpers can be driven directly for branch coverage.
#define private public
#define protected public
#include "components/custom/CustomComponent.h"
#undef protected
#undef private

#include "components/Component.h"
#include "components/custom/CustomComponentExpressionBinding.h"
#include "utils/JsonAdapter.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

JsonValue MakeString(const std::string& value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateString(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

JsonValue MakeNumber(double value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNumber(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

JsonValue MakeBool(bool value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateBool(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

std::set<std::string> CollectBindingPaths(const CustomComponent& component, const std::string& propertyName)
{
    std::set<std::string> paths;
    for (const auto& binding : component.GetDataBindings()) {
        if (binding.propertyName_ == propertyName) {
            paths.insert(binding.dataPath_);
        }
    }
    return paths;
}

// Drives the (otherwise protected/private) expression helpers of CustomComponent directly.
class CustomComponentExpressionProbe : public CustomComponent {
public:
    explicit CustomComponentExpressionProbe(const std::string& type) : CustomComponent(type) {}

    using CustomComponent::BuildCustomProps;
    using CustomComponent::EvaluateCustomExpression;
    using CustomComponent::IsExtendedEtsExpressionScope;
    using CustomComponent::ResolveExpressionsInValue;
};

class CustomComponentExpressionTddTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x2100);

    void SetUp() override
    {
        A2UITest::SetUp();
        napi_value callback = nullptr;
        mockNapiPtr_->CreateFunction(env_, "runtimeErrorCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, callback);
        // Count only dispatches that happen during each test case.
        mockNapiPtr_->callFunctionCallCount_ = 0;
    }

    // A probe with a non-negative renderId so a dispatched runtime error can reach the bridge.
    // CustomComponent is non-movable, so hand back a heap-owned probe.
    std::unique_ptr<CustomComponentExpressionProbe> MakeDispatchProbe(const std::string& type = "Tabs")
    {
        auto probe = std::make_unique<CustomComponentExpressionProbe>(type);
        probe->SetRenderId(7);
        probe->SetSurfaceId("expr-surface");
        probe->SetComponentId("expr-component");
        return probe;
    }

    size_t DispatchCount() const
    {
        return mockNapiPtr_->callFunctionCallCount_;
    }
};

} // namespace

// ============================ EvaluateCustomExpression ============================

/**
 * @tc.name: EvaluateCustomExpression_returns_invalid_for_non_expression_input
 * @tc.desc: Non-expression strings short-circuit before evaluation (IsExpression == false branch),
 *           returning an invalid value and never dispatching a runtime error.
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, EvaluateCustomExpression_returns_invalid_for_non_expression_input)
{
    auto probe = MakeDispatchProbe();

    JsonValue result = probe->EvaluateCustomExpression("plain text");
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(DispatchCount(), 0U);
}

/**
 * @tc.name: EvaluateCustomExpression_returns_resolved_value_without_dispatch_on_success
 * @tc.desc: A valid expression with no evaluation error returns the type-preserved value and
 *           skips the runtime-error dispatch (HasExpressionContextError == false branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, EvaluateCustomExpression_returns_resolved_value_without_dispatch_on_success)
{
    auto probe = MakeDispatchProbe();

    JsonValue result = probe->EvaluateCustomExpression("{{ 1 + 2 }}");
    ASSERT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.GetNumberValue(0.0), 3.0);
    EXPECT_EQ(DispatchCount(), 0U);
}

/**
 * @tc.name: EvaluateCustomExpression_dispatches_for_soft_error
 * @tc.desc: A soft error (PARSE_UNEXPECTED_TOKEN from an empty "{{}}") with an invalid value
 *           triggers the runtime-error dispatch (IsSoftExpressionContextError == true branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, EvaluateCustomExpression_dispatches_for_soft_error)
{
    auto probe = MakeDispatchProbe();

    JsonValue result = probe->EvaluateCustomExpression("{{}}");
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(DispatchCount(), 1U);
}

/**
 * @tc.name: EvaluateCustomExpression_dispatches_for_non_soft_error_with_invalid_value
 * @tc.desc: A non-soft error (EVAL_DIVISION_BY_ZERO) producing an invalid value still dispatches
 *           (IsSoftExpressionContextError == false, !value.IsValid() == true branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, EvaluateCustomExpression_dispatches_for_non_soft_error_with_invalid_value)
{
    auto probe = MakeDispatchProbe();

    JsonValue result = probe->EvaluateCustomExpression("{{ 1 / 0 }}");
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(DispatchCount(), 1U);
}

/**
 * @tc.name: EvaluateCustomExpression_skips_dispatch_for_non_soft_error_with_valid_value
 * @tc.desc: A non-soft error that still yields a valid value (size() fallback over a bad inner
 *           expression) must NOT dispatch
 *           (IsSoftExpressionContextError == false, !value.IsValid() == false branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, EvaluateCustomExpression_skips_dispatch_for_non_soft_error_with_valid_value)
{
    auto probe = MakeDispatchProbe();

    JsonValue result = probe->EvaluateCustomExpression("{{ size(\"${/missing}\") }}");
    ASSERT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsNumber());
    EXPECT_EQ(DispatchCount(), 0U);
}

// ============================ IsExtendedEtsExpressionScope ============================

/**
 * @tc.name: IsExtendedEtsExpressionScope_is_true_only_for_scoped_extended_types
 * @tc.desc: Covers both outcomes of the scopedTypes lookup: true for every scoped Extended type
 *           and false for a base custom component type. Enumerating all scoped types guards the
 *           scope set against accidental edits.
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, IsExtendedEtsExpressionScope_is_true_only_for_scoped_extended_types)
{
    CustomComponentExpressionProbe tabsProbe("Tabs");
    EXPECT_TRUE(tabsProbe.IsExtendedEtsExpressionScope());

    CustomComponentExpressionProbe webProbe("Web");
    EXPECT_TRUE(webProbe.IsExtendedEtsExpressionScope());

    CustomComponentExpressionProbe tabContentProbe("TabContent");
    EXPECT_TRUE(tabContentProbe.IsExtendedEtsExpressionScope());

    CustomComponentExpressionProbe selectProbe("Select");
    EXPECT_TRUE(selectProbe.IsExtendedEtsExpressionScope());

    CustomComponentExpressionProbe baseProbe("Icon");
    EXPECT_FALSE(baseProbe.IsExtendedEtsExpressionScope());

    CustomComponentExpressionProbe plainProbe("Card");
    EXPECT_FALSE(plainProbe.IsExtendedEtsExpressionScope());
}

// ============================ ResolveExpressionsInValue ============================

/**
 * @tc.name: ResolveExpressionsInValue_returns_invalid_for_invalid_node
 * @tc.desc: An invalid input node short-circuits to an invalid value (!node.IsValid() == true).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_returns_invalid_for_invalid_node)
{
    CustomComponentExpressionProbe probe("Tabs");

    JsonValue result = probe.ResolveExpressionsInValue(JsonValue(), "");
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: ResolveExpressionsInValue_resolves_expression_string_leaf_with_type_preserved
 * @tc.desc: A string leaf that is a valid expression is replaced by the resolved type-preserved
 *           value (IsString && IsExpression && resolved.IsValid() branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_resolves_expression_string_leaf_with_type_preserved)
{
    CustomComponentExpressionProbe probe("Tabs");

    JsonValue result = probe.ResolveExpressionsInValue(MakeString("{{ 1 + 2 }}"), "leaf");
    ASSERT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.GetNumberValue(0.0), 3.0);
}

/**
 * @tc.name: ResolveExpressionsInValue_keeps_raw_string_when_expression_resolve_fails
 * @tc.desc: A string leaf whose expression fails to resolve keeps its raw value
 *           (IsString && IsExpression && !resolved.IsValid() branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_keeps_raw_string_when_expression_resolve_fails)
{
    CustomComponentExpressionProbe probe("Tabs");

    JsonValue result = probe.ResolveExpressionsInValue(MakeString("{{}}"), "leaf");
    ASSERT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue(""), "{{}}");
}

/**
 * @tc.name: ResolveExpressionsInValue_keeps_raw_string_when_expression_eval_errors
 * @tc.desc: A string leaf whose expression hits a runtime (non-parse) evaluation error still keeps
 *           its raw value, while the underlying EvaluateCustomExpression dispatches the error.
 *           Complements the parse-error ("{{}}") case by exercising the
 *           IsString && IsExpression && !resolved.IsValid() branch under EVAL_DIVISION_BY_ZERO.
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_keeps_raw_string_when_expression_eval_errors)
{
    auto probe = MakeDispatchProbe();

    JsonValue result = probe->ResolveExpressionsInValue(MakeString("{{ 1 / 0 }}"), "leaf");
    ASSERT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue(""), "{{ 1 / 0 }}");
    // The leaf's EVAL_DIVISION_BY_ZERO dispatches a runtime error, but the raw value is kept.
    EXPECT_EQ(DispatchCount(), 1U);
}

/**
 * @tc.name: ResolveExpressionsInValue_clones_plain_string_leaf
 * @tc.desc: A string leaf that is not an expression is cloned verbatim
 *           (IsString && !IsExpression branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_clones_plain_string_leaf)
{
    CustomComponentExpressionProbe probe("Tabs");

    JsonValue result = probe.ResolveExpressionsInValue(MakeString("hello"), "leaf");
    ASSERT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue(""), "hello");
}

/**
 * @tc.name: ResolveExpressionsInValue_recurses_into_object_children
 * @tc.desc: Object nodes recurse into every child, preserving keys and resolving expression leaves
 *           (IsObject branch with populated children + resolvedChild.IsValid() Put branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_recurses_into_object_children)
{
    CustomComponentExpressionProbe probe("Tabs");

    std::unique_ptr<JsonAdapter> node = ParseJson(R"({"num":"{{ 1 + 1 }}","txt":"plain","flag":true})");
    ASSERT_NE(node, nullptr);

    JsonValue result = probe.ResolveExpressionsInValue(node->GetRoot(), "");
    ASSERT_TRUE(result.IsObject());
    EXPECT_DOUBLE_EQ(result.GetItem("num").GetNumberValue(0.0), 2.0);
    EXPECT_EQ(result.GetItem("txt").GetStringValue(""), "plain");
    EXPECT_TRUE(result.GetItem("flag").GetBoolValue(false));
}

/**
 * @tc.name: ResolveExpressionsInValue_handles_empty_object_without_children
 * @tc.desc: An object with no children takes the loop's immediate-exit edge and returns an empty
 *           object.
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_handles_empty_object_without_children)
{
    CustomComponentExpressionProbe probe("Tabs");

    std::unique_ptr<JsonAdapter> node = JsonAdapter::CreateObject();
    ASSERT_NE(node, nullptr);

    JsonValue result = probe.ResolveExpressionsInValue(node->GetRoot(), "");
    ASSERT_TRUE(result.IsObject());
    EXPECT_FALSE(result.GetChild().IsValid());
}

/**
 * @tc.name: ResolveExpressionsInValue_recurses_into_array_items
 * @tc.desc: Array nodes recurse into every item, resolving expression leaves and preserving order
 *           (IsArray branch with populated items + resolvedItem.IsValid() Append branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_recurses_into_array_items)
{
    CustomComponentExpressionProbe probe("Tabs");

    std::unique_ptr<JsonAdapter> node = ParseJson(R"(["{{ 2 + 3 }}","keep",42])");
    ASSERT_NE(node, nullptr);

    JsonValue result = probe.ResolveExpressionsInValue(node->GetRoot(), "");
    ASSERT_TRUE(result.IsArray());
    ASSERT_EQ(result.GetArraySize(), 3);
    EXPECT_DOUBLE_EQ(result.GetArrayItem(0).GetNumberValue(0.0), 5.0);
    EXPECT_EQ(result.GetArrayItem(1).GetStringValue(""), "keep");
    EXPECT_DOUBLE_EQ(result.GetArrayItem(2).GetNumberValue(0.0), 42.0);
}

/**
 * @tc.name: ResolveExpressionsInValue_handles_empty_array_without_items
 * @tc.desc: An array with no items takes the loop's immediate-exit edge and returns an empty array.
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_handles_empty_array_without_items)
{
    CustomComponentExpressionProbe probe("Tabs");

    std::unique_ptr<JsonAdapter> node = JsonAdapter::CreateArray();
    ASSERT_NE(node, nullptr);

    JsonValue result = probe.ResolveExpressionsInValue(node->GetRoot(), "");
    ASSERT_TRUE(result.IsArray());
    EXPECT_EQ(result.GetArraySize(), 0);
}

/**
 * @tc.name: ResolveExpressionsInValue_clones_primitive_non_string_node
 * @tc.desc: Number/bool/null leaves fall through to the clone-as-is path
 *           (!IsString && !IsObject && !IsArray branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_clones_primitive_non_string_node)
{
    CustomComponentExpressionProbe probe("Tabs");

    JsonValue numberResult = probe.ResolveExpressionsInValue(MakeNumber(9.0), "n");
    ASSERT_TRUE(numberResult.IsValid());
    EXPECT_DOUBLE_EQ(numberResult.GetNumberValue(0.0), 9.0);

    JsonValue boolResult = probe.ResolveExpressionsInValue(MakeBool(true), "b");
    ASSERT_TRUE(boolResult.IsValid());
    EXPECT_TRUE(boolResult.GetBoolValue(false));
}

/**
 * @tc.name: ResolveExpressionsInValue_resolves_nested_structures_recursively
 * @tc.desc: Mixed nested object/array structures exercise the recursive descent and path building.
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, ResolveExpressionsInValue_resolves_nested_structures_recursively)
{
    CustomComponentExpressionProbe probe("Tabs");

    std::unique_ptr<JsonAdapter> node = ParseJson(R"({"outer":{"inner":"{{ 7 }}"},"list":["{{ 1 }}","literal"]})");
    ASSERT_NE(node, nullptr);

    JsonValue result = probe.ResolveExpressionsInValue(node->GetRoot(), "");
    ASSERT_TRUE(result.IsObject());
    EXPECT_DOUBLE_EQ(result.GetItem("outer").GetItem("inner").GetNumberValue(0.0), 7.0);
    ASSERT_TRUE(result.GetItem("list").IsArray());
    EXPECT_DOUBLE_EQ(result.GetItem("list").GetArrayItem(0).GetNumberValue(0.0), 1.0);
    EXPECT_EQ(result.GetItem("list").GetArrayItem(1).GetStringValue(""), "literal");
}

// ============================ BuildCustomProps expression scope ============================

/**
 * @tc.name: BuildCustomProps_resolves_expressions_for_scoped_extended_type
 * @tc.desc: BuildCustomProps routes through ResolveExpressionsInValue for scoped Extended types
 *           (IsExtendedEtsExpressionScope() == true branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, BuildCustomProps_resolves_expressions_for_scoped_extended_type)
{
    CustomComponentExpressionProbe probe("Tabs");
    probe.customPropertyNames_.insert("title");
    probe.properties_["title"] = MakeString("{{ 1 + 4 }}");

    JsonValue customProps = probe.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    EXPECT_DOUBLE_EQ(customProps.GetItem("title").GetNumberValue(0.0), 5.0);
}

/**
 * @tc.name: BuildCustomProps_keeps_literals_for_non_scoped_type
 * @tc.desc: BuildCustomProps returns the custom props unchanged for non-scoped types, leaving
 *           expression-looking strings literal (IsExtendedEtsExpressionScope() == false branch).
 * @tc.type: FUNC
 */
TEST_F(CustomComponentExpressionTddTest, BuildCustomProps_keeps_literals_for_non_scoped_type)
{
    CustomComponentExpressionProbe probe("Icon");
    probe.customPropertyNames_.insert("name");
    probe.properties_["name"] = MakeString("{{ not resolved }}");

    JsonValue customProps = probe.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    // Untouched literal: not routed through ResolveExpressionsInValue.
    EXPECT_EQ(customProps.GetItem("name").GetStringValue(""), "{{ not resolved }}");
}

/**
 * @tc.name: CustomExpressionBindingHelpers_cover_prefix_resolution_and_expression_detection
 * @tc.desc: Covers the standalone helper utilities used by the split expression-binding module.
 * @tc.type: FUNC
 */
TEST_F(
    CustomComponentExpressionTddTest, CustomExpressionBindingHelpers_cover_prefix_resolution_and_expression_detection)
{
    EXPECT_FALSE(IsExpressionStringValue(JsonValue()));
    EXPECT_FALSE(IsExpressionStringValue(MakeString("plain")));
    EXPECT_TRUE(IsExpressionStringValue(MakeString("{{ 1 + 1 }}")));
    EXPECT_FALSE(IsExpressionStringValue(MakeNumber(3.0)));

    EXPECT_TRUE(BuildCustomExpressionBindingKey("").empty());
    EXPECT_EQ(BuildCustomExpressionBindingKey("title"), "__a2uiExpr__:title");

    EXPECT_FALSE(IsCustomExpressionBindingProperty("title"));
    EXPECT_TRUE(IsCustomExpressionBindingProperty("__a2uiExpr__:title"));

    EXPECT_EQ(ResolveCustomExpressionSourceProperty("title"), "title");
    EXPECT_EQ(ResolveCustomExpressionSourceProperty("__a2uiExpr__:title"), "title");
}

/**
 * @tc.name: RefreshCustomExpressionBindings_replaces_stale_bindings_and_collects_unique_paths
 * @tc.desc: Verifies recursive dependency extraction, duplicate-path de-duplication, parse-failure
 *           fallback, and literal-value cleanup for the standalone expression-binding refresher.
 * @tc.type: FUNC
 */
TEST_F(
    CustomComponentExpressionTddTest, RefreshCustomExpressionBindings_replaces_stale_bindings_and_collects_unique_paths)
{
    CustomComponentExpressionProbe probe("Tabs");
    probe.AddBinding("__a2uiExpr__:title", "/stale");

    RefreshCustomExpressionBindings(probe, "", MakeString("{{ $__dataModel.ignored }}"));
    EXPECT_EQ(CollectBindingPaths(probe, "__a2uiExpr__:title"), std::set<std::string>({ "/stale" }));

    RefreshCustomExpressionBindings(probe, "title", JsonValue());
    EXPECT_TRUE(CollectBindingPaths(probe, "__a2uiExpr__:title").empty());

    RefreshCustomExpressionBindings(probe, "title", MakeString("{{}}"));
    EXPECT_TRUE(CollectBindingPaths(probe, "__a2uiExpr__:title").empty());

    RefreshCustomExpressionBindings(probe, "title", MakeString("{{ $__dataModel[ }}"));
    EXPECT_TRUE(CollectBindingPaths(probe, "__a2uiExpr__:title").empty());

    std::unique_ptr<JsonAdapter> nested = ParseJson(
        R"({
            "label": "{{ $__dataModel.user.name }}",
            "meta": {
                "count": "{{ size($__dataModel.items) }}",
                "duplicate": "{{ $__dataModel.user.name }}"
            },
            "list": [
                "{{ $__dataModel.user.alias }}",
                { "inner": "{{ $__dataModel.user.alias }}" },
                "plain"
            ]
        })");
    ASSERT_NE(nested, nullptr);

    RefreshCustomExpressionBindings(probe, "title", nested->GetRoot());

    std::set<std::string> bindingPaths = CollectBindingPaths(probe, "__a2uiExpr__:title");
    EXPECT_EQ(bindingPaths.size(), 3U);
    EXPECT_TRUE(bindingPaths.find("/user/name") != bindingPaths.end());
    EXPECT_TRUE(bindingPaths.find("/items") != bindingPaths.end());
    EXPECT_TRUE(bindingPaths.find("/user/alias") != bindingPaths.end());

    RefreshCustomExpressionBindings(probe, "title", MakeString("literal"));
    EXPECT_TRUE(CollectBindingPaths(probe, "__a2uiExpr__:title").empty());
}
