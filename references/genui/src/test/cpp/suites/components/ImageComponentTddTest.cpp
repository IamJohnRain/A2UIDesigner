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

#include "components/A2UI/image/ImageComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

class ImageComponentTddTest : public A2UIComponentTddTest {};

namespace {
constexpr char DEFAULT_IMAGE_PLACEHOLDER_ALT[] = "resources/base/media/placeHolder_E5E5EA.png";
}

TEST_F(ImageComponentTddTest, L0_image_should_create_image_node_and_report_type)
{
    auto image = std::make_shared<ImageComponent>();
    ASSERT_NE(image, nullptr);

    EXPECT_EQ(image->GetType(), "Image");
    EXPECT_EQ(image->GetNativeView(), FindCreatedNode(ARKUI_NODE_IMAGE));
}

TEST_F(ImageComponentTddTest, L0_image_should_apply_source_alt_fit_and_avatar_variant)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor = ParseJson(R"({"id":"avatar","component":"Image","url":"res://avatar.png",)"
                                R"("description":"Profile photo","fit":"cover","variant":"avatar"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle imageNode = image->GetNativeView();
    EXPECT_TRUE(HasResetAttributeCall(imageNode, NODE_IMAGE_SRC));
    ExpectStringAttribute(imageNode, NODE_IMAGE_SRC, "res://avatar.png");
    ExpectStringAttribute(imageNode, NODE_IMAGE_ALT, "Profile photo");
    ExpectI32Attribute(imageNode, NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_COVER);
    ExpectF32Attribute(imageNode, NODE_WIDTH, 32.0F);
    ExpectF32Attribute(imageNode, NODE_HEIGHT, 32.0F);
    ExpectF32Attribute(imageNode, NODE_BORDER_RADIUS, 16.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_fallback_invalid_fit_and_variant)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor =
        ParseJson(R"({"id":"image","component":"Image","url":"res://x.png","fit":"bad","variant":"bad"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_FILL);
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH, 150.0F);
    ExpectF32Attribute(image->GetNativeView(), NODE_HEIGHT, 150.0F);
    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_ALT, DEFAULT_IMAGE_PLACEHOLDER_ALT);
}

TEST_F(ImageComponentTddTest, L0_image_should_reset_source_and_apply_description_when_url_is_missing)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor = ParseJson(R"({"id":"image","component":"Image","description":"Fallback description",)"
                                R"("variant":"mediumFeature"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_ALT, "Fallback description");
    EXPECT_TRUE(HasResetAttributeCall(image->GetNativeView(), NODE_IMAGE_SRC));
    EXPECT_EQ(CountAttributeCall(image->GetNativeView(), NODE_IMAGE_SRC), 0);
}

TEST_F(ImageComponentTddTest, L0_image_should_apply_public_source_fit_and_alt_setters)
{
    auto image = std::make_shared<ImageComponent>();

    image->SetSrc("");
    image->SetSrc("res://manual.png");
    image->SetObjectFit(A2UIObjectFit::CONTAIN);
    image->SetAlt("");
    image->SetAlt("Manual image");

    EXPECT_TRUE(HasResetAttributeCall(image->GetNativeView(), NODE_IMAGE_SRC));
    EXPECT_TRUE(HasResetAttributeCall(image->GetNativeView(), NODE_IMAGE_ALT));
    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_SRC, "res://manual.png");
    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_CONTAIN);
    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_ALT, "Manual image");
}

TEST_F(ImageComponentTddTest, L0_image_should_default_header_variant_fit_to_contain_and_apply_width_percent)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor = ParseJson(R"({"id":"hero","component":"Image","url":"res://hero.png","variant":"header"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_CONTAIN);
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH_PERCENT, 1.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_keep_explicit_fill_when_header_variant_sets_fit)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor =
        ParseJson(R"({"id":"hero","component":"Image","url":"res://hero.png","variant":"header","fit":"fill"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_FILL);
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH_PERCENT, 1.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_fallback_invalid_fit_to_contain_when_header_variant_is_used)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor =
        ParseJson(R"({"id":"hero","component":"Image","url":"res://hero.png","variant":"header","fit":"bad"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_CONTAIN);
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH_PERCENT, 1.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_resolve_header_fit_fallback_when_fit_field_appears_before_variant)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor =
        ParseJson(R"({"id":"hero","component":"Image","url":"res://hero.png","fit":"bad","variant":"header"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_CONTAIN);
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH_PERCENT, 1.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_fallback_invalid_fit_to_fill_when_variant_is_missing_after_header_case)
{
    auto headerImage = std::make_shared<ImageComponent>();
    auto headerDescriptor =
        ParseJson(R"({"id":"hero","component":"Image","url":"res://hero.png","variant":"header","fit":"bad"})");
    ASSERT_NE(headerDescriptor, nullptr);

    headerImage->ApplyDescriptor(headerDescriptor->GetRoot());
    ExpectI32Attribute(headerImage->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_CONTAIN);

    auto defaultImage = std::make_shared<ImageComponent>();
    auto defaultDescriptor = ParseJson(R"({"id":"image","component":"Image","url":"res://x.png","fit":"bad"})");
    ASSERT_NE(defaultDescriptor, nullptr);

    defaultImage->ApplyDescriptor(defaultDescriptor->GetRoot());

    ExpectI32Attribute(defaultImage->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_FILL);
    ExpectF32Attribute(defaultImage->GetNativeView(), NODE_WIDTH, 150.0F);
    ExpectF32Attribute(defaultImage->GetNativeView(), NODE_HEIGHT, 150.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_fallback_invalid_fit_to_fill_when_variant_is_not_string)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor = ParseJson(R"({"id":"image","component":"Image","url":"res://x.png","fit":"bad","variant":123})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(image->GetNativeView(), NODE_IMAGE_OBJECT_FIT, ARKUI_OBJECT_FIT_FILL);
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH, 150.0F);
    ExpectF32Attribute(image->GetNativeView(), NODE_HEIGHT, 150.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_apply_icon_small_and_large_variant_presets)
{
    auto image = std::make_shared<ImageComponent>();
    auto iconDescriptor = ParseJson(R"({"id":"icon","component":"Image","variant":"icon"})");
    ASSERT_NE(iconDescriptor, nullptr);
    auto smallDescriptor = ParseJson(R"({"id":"small","component":"Image","variant":"smallFeature"})");
    ASSERT_NE(smallDescriptor, nullptr);
    auto largeDescriptor = ParseJson(R"({"id":"large","component":"Image","variant":"largeFeature"})");
    ASSERT_NE(largeDescriptor, nullptr);

    image->ApplyDescriptor(iconDescriptor->GetRoot());
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH, 32.0F);
    ExpectF32Attribute(image->GetNativeView(), NODE_HEIGHT, 32.0F);

    image->ApplyDescriptor(smallDescriptor->GetRoot());
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH, 50.0F);
    ExpectF32Attribute(image->GetNativeView(), NODE_HEIGHT, 50.0F);

    image->ApplyDescriptor(largeDescriptor->GetRoot());
    ExpectF32Attribute(image->GetNativeView(), NODE_WIDTH, 400.0F);
    ExpectF32Attribute(image->GetNativeView(), NODE_HEIGHT, 400.0F);
}

TEST_F(ImageComponentTddTest, L0_image_should_return_theme_when_surface_context_is_available)
{
    auto image = std::make_shared<ImageComponent>();
    PrepareThemeContext(*image);

    EXPECT_NE(image->GetTheme(), nullptr);
}

TEST_F(ImageComponentTddTest, L0_image_should_apply_default_placeholder_when_alt_and_description_are_missing)
{
    auto image = std::make_shared<ImageComponent>();
    auto descriptor = ParseJson(
        R"({"id":"image","component":"Image","url":"resources/base/media/startIcon.png","variant":"mediumFeature"})");
    ASSERT_NE(descriptor, nullptr);

    image->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(image->GetNativeView(), NODE_IMAGE_ALT, DEFAULT_IMAGE_PLACEHOLDER_ALT);
}
