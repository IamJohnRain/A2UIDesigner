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

#include "components/extended/ExtendedImageComponent.h"

#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "data/DataBinding.h"
#include "styles/StyleResolver.h"
#include "utils/JsonAdapter.h"

#include "A2UIComponentTddTestHelper.h"
#include "ArkUINodeApiAdapter.h"
#include "SchemaWarningTestHelper.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

constexpr char DEFAULT_IMAGE_PLACEHOLDER_ALT[] = "resources/base/media/placeHolder_E5E5EA.png";
constexpr int32_t MIN_API_VERSION_IMAGE_OBJECT_FIT_MATRIX = 21;
constexpr int32_t OBJECT_FIT_NONE_MATRIX_VALUE = 15;

constexpr int32_t ToA2UIObjectFitValue(A2UIObjectFit value)
{
    return static_cast<int32_t>(value);
}

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Image", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

class ExtendedImageComponentTest : public A2UIComponentTddTest {
protected:
    void SetUp() override
    {
        A2UIComponentTddTest::SetUp();
        slot_.SetSurfaceId("surface-extended-image");
        slot_.SetRenderId(14);
    }

    SurfaceSlot slot_;
};

class ExtendedImageComponentSchemaWarningTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        callbacks_ = TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    }

    TestHelpers::DispatchCallbacks callbacks_;
};

class TestableExtendedImageComponent : public ExtendedImageComponent {
public:
    using ExtendedImageComponent::ApplyPrivateAttributes;
    using ExtendedImageComponent::OnDataUpdate;
    using ExtendedImageComponent::SetApplyingStyleDeltaUpdateForTest;
};

TEST_F(ExtendedImageComponentTest, L0_should_return_image_type_and_default_object_fit)
{
    ExtendedImageComponent component;

    EXPECT_EQ(component.GetType(), "Image");
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
    ExpectI32Attribute(component.GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_COVER);
}

TEST_F(ExtendedImageComponentTest, L0_should_apply_src_aspect_ratio_and_object_fit_from_descriptor)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "https://example.com/photo.png",
                "styles": {
                    "aspectRatio": 1.5,
                    "objectFit": "contain"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetSrcValueForTest(), "https://example.com/photo.png");
    EXPECT_FLOAT_EQ(image->GetAspectRatioForTest(), 1.5F);
    EXPECT_EQ(image->GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::CONTAIN));

    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_SRC, "https://example.com/photo.png");
    ExpectF32Attribute(image->GetNativeView(), NODE_ASPECT_RATIO, 1.5F);
    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_CONTAIN);
}

TEST_F(ExtendedImageComponentTest, L0_should_apply_default_placeholder_alt_when_description_is_missing)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "https://example.com/photo.png"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetAltValueForTest(), DEFAULT_IMAGE_PLACEHOLDER_ALT);
    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_ALT, DEFAULT_IMAGE_PLACEHOLDER_ALT);
}

TEST_F(ExtendedImageComponentTest, L0_should_ignore_description_and_keep_default_placeholder_alt)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "https://example.com/photo.png",
                "description": "resources/base/media/custom-placeholder.png"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetAltValueForTest(), DEFAULT_IMAGE_PLACEHOLDER_ALT);
    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_ALT, DEFAULT_IMAGE_PLACEHOLDER_ALT);

    std::unique_ptr<JsonAdapter> clearAlt = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "https://example.com/photo.png",
                "description": ""
            }
        ]
    })");
    ASSERT_NE(clearAlt, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(clearAlt->GetRoot()));

    image = std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetAltValueForTest(), DEFAULT_IMAGE_PLACEHOLDER_ALT);
    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_ALT, DEFAULT_IMAGE_PLACEHOLDER_ALT);
}

TEST_F(ExtendedImageComponentTest, L0_should_reset_alt_when_empty_via_node_applier)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "https://example.com/photo.png"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);

    image->SetAltForTest("");

    EXPECT_EQ(image->GetAltValueForTest(), "");
    EXPECT_TRUE(HasResetAttributeCall(image->GetNativeView(), NODE_IMAGE_ALT));
}

TEST_F(ExtendedImageComponentTest, L0_should_cover_alt_native_path_empty_and_missing_native_state)
{
    ExtendedImageComponent component;
    ArkUI_NodeHandle originalView = component.GetNativeView();
    void* originalApi = component.GetNativeNodeApiForTest();

    int32_t originalAltSetCalls = CountAttributeCall(originalView, NODE_IMAGE_ALT);

    component.SetAltForTest("");
    EXPECT_EQ(component.GetAltValueForTest(), "");
    EXPECT_TRUE(HasResetAttributeCall(originalView, NODE_IMAGE_ALT));
    EXPECT_EQ(CountAttributeCall(originalView, NODE_IMAGE_ALT), originalAltSetCalls);

    component.OverrideNativeStateForTest(originalApi, nullptr);
    component.SetAltForTest("missing-view");
    EXPECT_EQ(CountAttributeCall(originalView, NODE_IMAGE_ALT), originalAltSetCalls);

    component.OverrideNativeStateForTest(nullptr, originalView);
    component.SetAltForTest("missing-api");
    EXPECT_EQ(CountAttributeCall(originalView, NODE_IMAGE_ALT), originalAltSetCalls + 1);
    ExpectStringAttribute(originalView, NODE_IMAGE_ALT, "missing-api");

    component.OverrideNativeStateForTest(originalApi, originalView);
}

TEST_F(ExtendedImageComponentTest, L0_should_fallback_to_empty_src_and_cover_when_properties_missing_or_unknown)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> missingSrc = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image"
            }
        ]
    })");
    ASSERT_NE(missingSrc, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(missingSrc->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetSrcValueForTest(), "");
    EXPECT_EQ(image->GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    std::unique_ptr<JsonAdapter> unknownFit = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "x",
                "styles": {
                    "objectFit": "invalidValue"
                }
            }
        ]
    })");
    ASSERT_NE(unknownFit, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(unknownFit->GetRoot()));

    image = std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
}

TEST_F(ExtendedImageComponentTest, L0_should_parse_all_supported_object_fit_enum_values)
{
    struct Case {
        const char* value;
        int32_t expected;
    };
    const Case cases[] = { { "contain", ToA2UIObjectFitValue(A2UIObjectFit::CONTAIN) },
        { "cover", ToA2UIObjectFitValue(A2UIObjectFit::COVER) }, { "auto", ToA2UIObjectFitValue(A2UIObjectFit::AUTO) },
        { "fill", ToA2UIObjectFitValue(A2UIObjectFit::FILL) },
        { "scaleDown", ToA2UIObjectFitValue(A2UIObjectFit::SCALE_DOWN) },
        { "none", ToA2UIObjectFitValue(A2UIObjectFit::NONE) },
        { "topStart", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_TOP_START) },
        { "top", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_TOP) },
        { "topEnd", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_TOP_END) },
        { "start", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_START) },
        { "center", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_CENTER) },
        { "end", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_END) },
        { "bottomStart", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_START) },
        { "bottom", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_BOTTOM) },
        { "bottomEnd", ToA2UIObjectFitValue(A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_END) },
        { "matrix", ToA2UIObjectFitValue(A2UIObjectFit::NONE_MATRIX) } };

    slot_.SetApiVersion(MIN_API_VERSION_IMAGE_OBJECT_FIT_MATRIX);
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    for (const auto& testCase : cases) {
        std::string json = R"({
            "components": [
                {
                    "id": "root",
                    "component": "Image",
                    "src": "x",
                    "styles": {
                        "objectFit": ")" +
                           std::string(testCase.value) + R"("
                    }
                }
            ]
        })";
        std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(json);
        ASSERT_NE(message, nullptr);
        ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

        std::shared_ptr<ExtendedImageComponent> image =
            std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
        ASSERT_NE(image, nullptr);
        EXPECT_EQ(image->GetObjectFitForTest(), testCase.expected) << testCase.value;
    }
}

TEST_F(ExtendedImageComponentTest, L0_should_fallback_matrix_object_fit_when_runtime_api_is_below_21)
{
    slot_.SetApiVersion(MIN_API_VERSION_IMAGE_OBJECT_FIT_MATRIX - 1);
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "x",
                "styles": {
                    "objectFit": "matrix"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_COVER);
}

TEST_F(ExtendedImageComponentTest, L0_should_apply_matrix_object_fit_when_runtime_api_is_at_least_21)
{
    slot_.SetApiVersion(MIN_API_VERSION_IMAGE_OBJECT_FIT_MATRIX);
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "x",
                "styles": {
                    "objectFit": "matrix"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::NONE_MATRIX));
    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, OBJECT_FIT_NONE_MATRIX_VALUE);
}

TEST_F(ExtendedImageComponentTest, L0_should_set_and_reset_image_src_via_applier_and_native_paths)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> initial = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "https://example.com/img.png",
                "styles": {}
            }
        ]
    })");
    ASSERT_NE(initial, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(initial->GetRoot()));

    std::shared_ptr<ExtendedImageComponent> image =
        std::dynamic_pointer_cast<ExtendedImageComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(image, nullptr);
    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_SRC, "https://example.com/img.png");

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Image",
                "src": "",
                "styles": {}
            }
        ]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));
    EXPECT_TRUE(HasResetAttributeCall(image->GetNativeView(), NODE_IMAGE_SRC));

    ExtendedImageComponent nativeComponent;
    PropertyDeclaration declaration = nativeComponent.GetPrivatePropertyDeclarationForTest("src");
    ASSERT_TRUE(static_cast<bool>(declaration.applyValue));

    std::unique_ptr<JsonAdapter> setValue = JsonAdapter::Parse(R"({"value":"https://example.com/native.png"})");
    ASSERT_NE(setValue, nullptr);
    declaration.applyValue(setValue->GetRoot().GetItem("value"));
    ExpectStringAttribute(nativeComponent.GetNativeView(), NODE_IMAGE_SRC, "https://example.com/native.png");

    std::unique_ptr<JsonAdapter> resetValue = JsonAdapter::Parse(R"({"value":""})");
    ASSERT_NE(resetValue, nullptr);
    declaration.applyValue(resetValue->GetRoot().GetItem("value"));
    EXPECT_TRUE(HasResetAttributeCall(nativeComponent.GetNativeView(), NODE_IMAGE_SRC));
}

TEST_F(ExtendedImageComponentTest, L0_should_register_image_src_path_binding_and_apply_runtime_updates)
{
    TestableExtendedImageComponent component;

    std::unique_ptr<JsonAdapter> bindingSrc = JsonAdapter::Parse(R"({
        "src": {
            "path": "/image/src"
        }
    })");
    ASSERT_NE(bindingSrc, nullptr);
    component.ApplyPrivateAttributes(bindingSrc->GetRoot());

    const std::vector<DataBinding>& bindings = component.GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "src");
    EXPECT_EQ(bindings[0].dataPath_, "/image/src");
    EXPECT_EQ(bindings[0].type_, BindingType::PATH);

    std::unique_ptr<JsonAdapter> updatedSrc = JsonAdapter::Parse(R"("https://example.com/updated.png")");
    ASSERT_NE(updatedSrc, nullptr);
    component.OnDataUpdate("src", updatedSrc->GetRoot());
    EXPECT_EQ(component.GetSrcValueForTest(), "https://example.com/updated.png");
    ExpectStringAttribute(component.GetNativeView(), NODE_IMAGE_SRC, "https://example.com/updated.png");
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(ExtendedImageComponentTest, L0_should_resolve_literal_expression_for_image_src)
{
    TestableExtendedImageComponent component;

    std::unique_ptr<JsonAdapter> expressionSrc = JsonAdapter::Parse(R"({
        "src": "{{ 'https://example.com/expression.png' }}"
    })");
    ASSERT_NE(expressionSrc, nullptr);
    component.ApplyPrivateAttributes(expressionSrc->GetRoot());

    EXPECT_TRUE(component.GetDataBindings().empty());
    EXPECT_EQ(component.GetSrcValueForTest(), "https://example.com/expression.png");
    ExpectStringAttribute(component.GetNativeView(), NODE_IMAGE_SRC, "https://example.com/expression.png");
}
#endif

TEST_F(ExtendedImageComponentTest, L0_should_apply_object_fit_and_aspect_ratio_via_native_api_without_node_applier)
{
    ExtendedImageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "aspectRatio": 3.25,
        "objectFit": "matrix"
    })");
    ASSERT_NE(styles, nullptr);

    component.ApplyComponentSpecificStylesForTest(styles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 3.25F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
    ExpectF32Attribute(component.GetNativeView(), NODE_ASPECT_RATIO, 3.25F);
    ExpectI32Attribute(component.GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_COVER);
}

TEST_F(ExtendedImageComponentTest, L0_should_ignore_non_object_styles_and_unknown_private_property)
{
    ExtendedImageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"("not-an-object")");
    ASSERT_NE(styles, nullptr);

    component.ApplyComponentSpecificStylesForTest(styles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    PropertyDeclaration declaration = component.GetPrivatePropertyDeclarationForTest("unknown");
    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));

    PropertyDeclaration descriptionDeclaration = component.GetPrivatePropertyDeclarationForTest("description");
    EXPECT_TRUE(descriptionDeclaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(descriptionDeclaration.applyValue));
}

TEST_F(ExtendedImageComponentTest, L0_should_skip_native_updates_when_native_view_or_api_is_missing)
{
    ExtendedImageComponent component;
    ArkUI_NodeHandle originalView = component.GetNativeView();
    void* originalApi = component.GetNativeNodeApiForTest();
    int32_t originalSrcCalls = CountAttributeCall(originalView, NODE_IMAGE_SRC);
    int32_t originalAspectCalls = CountAttributeCall(originalView, NODE_ASPECT_RATIO);
    int32_t originalObjectFitCalls = CountAttributeCall(originalView, NODE_IMAGE_OBJECT_FIT);

    PropertyDeclaration declaration = component.GetPrivatePropertyDeclarationForTest("src");
    ASSERT_TRUE(static_cast<bool>(declaration.applyValue));

    component.OverrideNativeStateForTest(originalApi, nullptr);
    std::unique_ptr<JsonAdapter> missingViewValue =
        JsonAdapter::Parse(R"({"value":"https://example.com/missing-view.png"})");
    ASSERT_NE(missingViewValue, nullptr);
    declaration.applyValue(missingViewValue->GetRoot().GetItem("value"));

    std::unique_ptr<JsonAdapter> missingViewStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 4.0,
        "objectFit": "fill"
    })");
    ASSERT_NE(missingViewStyles, nullptr);
    ArkUINodeApiAdapter missingViewApplier = CreateNodeApiAdapter(component);
    component.ApplyComponentSpecificStylesForTest(missingViewStyles->GetRoot(), missingViewApplier);

    EXPECT_EQ(CountAttributeCall(originalView, NODE_IMAGE_SRC), originalSrcCalls);
    EXPECT_EQ(CountAttributeCall(originalView, NODE_ASPECT_RATIO), originalAspectCalls);
    EXPECT_EQ(CountAttributeCall(originalView, NODE_IMAGE_OBJECT_FIT), originalObjectFitCalls);

    component.OverrideNativeStateForTest(nullptr, originalView);
    std::unique_ptr<JsonAdapter> missingApiValue = JsonAdapter::Parse(R"({"value":""})");
    ASSERT_NE(missingApiValue, nullptr);
    declaration.applyValue(missingApiValue->GetRoot().GetItem("value"));

    std::unique_ptr<JsonAdapter> missingApiStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 5.0,
        "objectFit": "topStart"
    })");
    ASSERT_NE(missingApiStyles, nullptr);
    ArkUINodeApiAdapter missingApiApplier = CreateNodeApiAdapter(component);
    component.ApplyComponentSpecificStylesForTest(missingApiStyles->GetRoot(), missingApiApplier);

    EXPECT_EQ(CountAttributeCall(originalView, NODE_IMAGE_SRC), originalSrcCalls);
    EXPECT_EQ(CountAttributeCall(originalView, NODE_ASPECT_RATIO), originalAspectCalls + 1);
    EXPECT_EQ(CountAttributeCall(originalView, NODE_IMAGE_OBJECT_FIT), originalObjectFitCalls + 1);
    ExpectF32Attribute(originalView, NODE_ASPECT_RATIO, 5.0F);
    ExpectI32Attribute(originalView, NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_START);

    component.OverrideNativeStateForTest(originalApi, originalView);
}

TEST_F(ExtendedImageComponentTest, L0_should_create_image_via_extended_factory)
{
    auto& factory = ExtendedComponentFactory::GetInstance();

    EXPECT_TRUE(factory.IsExtendedComponent("Image"));
    std::shared_ptr<ExtendedComponent> component = factory.CreateComponent("Extended.Image");
    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->GetType(), "Image");
}

TEST_F(ExtendedImageComponentTest, L0_should_fallback_image_styles_to_defaults_for_invalid_style_deltas)
{
    TestableExtendedImageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::Parse("true");
    ASSERT_NE(nonObjectStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(nonObjectStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 2.5,
        "objectFit": "fill"
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 2.5F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::FILL));

    component.SetApplyingStyleDeltaUpdateForTest(true);
    component.ApplyComponentSpecificStylesForTest(nonObjectStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    std::unique_ptr<JsonAdapter> invalidFullStyles = JsonAdapter::Parse(R"({
        "aspectRatio": false,
        "objectFit": "unsupported"
    })");
    ASSERT_NE(invalidFullStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidFullStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    std::unique_ptr<JsonAdapter> invalidObjectFitDelta = JsonAdapter::Parse(R"("unsupported")");
    ASSERT_NE(invalidObjectFitDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("objectFit"), invalidObjectFitDelta->GetRoot());
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    std::unique_ptr<JsonAdapter> invalidAspectRatioDelta = JsonAdapter::Parse("false");
    ASSERT_NE(invalidAspectRatioDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("aspectRatio"), invalidAspectRatioDelta->GetRoot());
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);

    component.SetApplyingStyleDeltaUpdateForTest(false);
    component.ApplyComponentSpecificStylesForTest(invalidFullStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
}

TEST_F(ExtendedImageComponentTest, L0_should_cover_private_attribute_non_object_and_missing_full_style_fields)
{
    TestableExtendedImageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> nonObjectDescriptor = JsonAdapter::Parse("true");
    ASSERT_NE(nonObjectDescriptor, nullptr);
    component.ApplyPrivateAttributes(nonObjectDescriptor->GetRoot());
    EXPECT_EQ(component.GetSrcValueForTest(), "");
    EXPECT_EQ(component.GetAltValueForTest(), DEFAULT_IMAGE_PLACEHOLDER_ALT);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 2.0,
        "objectFit": "fill"
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 2.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::FILL));

    std::unique_ptr<JsonAdapter> emptyStyles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(emptyStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(emptyStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    std::unique_ptr<JsonAdapter> invalidTypeStyles = JsonAdapter::Parse(R"({
        "objectFit": true
    })");
    ASSERT_NE(invalidTypeStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidTypeStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    component.SetApplyingStyleDeltaUpdateForTest(true);
    component.ApplyComponentSpecificStylesForTest(invalidTypeStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 2.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
    component.SetApplyingStyleDeltaUpdateForTest(false);

    PropertyDeclaration declaration = component.GetPrivatePropertyDeclarationForTest("src");
    EXPECT_EQ(declaration.fallbackString, "");
    EXPECT_TRUE(static_cast<bool>(declaration.applyValue));
}

TEST_F(ExtendedImageComponentTest, L0_should_cover_image_invalid_numeric_aspect_ratio_and_missing_object_fit_defaults)
{
    TestableExtendedImageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 3.0,
        "objectFit": "fill"
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);

    std::unique_ptr<JsonAdapter> invalidNumericAspectRatio = JsonAdapter::Parse(R"({
        "aspectRatio": 0
    })");
    ASSERT_NE(invalidNumericAspectRatio, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidNumericAspectRatio->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    component.SetApplyingStyleDeltaUpdateForTest(true);
    component.ApplyComponentSpecificStylesForTest(invalidNumericAspectRatio->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::FILL));
    component.SetApplyingStyleDeltaUpdateForTest(false);

    std::unique_ptr<JsonAdapter> nanStyles = JsonAdapter::CreateObject();
    ASSERT_NE(nanStyles, nullptr);
    JsonValue nanRoot = nanStyles->GetRoot();
    ASSERT_TRUE(nanRoot.PutNumber("aspectRatio", std::numeric_limits<double>::quiet_NaN()));
    component.ApplyComponentSpecificStylesForTest(nanRoot, applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);

    std::unique_ptr<JsonAdapter> aspectOnlyStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 2.25
    })");
    ASSERT_NE(aspectOnlyStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(aspectOnlyStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 2.25F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
}

TEST_F(ExtendedImageComponentTest, L0_should_reject_string_typed_image_aspect_ratio_styles)
{
    TestableExtendedImageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> invalidTypedStyles = JsonAdapter::Parse(R"({
        "aspectRatio": "2.5",
        "objectFit": " fill "
    })");
    ASSERT_NE(invalidTypedStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidTypedStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::FILL));

    JsonValue invalidStyles;
    component.ApplyComponentSpecificStylesForTest(invalidStyles, applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    std::unique_ptr<JsonAdapter> invalidStringStyles = JsonAdapter::Parse(R"({
        "aspectRatio": "bad",
        "objectFit": " unsupported "
    })");
    ASSERT_NE(invalidStringStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidStringStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));
}

TEST_F(ExtendedImageComponentSchemaWarningTest,
    L0_should_fallback_image_invalid_private_fields_and_dispatch_schema_warnings)
{
    TestableExtendedImageComponent component;
    component.SetRenderId(1004);
    component.SetSurfaceId("surface-image-warning");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> invalidSrcDescriptor = JsonAdapter::Parse(R"({"src":true})");
    ASSERT_NE(invalidSrcDescriptor, nullptr);
    component.ApplyPrivateAttributes(invalidSrcDescriptor->GetRoot());
    EXPECT_EQ(component.GetSrcValueForTest(), "");
    EXPECT_EQ(component.GetAltValueForTest(), DEFAULT_IMAGE_PLACEHOLDER_ALT);

    std::unique_ptr<JsonAdapter> nullSrcDescriptor = JsonAdapter::Parse(R"({"src":null})");
    ASSERT_NE(nullSrcDescriptor, nullptr);
    component.ApplyPrivateAttributes(nullSrcDescriptor->GetRoot());
    EXPECT_EQ(component.GetSrcValueForTest(), "");
    EXPECT_EQ(component.GetAltValueForTest(), DEFAULT_IMAGE_PLACEHOLDER_ALT);

    std::unique_ptr<JsonAdapter> bindingObjectSrcDescriptor = JsonAdapter::Parse(R"({
        "src": {
            "path": "/image/src"
        }
    })");
    ASSERT_NE(bindingObjectSrcDescriptor, nullptr);
    component.ApplyPrivateAttributes(bindingObjectSrcDescriptor->GetRoot());
    EXPECT_EQ(component.GetSrcValueForTest(), "");
    EXPECT_EQ(component.GetAltValueForTest(), DEFAULT_IMAGE_PLACEHOLDER_ALT);
    const std::vector<DataBinding>& bindings = component.GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "src");
    EXPECT_EQ(bindings[0].dataPath_, "/image/src");
    EXPECT_EQ(bindings[0].type_, BindingType::PATH);

    std::unique_ptr<JsonAdapter> missingSrcDescriptor = JsonAdapter::Parse(R"({})");
    ASSERT_NE(missingSrcDescriptor, nullptr);
    component.ApplyPrivateAttributes(missingSrcDescriptor->GetRoot());
    EXPECT_EQ(component.GetSrcValueForTest(), "");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "aspectRatio": null,
        "objectFit": false
    })");
    ASSERT_NE(invalidStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "src"), 2U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "src"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.aspectRatio"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.objectFit"), 1U);
}

TEST_F(ExtendedImageComponentSchemaWarningTest,
    L0_should_dispatch_image_invalid_value_and_non_object_style_schema_warnings)
{
    TestableExtendedImageComponent component;
    component.SetRenderId(1005);
    component.SetSurfaceId("surface-image-invalid-value-warning");
    component.SetComponentId("root");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 2.5,
        "objectFit": "fill"
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 2.5F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::FILL));

    std::unique_ptr<JsonAdapter> invalidValueStyles = JsonAdapter::Parse(R"({
        "aspectRatio": 0,
        "objectFit": "unsupported"
    })");
    ASSERT_NE(invalidValueStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidValueStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::Parse("true");
    ASSERT_NE(nonObjectStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(nonObjectStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetAspectRatioForTest(), 1.0F);
    EXPECT_EQ(component.GetObjectFitForTest(), ToA2UIObjectFitValue(A2UIObjectFit::COVER));

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.aspectRatio"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.objectFit"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles"), 1U);
}

TEST_F(ExtendedImageComponentTest, L0_should_apply_reset_and_validate_static_image_fill_color)
{
    TestableExtendedImageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "fillColor": "#FF317AF7"
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    EXPECT_TRUE(component.HasFillColorForTest());
    EXPECT_EQ(component.GetFillColorForTest(), 0xFF317AF7U);
    ExpectU32Attribute(component.GetNativeView(), NODE_IMAGE_FILL_COLOR, 0xFF317AF7U);

    component.SetApplyingStyleDeltaUpdateForTest(true);
    std::unique_ptr<JsonAdapter> unrelatedDelta = JsonAdapter::Parse(R"({
        "objectFit": "contain"
    })");
    ASSERT_NE(unrelatedDelta, nullptr);
    component.ApplyComponentSpecificStylesForTest(unrelatedDelta->GetRoot(), applier);
    EXPECT_TRUE(component.HasFillColorForTest());
    EXPECT_EQ(component.GetFillColorForTest(), 0xFF317AF7U);

    std::unique_ptr<JsonAdapter> invalidDelta = JsonAdapter::Parse(R"({
        "fillColor": "blue"
    })");
    ASSERT_NE(invalidDelta, nullptr);
    component.ApplyComponentSpecificStylesForTest(invalidDelta->GetRoot(), applier);
    EXPECT_FALSE(component.HasFillColorForTest());

    component.SetApplyingStyleDeltaUpdateForTest(false);
    component.ApplyComponentSpecificStylesForTest(validStyles->GetRoot(), applier);
    ASSERT_TRUE(component.HasFillColorForTest());
    std::unique_ptr<JsonAdapter> emptyStyles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(emptyStyles, nullptr);
    component.ApplyComponentSpecificStylesForTest(emptyStyles->GetRoot(), applier);
    EXPECT_FALSE(component.HasFillColorForTest());
}

} // namespace
