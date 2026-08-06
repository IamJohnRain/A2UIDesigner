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
#include <string>

#include "styles/StyleParser.h"
#include "styles/StyleTypes.h"
#include "utils/JsonAdapter.h"

using namespace NativeModule;

// =============================================================================
// These TDD cases pin down the alias-removal refactor in StyleParser:
//   * Directional padding/margin aliases (paddingTop/Right/Bottom/Left,
//     marginTop/Right/Bottom/Left) are no longer registered in the name map.
//   * Lowercase background variants (backgroundimage, backgroundimageSize,
//     backgroundimageSizeWithStyle) are no longer registered.
//   * The dedicated backgroundImageSize branch in ToPropertyName was removed, so
//     backgroundImageSize now resolves to UNKNOWN. The resolver still applies it
//     through its raw JSON key (ExtendedStyleResolver::ApplyBackgroundImageSize),
//     so the property keeps working at runtime despite the name mapping change.
//
// Branch coverage of StyleParser::ToPropertyName is fully exercised:
//   - map-hit (iter != end) true  -> canonical keys
//   - map-hit false, linearGradient true
//   - map-hit false, linearGradient false -> UNKNOWN fallthrough
// =============================================================================

namespace {
const StyleProperty* FindProperty(const StyleParseResult& result, const std::string& rawName)
{
    for (const auto& property : result.properties) {
        if (property.rawName == rawName) {
            return &property;
        }
    }
    return nullptr;
}
} // namespace

/**
 * @tc.name: StyleParserPropertyNameTddTest001
 * @tc.desc: Every canonical key still registered in the name map resolves to its enum.
 * @tc.type: FUNC
 */
TEST(StyleParserPropertyNameTddTest, StyleParserPropertyNameTddTest001)
{
    EXPECT_EQ(StyleParser::ToPropertyName("width"), StylePropertyName::WIDTH);
    EXPECT_EQ(StyleParser::ToPropertyName("height"), StylePropertyName::HEIGHT);
    EXPECT_EQ(StyleParser::ToPropertyName("padding"), StylePropertyName::PADDING);
    EXPECT_EQ(StyleParser::ToPropertyName("margin"), StylePropertyName::MARGIN);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundColor"), StylePropertyName::BACKGROUND_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("borderRadius"), StylePropertyName::BORDER_RADIUS);
    EXPECT_EQ(StyleParser::ToPropertyName("borderWidth"), StylePropertyName::BORDER_WIDTH);
    EXPECT_EQ(StyleParser::ToPropertyName("borderColor"), StylePropertyName::BORDER_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("fontColor"), StylePropertyName::FONT_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("fontSize"), StylePropertyName::FONT_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("fontWeight"), StylePropertyName::FONT_WEIGHT);
    EXPECT_EQ(StyleParser::ToPropertyName("textAlign"), StylePropertyName::TEXT_ALIGN);
    EXPECT_EQ(StyleParser::ToPropertyName("maxLines"), StylePropertyName::MAX_LINES);
    EXPECT_EQ(StyleParser::ToPropertyName("minFontSize"), StylePropertyName::TEXT_MIN_FONT_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("maxFontSize"), StylePropertyName::TEXT_MAX_FONT_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("textOverflow"), StylePropertyName::TEXT_OVERFLOW);
    EXPECT_EQ(StyleParser::ToPropertyName("wordBreak"), StylePropertyName::WORD_BREAK);
    EXPECT_EQ(StyleParser::ToPropertyName("decoration"), StylePropertyName::DECORATION);
    EXPECT_EQ(StyleParser::ToPropertyName("placeholderColor"), StylePropertyName::PLACEHOLDER_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("caretColor"), StylePropertyName::CARET_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("showUnderline"), StylePropertyName::SHOW_UNDERLINE);
    EXPECT_EQ(StyleParser::ToPropertyName("visibility"), StylePropertyName::VISIBILITY);
    EXPECT_EQ(StyleParser::ToPropertyName("opacity"), StylePropertyName::OPACITY);
    EXPECT_EQ(StyleParser::ToPropertyName("shadow"), StylePropertyName::SHADOW);
    EXPECT_EQ(StyleParser::ToPropertyName("flexShrink"), StylePropertyName::FLEX_SHRINK);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImage"), StylePropertyName::BACKGROUND_IMAGE);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImageSizeWithStyle"), StylePropertyName::BACKGROUND_IMAGE_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("clip"), StylePropertyName::CLIP);
    EXPECT_EQ(StyleParser::ToPropertyName("layoutWeight"), StylePropertyName::LAYOUT_WEIGHT);
    EXPECT_EQ(StyleParser::ToPropertyName("constraintSize"), StylePropertyName::CONSTRAINT_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("aspectRatio"), StylePropertyName::ASPECT_RATIO);
}

/**
 * @tc.name: StyleParserPropertyNameTddTest002
 * @tc.desc: linearGradient resolves through its dedicated branch (not via the name map).
 * @tc.type: FUNC
 */
TEST(StyleParserPropertyNameTddTest, StyleParserPropertyNameTddTest002)
{
    EXPECT_EQ(StyleParser::ToPropertyName("linearGradient"), StylePropertyName::LINEAR_GRADIENT);
    // Case-sensitive: the lowercase variant is not an alias and must fall through to UNKNOWN.
    EXPECT_EQ(StyleParser::ToPropertyName("lineargradient"), StylePropertyName::UNKNOWN);
}

/**
 * @tc.name: StyleParserPropertyNameTddTest003
 * @tc.desc: Removed directional padding aliases no longer map to PADDING.
 * @tc.type: FUNC
 */
TEST(StyleParserPropertyNameTddTest, StyleParserPropertyNameTddTest003)
{
    EXPECT_EQ(StyleParser::ToPropertyName("paddingTop"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("paddingRight"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("paddingBottom"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("paddingLeft"), StylePropertyName::UNKNOWN);
    // The aggregate key is unaffected and still resolves.
    EXPECT_EQ(StyleParser::ToPropertyName("padding"), StylePropertyName::PADDING);
}

/**
 * @tc.name: StyleParserPropertyNameTddTest004
 * @tc.desc: Removed directional margin aliases no longer map to MARGIN.
 * @tc.type: FUNC
 */
TEST(StyleParserPropertyNameTddTest, StyleParserPropertyNameTddTest004)
{
    EXPECT_EQ(StyleParser::ToPropertyName("marginTop"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("marginRight"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("marginBottom"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("marginLeft"), StylePropertyName::UNKNOWN);
    // The aggregate key is unaffected and still resolves.
    EXPECT_EQ(StyleParser::ToPropertyName("margin"), StylePropertyName::MARGIN);
}

/**
 * @tc.name: StyleParserPropertyNameTddTest005
 * @tc.desc: Removed lowercase background aliases and the bare backgroundImageSize key resolve to UNKNOWN.
 * @tc.type: FUNC
 */
TEST(StyleParserPropertyNameTddTest, StyleParserPropertyNameTddTest005)
{
    // backgroundImageSize lost its dedicated branch; it is not in the name map either.
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImageSize"), StylePropertyName::UNKNOWN);
    // Lowercase background variants are no longer registered as case-insensitive aliases.
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundimage"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundimageSize"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundimageSizeWithStyle"), StylePropertyName::UNKNOWN);
    // The camelCase canonical keys remain valid.
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImage"), StylePropertyName::BACKGROUND_IMAGE);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImageSizeWithStyle"), StylePropertyName::BACKGROUND_IMAGE_SIZE);
}

/**
 * @tc.name: StyleParserPropertyNameTddTest006
 * @tc.desc: Arbitrary unrecognized keys resolve to UNKNOWN.
 * @tc.type: FUNC
 */
TEST(StyleParserPropertyNameTddTest, StyleParserPropertyNameTddTest006)
{
    EXPECT_EQ(StyleParser::ToPropertyName(""), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("notARealStyle"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("BackgroundColor"), StylePropertyName::UNKNOWN);
}

/**
 * @tc.name: StyleParserPropertyNameTddTest007
 * @tc.desc: Parse surfaces the alias removal end-to-end — canonical keys resolve while removed aliases become UNKNOWN.
 * @tc.type: FUNC
 */
TEST(StyleParserPropertyNameTddTest, StyleParserPropertyNameTddTest007)
{
    auto adapter = JsonAdapter::Parse(R"({
        "width": 100,
        "padding": 10,
        "paddingTop": 5,
        "margin": 8,
        "marginTop": 4,
        "backgroundImage": "bg.png",
        "backgroundimage": "bg2.png",
        "backgroundImageSize": { "width": 10 },
        "backgroundImageSizeWithStyle": 1,
        "backgroundimageSizeWithStyle": 1,
        "totallyUnknown": 0
    })");
    ASSERT_NE(adapter, nullptr);

    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.properties.size(), 11u);

    // Canonical keys keep their resolved property names.
    ASSERT_NE(FindProperty(result, "width"), nullptr);
    EXPECT_EQ(FindProperty(result, "width")->name, StylePropertyName::WIDTH);
    ASSERT_NE(FindProperty(result, "padding"), nullptr);
    EXPECT_EQ(FindProperty(result, "padding")->name, StylePropertyName::PADDING);
    ASSERT_NE(FindProperty(result, "margin"), nullptr);
    EXPECT_EQ(FindProperty(result, "margin")->name, StylePropertyName::MARGIN);
    ASSERT_NE(FindProperty(result, "backgroundImage"), nullptr);
    EXPECT_EQ(FindProperty(result, "backgroundImage")->name, StylePropertyName::BACKGROUND_IMAGE);
    ASSERT_NE(FindProperty(result, "backgroundImageSizeWithStyle"), nullptr);
    EXPECT_EQ(FindProperty(result, "backgroundImageSizeWithStyle")->name, StylePropertyName::BACKGROUND_IMAGE_SIZE);

    // Removed aliases are now parsed as UNKNOWN-named properties.
    ASSERT_NE(FindProperty(result, "paddingTop"), nullptr);
    EXPECT_EQ(FindProperty(result, "paddingTop")->name, StylePropertyName::UNKNOWN);
    ASSERT_NE(FindProperty(result, "marginTop"), nullptr);
    EXPECT_EQ(FindProperty(result, "marginTop")->name, StylePropertyName::UNKNOWN);
    ASSERT_NE(FindProperty(result, "backgroundimage"), nullptr);
    EXPECT_EQ(FindProperty(result, "backgroundimage")->name, StylePropertyName::UNKNOWN);
    ASSERT_NE(FindProperty(result, "backgroundImageSize"), nullptr);
    EXPECT_EQ(FindProperty(result, "backgroundImageSize")->name, StylePropertyName::UNKNOWN);
    ASSERT_NE(FindProperty(result, "backgroundimageSizeWithStyle"), nullptr);
    EXPECT_EQ(FindProperty(result, "backgroundimageSizeWithStyle")->name, StylePropertyName::UNKNOWN);
    ASSERT_NE(FindProperty(result, "totallyUnknown"), nullptr);
    EXPECT_EQ(FindProperty(result, "totallyUnknown")->name, StylePropertyName::UNKNOWN);
}
