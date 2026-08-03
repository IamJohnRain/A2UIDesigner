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

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#define private public
#define protected public
#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedCommonTheme.h"
#include "components/extended/ExtendedComponent.h"
#include "components/extended/ExtendedDividerTheme.h"
#include "components/extended/ExtendedGridTheme.h"
#include "components/extended/ExtendedListTheme.h"
#include "components/extended/ExtendedProgressTheme.h"
#include "components/extended/ExtendedStyleResolver.h"
#include "components/extended/ExtendedTextTheme.h"
#include "styles/StyleApplyUtils.h"
#include "styles/StyleTypes.h"
#include "theme/ThemeBase.h"
#include "theme/ThemeManager.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "RenderSlot.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"
#undef protected
#undef private

using namespace NativeModule;

namespace {

// Brand color used to make the theme-aware shadow distinguishable from the default 0xFF000000.
constexpr uint32_t TEST_BRAND_COLOR = 0xFFAA1122u;
// A user-supplied shadow color, distinct from both the default and the brand color.
constexpr uint32_t TEST_USER_SHADOW_COLOR = 0xFF334455u;
// Default shadow color baked into StyleShadow / ExtendedCommonTheme (light == dark == 0xFF000000).
constexpr uint32_t DEFAULT_SHADOW_COLOR = 0xFF000000u;

// Helper: create an ArkUINodeApiAdapter wired to a dummy node (mirrors ExtendedStyleResolverTddTest).
ArkUINodeApiAdapter MakeTestApplier(ArkUI_NodeHandle node)
{
    return ArkUINodeApiAdapter([node]() { return node; }, []() { return std::string("shadow-theme-comp"); },
        ArkUINodeApiAdapter::EdgeSetter(), []() {}, [](const std::function<void()>&) {});
}

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog(std::initializer_list<const char*> componentNames)
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* componentName : componentNames) {
        if (componentName == nullptr || componentName[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

const MockArkUINativeProvider::SetAttributeRecord* FindLastAttributeForNode(
    const MockArkUINativeProvider& provider, ArkUI_NodeHandle node, int32_t attribute)
{
    for (auto it = provider.setAttributeRecords_.rbegin(); it != provider.setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle == node && it->attribute == attribute) {
            return &(*it);
        }
    }
    return nullptr;
}

// Build an ExtendedCommonTheme whose context already carries a brand color.
std::shared_ptr<ExtendedCommonTheme> MakeBrandCommonTheme(uint32_t brandColor)
{
    ThemeContext context;
    context.hasBrandColor = true;
    context.brandColor = brandColor;
    return std::make_shared<ExtendedCommonTheme>(context);
}

class ExtendedShadowThemeTddTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-shadow-theme");
        slot_.SetRenderId(1);
        slot_.InitializeThemeManager(ThemeContext {});
    }

    SurfaceSlot slot_;
    ArkUI_NodeHandle testNode_ = reinterpret_cast<ArkUI_NodeHandle>(0xA510);
};

// =============================================================================
// Group A — ExtendedCommonTheme::GetShadowColor / OnConfigChange
// Covers every branch of the new shadow-color resolution logic.
// =============================================================================

TEST_F(ExtendedShadowThemeTddTest, GetShadowColor_ReturnsBrandColorWhenBrandColorSet)
{
    ThemeContext context;
    context.hasBrandColor = true;
    context.brandColor = TEST_BRAND_COLOR;
    ExtendedCommonTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, GetShadowColor_ReturnsDarkDefaultInDarkModeWithoutBrand)
{
    ThemeContext context;
    context.colorMode = ThemeMode::DARK;
    context.hasBrandColor = false;
    ExtendedCommonTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), DEFAULT_SHADOW_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, GetShadowColor_ReturnsLightDefaultInLightModeWithoutBrand)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    context.hasBrandColor = false;
    ExtendedCommonTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), DEFAULT_SHADOW_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, Constructor_DefaultsToLightModeShadowColor)
{
    // Default-constructed ThemeContext is LIGHT with no brand color.
    ExtendedCommonTheme theme(ThemeContext {});
    EXPECT_EQ(theme.GetShadowColor(), DEFAULT_SHADOW_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, OnConfigChange_UpdatesContextSoShadowColorFollowsBrand)
{
    ExtendedCommonTheme theme(ThemeContext {});
    EXPECT_EQ(theme.GetShadowColor(), DEFAULT_SHADOW_COLOR);

    ThemeContext branded;
    branded.hasBrandColor = true;
    branded.brandColor = TEST_BRAND_COLOR;
    theme.OnConfigChange(branded);

    // After re-configuring with a brand color, the shadow color must follow it.
    EXPECT_EQ(theme.GetShadowColor(), TEST_BRAND_COLOR);
}

// =============================================================================
// Group A2 — Theme subclasses inherit GetShadowColor via ExtendedCommonTheme
// The diff reparented Divider/Grid/List/Progress/Text themes from ThemeBase onto
// ExtendedCommonTheme. These tests prove each subclass inherits the new behavior.
// =============================================================================

TEST_F(ExtendedShadowThemeTddTest, ExtendedDividerTheme_InheritsShadowColorFromCommonBase)
{
    ThemeContext context;
    context.hasBrandColor = true;
    context.brandColor = TEST_BRAND_COLOR;
    ExtendedDividerTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ExtendedGridTheme_InheritsShadowColorFromCommonBase)
{
    ThemeContext context;
    context.hasBrandColor = true;
    context.brandColor = TEST_BRAND_COLOR;
    ExtendedGridTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ExtendedListTheme_InheritsShadowColorFromCommonBase)
{
    ThemeContext context;
    context.hasBrandColor = true;
    context.brandColor = TEST_BRAND_COLOR;
    ExtendedListTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ExtendedProgressTheme_InheritsShadowColorFromCommonBase)
{
    ThemeContext context;
    context.hasBrandColor = true;
    context.brandColor = TEST_BRAND_COLOR;
    ExtendedProgressTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ExtendedTextTheme_InheritsShadowColorFromCommonBase)
{
    ThemeContext context;
    context.hasBrandColor = true;
    context.brandColor = TEST_BRAND_COLOR;
    ExtendedTextTheme theme(context);
    EXPECT_EQ(theme.GetShadowColor(), TEST_BRAND_COLOR);
}

// =============================================================================
// Group B — StyleApplyUtils::ParseShadow sets the new hasColor flag
// =============================================================================

TEST_F(ExtendedShadowThemeTddTest, ParseShadow_SetsHasColorTrueWhenColorProvided)
{
    auto value = JsonAdapter::Parse(R"({"radius": 10, "color": "#FF334455", "offsetX": 2, "offsetY": 4})");
    ASSERT_NE(value, nullptr);
    StyleShadow shadow;
    ASSERT_TRUE(StyleApplyUtils::ParseShadow(value->GetRoot(), shadow));
    EXPECT_TRUE(shadow.hasColor);
    EXPECT_EQ(shadow.color, TEST_USER_SHADOW_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ParseShadow_KeepsHasColorFalseWhenColorAbsent)
{
    auto value = JsonAdapter::Parse(R"({"radius": 10, "offsetX": 2, "offsetY": 4})");
    ASSERT_NE(value, nullptr);
    StyleShadow shadow;
    ASSERT_TRUE(StyleApplyUtils::ParseShadow(value->GetRoot(), shadow));
    EXPECT_FALSE(shadow.hasColor);
}

// =============================================================================
// Group C — ExtendedStyleResolver::ApplyShadow theme-aware color branch
// Exercises all combinations of (hasColor, themeManager) for the custom shadow path.
// =============================================================================

TEST_F(ExtendedShadowThemeTddTest, ApplyShadow_UsesBrandColorWhenNoColorAndBrandThemeManager)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto shadowValue = JsonAdapter::Parse(R"({"radius": 10, "offsetX": 2, "offsetY": 4})");
    ASSERT_NE(shadowValue, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    auto commonTheme = MakeBrandCommonTheme(TEST_BRAND_COLOR);

    ExtendedStyleResolver::ApplyShadow(shadowValue->GetRoot(), applier, issues, commonTheme);

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ApplyShadow_UsesDefaultColorWhenNoColorAndDarkThemeManager)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto shadowValue = JsonAdapter::Parse(R"({"radius": 10, "offsetX": 2, "offsetY": 4})");
    ASSERT_NE(shadowValue, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ThemeContext context;
    context.colorMode = ThemeMode::DARK;
    auto commonTheme = std::make_shared<ExtendedCommonTheme>(context);

    ExtendedStyleResolver::ApplyShadow(shadowValue->GetRoot(), applier, issues, commonTheme);

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, DEFAULT_SHADOW_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ApplyShadow_UsesDefaultColorWhenNoColorAndLightThemeManager)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto shadowValue = JsonAdapter::Parse(R"({"radius": 10, "offsetX": 2, "offsetY": 4})");
    ASSERT_NE(shadowValue, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    auto commonTheme = std::make_shared<ExtendedCommonTheme>(context);

    ExtendedStyleResolver::ApplyShadow(shadowValue->GetRoot(), applier, issues, commonTheme);

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, DEFAULT_SHADOW_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ApplyShadow_KeepsDefaultColorWhenNoColorAndNoThemeManager)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto shadowValue = JsonAdapter::Parse(R"({"radius": 10, "offsetX": 2, "offsetY": 4})");
    ASSERT_NE(shadowValue, nullptr);
    std::vector<DescriptorValidationIssue> issues;

    // No theme manager → theme-aware branch skipped, default StyleShadow color used.
    ExtendedStyleResolver::ApplyShadow(shadowValue->GetRoot(), applier, issues, nullptr);

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, DEFAULT_SHADOW_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ApplyShadow_KeepsUserColorWhenColorProvidedEvenWithThemeManager)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto shadowValue = JsonAdapter::Parse(R"({"radius": 10, "color": "#FF334455", "offsetX": 2, "offsetY": 4})");
    ASSERT_NE(shadowValue, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    auto commonTheme = MakeBrandCommonTheme(TEST_BRAND_COLOR);

    ExtendedStyleResolver::ApplyShadow(shadowValue->GetRoot(), applier, issues, commonTheme);

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    // Explicit user color wins over the theme brand color.
    EXPECT_EQ(rec->values[5].u32, TEST_USER_SHADOW_COLOR);
    EXPECT_NE(rec->values[5].u32, TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ApplyShadow_StyleShadowSkipsThemeColorPath)
{
    // A style shadow (string token) takes the early-return path and must never consult the theme.
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto shadowValue = JsonAdapter::Parse(R"("OUTER_DEFAULT_MD")");
    ASSERT_NE(shadowValue, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    auto commonTheme = MakeBrandCommonTheme(TEST_BRAND_COLOR);

    ExtendedStyleResolver::ApplyShadow(shadowValue->GetRoot(), applier, issues, commonTheme);

    // Style shadows are applied via NODE_SHADOW, never NODE_CUSTOM_SHADOW.
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW), nullptr);
    EXPECT_NE(FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_SHADOW), nullptr);
}

TEST_F(ExtendedShadowThemeTddTest, ApplyShadow_InvalidShadowPushesIssueAndResets)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto shadowValue = JsonAdapter::Parse(R"({"radius": "not-a-number"})");
    ASSERT_NE(shadowValue, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    auto commonTheme = MakeBrandCommonTheme(TEST_BRAND_COLOR);

    ExtendedStyleResolver::ApplyShadow(shadowValue->GetRoot(), applier, issues, commonTheme);

    // Invalid input → validation issue pushed, nothing applied.
    EXPECT_FALSE(issues.empty());
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW), nullptr);
}

TEST_F(ExtendedShadowThemeTddTest, ResolveAndApply_ThreadsThemeManagerToShadowViaDispatchContext)
{
    // Integration path: ResolveAndApply -> ApplyDecorationStyles -> ApplyShadow, where the
    // new ternary `dispatchContext.has_value() ? dispatchContext->themeManager : nullptr`
    // forwards a brand-color theme manager.
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"shadow": {"radius": 10, "offsetX": 2, "offsetY": 4}})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "shadow-theme-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    dispatchCtx.commonTheme = MakeBrandCommonTheme(TEST_BRAND_COLOR);

    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ResolveAndApply_ShadowWithoutDispatchContextUsesDefaultColor)
{
    // No dispatch context -> ApplyDecorationStyles forwards nullptr theme manager ->
    // theme-aware branch skipped, default StyleShadow color used.
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"shadow": {"radius": 10, "offsetX": 2, "offsetY": 4}})");
    ASSERT_NE(styles, nullptr);

    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, testNode_, NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, DEFAULT_SHADOW_COLOR);
}

// =============================================================================
// Group D — ExtendedComponent shadow caching + OnConfigChange re-application
// =============================================================================

TEST_F(ExtendedShadowThemeTddTest, ApplyResolvedStyles_CachesShadowWhenPresentInStyles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Column",
            "styles": {
                "shadow": { "radius": 10, "offsetX": 2, "offsetY": 4 }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto root = std::dynamic_pointer_cast<ExtendedComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->hasCachedShadow_);
    EXPECT_TRUE(root->cachedShadowValue_.IsValid());
}

TEST_F(ExtendedShadowThemeTddTest, ApplyResolvedStyles_DoesNotCacheShadowWhenAbsentFromStyles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Column",
            "styles": { "opacity": 1 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto root = std::dynamic_pointer_cast<ExtendedComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    EXPECT_FALSE(root->hasCachedShadow_);
}

TEST_F(ExtendedShadowThemeTddTest, OnConfigChange_ReappliesCachedShadowWithThemeBrandColor)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Column",
            "styles": {
                "shadow": { "radius": 10, "offsetX": 2, "offsetY": 4 }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto root = std::dynamic_pointer_cast<ExtendedComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->hasCachedShadow_);

    // Seed the component's own common theme with a brand color so the re-applied
    // shadow is distinguishable. OnConfigChange reads the color via GetCommonTheme().
    auto brandTheme = MakeBrandCommonTheme(TEST_BRAND_COLOR);
    root->cachedCommonTheme_ = brandTheme;

    mockArkUIPtr_->setAttributeRecords_.clear();
    root->OnConfigChange(ThemeContext {});

    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, root->GetNativeView(), NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, OnConfigChange_IsNoOpWhenNoShadowCached)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Column",
            "styles": { "opacity": 1 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto root = std::dynamic_pointer_cast<ExtendedComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    ASSERT_FALSE(root->hasCachedShadow_);

    mockArkUIPtr_->setAttributeRecords_.clear();
    // No cached shadow → early return, no crash, no shadow attribute recorded.
    ThemeContext context;
    context.colorMode = ThemeMode::DARK;
    root->OnConfigChange(context);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, root->GetNativeView(), NODE_CUSTOM_SHADOW), nullptr);
}

TEST_F(ExtendedShadowThemeTddTest, OnConfigChange_IsNoOpWhenNodeApplierIsNull)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Column",
            "styles": {
                "shadow": { "radius": 10, "offsetX": 2, "offsetY": 4 }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto root = std::dynamic_pointer_cast<ExtendedComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->hasCachedShadow_);
    ASSERT_NE(root->nodeApplier_, nullptr);

    // Force the null-applier branch while a shadow is cached. Restore afterwards so slot
    // teardown is unaffected.
    auto savedApplier = root->nodeApplier_;
    root->nodeApplier_ = nullptr;
    mockArkUIPtr_->setAttributeRecords_.clear();
    ThemeContext context;
    context.colorMode = ThemeMode::DARK;
    root->OnConfigChange(context);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, root->GetNativeView(), NODE_CUSTOM_SHADOW), nullptr);
    root->nodeApplier_ = savedApplier;
}

TEST_F(ExtendedShadowThemeTddTest, ApplySingleResolvedStyle_CachesShadowWhenStyleNameIsShadow)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Column"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto root = std::dynamic_pointer_cast<ExtendedComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    ASSERT_FALSE(root->hasCachedShadow_);

    auto shadowValue = JsonAdapter::Parse(R"({"radius": 12, "offsetX": 3, "offsetY": 5})");
    ASSERT_NE(shadowValue, nullptr);
    // ApplySingleResolvedStyle manages the delta-update flag internally.
    root->ApplySingleResolvedStyle("shadow", shadowValue->GetRoot());

    EXPECT_TRUE(root->hasCachedShadow_);
    EXPECT_TRUE(root->cachedShadowValue_.IsValid());

    // And once cached, a subsequent config change re-applies it using the component's
    // common theme color. Seed a brand color so the re-applied shadow is distinguishable.
    auto brandTheme = MakeBrandCommonTheme(TEST_BRAND_COLOR);
    root->cachedCommonTheme_ = brandTheme;
    mockArkUIPtr_->setAttributeRecords_.clear();
    root->OnConfigChange(ThemeContext {});
    const auto* rec = FindLastAttributeForNode(*mockArkUIPtr_, root->GetNativeView(), NODE_CUSTOM_SHADOW);
    ASSERT_NE(rec, nullptr);
    ASSERT_GE(rec->values.size(), 6U);
    EXPECT_EQ(rec->values[5].u32, TEST_BRAND_COLOR);
}

TEST_F(ExtendedShadowThemeTddTest, ApplySingleResolvedStyle_DoesNotCacheWhenStyleNameIsNotShadow)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Column"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto root = std::dynamic_pointer_cast<ExtendedComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    ASSERT_FALSE(root->hasCachedShadow_);

    auto opacityValue = JsonAdapter::Parse(R"(0.5)");
    ASSERT_NE(opacityValue, nullptr);
    root->ApplySingleResolvedStyle("opacity", opacityValue->GetRoot());

    // A non-shadow style must not populate the shadow cache.
    EXPECT_FALSE(root->hasCachedShadow_);
}

} // namespace
