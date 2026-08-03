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

#include "components/A2UI/slider/SliderComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

namespace {

std::vector<ArkUI_NodeHandle> g_sliderCreateNodeResults;
size_t g_sliderCreateNodeIndex = 0;

ArkUI_NodeHandle CreateSliderNodeFromSequence(ArkUI_NodeType type)
{
    if (g_sliderCreateNodeIndex < g_sliderCreateNodeResults.size()) {
        return g_sliderCreateNodeResults[g_sliderCreateNodeIndex++];
    }
    ++g_sliderCreateNodeIndex;
    return TrackCreateNode(type);
}

void UseSliderCreateNodeSequence(ArkUI_NativeNodeAPI_1* api, const std::vector<ArkUI_NodeHandle>& results)
{
    g_sliderCreateNodeResults = results;
    g_sliderCreateNodeIndex = 0;
    api->createNode = CreateSliderNodeFromSequence;
}

int32_t CountSliderAddChildCalls(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.addChildCalls) {
        if (call.first == parent && call.second == child) {
            ++count;
        }
    }
    return count;
}

class SliderComponentProbe : public SliderComponent {
public:
    void InvokeOnAttachToParent()
    {
        OnAttachToParent();
    }
};

} // namespace

class SliderComponentTddTest : public A2UIComponentTddTest {};

TEST_F(SliderComponentTddTest, L0_slider_should_create_internal_nodes_and_report_type)
{
    auto slider = std::make_shared<SliderComponent>();
    ASSERT_NE(slider, nullptr);
    ArkUI_NodeHandle columnNode = slider->GetNativeView();
    ArkUI_NodeHandle textNode = FindCreatedNode(ARKUI_NODE_TEXT);
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);

    EXPECT_EQ(slider->GetType(), "Slider");
    EXPECT_EQ(columnNode, FindCreatedNode(ARKUI_NODE_COLUMN));
    ASSERT_NE(textNode, nullptr);
    ASSERT_NE(sliderNode, nullptr);
    EXPECT_FALSE(HasAddChildCall(columnNode, textNode));
    EXPECT_FALSE(HasAddChildCall(columnNode, sliderNode));

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(slider);

    EXPECT_TRUE(HasAddChildCall(columnNode, textNode));
    EXPECT_TRUE(HasAddChildCall(columnNode, sliderNode));
}

TEST_F(SliderComponentTddTest, L0_slider_should_apply_descriptor_and_clamp_value)
{
    auto slider = std::make_shared<SliderComponent>();
    ArkUI_NodeHandle columnNode = slider->GetNativeView();
    ArkUI_NodeHandle textNode = FindCreatedNode(ARKUI_NODE_TEXT);
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);
    auto descriptor =
        ParseJson(R"({"id":"volume","component":"Slider","label":"Volume","min":10,"max":20,"value":50})");
    ASSERT_NE(descriptor, nullptr);

    slider->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(textNode, NODE_TEXT_CONTENT, "Volume");
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MIN_VALUE, 10.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MAX_VALUE, 20.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_VALUE, 20.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_STEP, 1.0F);
    ExpectI32Attribute(sliderNode, NODE_SLIDER_STYLE, ARKUI_SLIDER_STYLE_OUT_SET);
    ExpectI32Attribute(sliderNode, NODE_ENABLED, 1);
    ExpectI32Attribute(columnNode, NODE_COLUMN_ALIGN_ITEMS, ARKUI_HORIZONTAL_ALIGNMENT_CENTER);
}

TEST_F(SliderComponentTddTest, L0_slider_should_reset_invalid_min_max_and_disable_on_failed_checks)
{
    auto slider = std::make_shared<SliderComponent>();
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);
    auto descriptor =
        ParseJson(R"({"id":"guardedSlider","component":"Slider","label":"Guarded","min":30,"max":10,"value":150,)"
                  R"("checks":[{"condition":{"call":"required","args":{"value":""}},"message":"required"}]})");
    ASSERT_NE(descriptor, nullptr);

    slider->ApplyDescriptor(descriptor->GetRoot());

    ExpectF32Attribute(sliderNode, NODE_SLIDER_MIN_VALUE, 0.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MAX_VALUE, 100.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_VALUE, 100.0F);
    ExpectI32Attribute(sliderNode, NODE_ENABLED, 0);
}

TEST_F(SliderComponentTddTest, L0_slider_should_apply_public_slider_setters)
{
    auto slider = std::make_shared<SliderComponent>();
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);

    slider->SetMinValue(5.0F);
    slider->SetMaxValue(55.0F);
    slider->SetValue(40.0F);
    slider->SetStep(5.0F);
    slider->SetStyle(A2UISliderStyle::OUT_SET);

    ExpectF32Attribute(sliderNode, NODE_SLIDER_MIN_VALUE, 5.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MAX_VALUE, 55.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_VALUE, 40.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_STEP, 5.0F);
    ExpectI32Attribute(sliderNode, NODE_SLIDER_STYLE, ARKUI_SLIDER_STYLE_OUT_SET);
}

TEST_F(SliderComponentTddTest, L0_slider_should_reclamp_value_when_dynamic_min_max_or_value_changes)
{
    auto slider = std::make_shared<SliderComponent>();
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);
    auto descriptor =
        ParseJson(R"({"id":"dynamicSlider","component":"Slider","label":"Dynamic","min":10,"max":20,"value":15})");
    ASSERT_NE(descriptor, nullptr);
    slider->ApplyDescriptor(descriptor->GetRoot());

    auto highValue = JsonAdapter::CreateNumber(50.0);
    ASSERT_NE(highValue, nullptr);
    slider->OnDataUpdate("value", highValue->GetRoot());
    ExpectF32Attribute(sliderNode, NODE_SLIDER_VALUE, 20.0F);

    auto invalidMin = JsonAdapter::CreateNumber(30.0);
    ASSERT_NE(invalidMin, nullptr);
    slider->OnDataUpdate("min", invalidMin->GetRoot());
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MIN_VALUE, 0.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MAX_VALUE, 100.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_VALUE, 20.0F);
}

TEST_F(SliderComponentTddTest, L0_slider_should_return_theme_when_surface_context_is_available)
{
    auto slider = std::make_shared<SliderComponent>();
    PrepareThemeContext(*slider);

    EXPECT_NE(slider->GetTheme(), nullptr);
}

TEST_F(SliderComponentTddTest, L0_slider_should_handle_config_change_with_null_theme_gracefully)
{
    auto slider = std::make_shared<SliderComponent>();
    // Do NOT prepare theme context, so GetTheme() will return nullptr

    ThemeContext context;
    // Should not crash even if theme is null
    EXPECT_NO_THROW(slider->OnConfigChange(context));
}

TEST_F(SliderComponentTddTest, L0_slider_should_apply_attributes_without_theme)
{
    auto slider = std::make_shared<SliderComponent>();
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);

    // Apply descriptor without preparing theme context
    auto descriptor =
        ParseJson(R"({"id":"noThemeSlider","component":"Slider","label":"No Theme","min":0,"max":100,"value":50})");
    ASSERT_NE(descriptor, nullptr);

    // Should not crash and should apply basic attributes
    EXPECT_NO_THROW(slider->ApplyDescriptor(descriptor->GetRoot()));

    ExpectF32Attribute(sliderNode, NODE_SLIDER_MIN_VALUE, 0.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MAX_VALUE, 100.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_VALUE, 50.0F);

    // Verify that selected color was NOT set (since theme is null)
    // We check that there is no call to NODE_SLIDER_SELECTED_COLOR
    EXPECT_EQ(FindLastAttributeCall(sliderNode, NODE_SLIDER_SELECTED_COLOR), nullptr);
}

TEST_F(SliderComponentTddTest, L0_slider_should_reset_defaults_when_dynamic_max_becomes_less_than_min)
{
    auto slider = std::make_shared<SliderComponent>();
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);

    // Initial state: min=10, max=20
    auto descriptor =
        ParseJson(R"({"id":"dynamicMinMax","component":"Slider","label":"Dynamic","min":10,"max":20,"value":15})");
    ASSERT_NE(descriptor, nullptr);
    slider->ApplyDescriptor(descriptor->GetRoot());

    ExpectF32Attribute(sliderNode, NODE_SLIDER_MIN_VALUE, 10.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MAX_VALUE, 20.0F);

    // Update max to 5, which is less than current min (10)
    // This should trigger the reset logic in OnDataUpdate
    auto newMax = JsonAdapter::CreateNumber(5.0);
    ASSERT_NE(newMax, nullptr);
    slider->OnDataUpdate("max", newMax->GetRoot());

    // Expect defaults: min=0, max=100
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MIN_VALUE, 0.0F);
    ExpectF32Attribute(sliderNode, NODE_SLIDER_MAX_VALUE, 100.0F);
    // Value should be clamped to new max (100), but since value was 15 and max became 100 (after reset),
    // actually the reset happens first, then clamp.
    // In OnDataUpdate: if min >= max, reset to 0/100. Then if property is value/min/max, clamp.
    // Value is still 15 internally? No, SetValue is called with ClampValue(value_, minValue_, maxValue_).
    // After reset, min=0, max=100. Value 15 is within range. So value remains 15.
    ExpectF32Attribute(sliderNode, NODE_SLIDER_VALUE, 15.0F);
}

TEST_F(SliderComponentTddTest, L0_slider_should_not_attach_internal_nodes_twice)
{
    auto slider = std::make_shared<SliderComponentProbe>();
    ArkUI_NodeHandle columnNode = slider->GetNativeView();
    ArkUI_NodeHandle textNode = FindCreatedNode(ARKUI_NODE_TEXT);
    ArkUI_NodeHandle sliderNode = FindCreatedNode(ARKUI_NODE_SLIDER);
    ASSERT_NE(columnNode, nullptr);
    ASSERT_NE(textNode, nullptr);
    ASSERT_NE(sliderNode, nullptr);

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(slider);
    ASSERT_EQ(CountSliderAddChildCalls(columnNode, textNode), 1);
    ASSERT_EQ(CountSliderAddChildCalls(columnNode, sliderNode), 1);

    slider->InvokeOnAttachToParent();

    EXPECT_EQ(CountSliderAddChildCalls(columnNode, textNode), 1);
    EXPECT_EQ(CountSliderAddChildCalls(columnNode, sliderNode), 1);
}

TEST_F(SliderComponentTddTest, L0_slider_should_skip_attach_when_native_view_is_missing)
{
    ArkUI_NodeHandle textNode = reinterpret_cast<ArkUI_NodeHandle>(0x820010);
    ArkUI_NodeHandle sliderNode = reinterpret_cast<ArkUI_NodeHandle>(0x820011);
    UseSliderCreateNodeSequence(api_, { nullptr, textNode, sliderNode });

    auto slider = std::make_shared<SliderComponent>();
    EXPECT_EQ(slider->GetNativeView(), nullptr);
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(slider);

    EXPECT_FALSE(HasAddChildCall(nullptr, textNode));
    EXPECT_FALSE(HasAddChildCall(nullptr, sliderNode));
}

TEST_F(SliderComponentTddTest, L0_slider_should_skip_attach_when_text_node_is_missing)
{
    ArkUI_NodeHandle columnNode = reinterpret_cast<ArkUI_NodeHandle>(0x820020);
    ArkUI_NodeHandle sliderNode = reinterpret_cast<ArkUI_NodeHandle>(0x820021);
    UseSliderCreateNodeSequence(api_, { columnNode, nullptr, sliderNode });

    auto slider = std::make_shared<SliderComponent>();
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(slider);

    EXPECT_FALSE(HasAddChildCall(columnNode, sliderNode));
}

TEST_F(SliderComponentTddTest, L0_slider_should_skip_attach_when_slider_node_is_missing)
{
    ArkUI_NodeHandle columnNode = reinterpret_cast<ArkUI_NodeHandle>(0x820030);
    ArkUI_NodeHandle textNode = reinterpret_cast<ArkUI_NodeHandle>(0x820031);
    UseSliderCreateNodeSequence(api_, { columnNode, textNode, nullptr });

    auto slider = std::make_shared<SliderComponent>();
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(slider);

    EXPECT_FALSE(HasAddChildCall(columnNode, textNode));
}
