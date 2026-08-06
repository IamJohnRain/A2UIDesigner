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
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedTextComponent.h"
#include "utils/JsonAdapter.h"

#include "A2UIComponentTddTestHelper.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedTextCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Text", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

class ExtendedTextAliasTest : public A2UIComponentTddTest {
protected:
    void SetUp() override
    {
        A2UIComponentTddTest::SetUp();
        slot_.SetSurfaceId("surface-extended-text-alias");
        slot_.SetRenderId(912101);
        slot_.SetCatalog(BuildExtendedTextCatalog());
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(slot_.GetRenderId());
        A2UIComponentTddTest::TearDown();
    }

    std::shared_ptr<ExtendedTextComponent> GetTextComponent()
    {
        return std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedTextAliasTest, should_accept_text_alias_for_literal)
{
    std::unique_ptr<JsonAdapter> literalMessage = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "text": "literal alias"
        }]
    })");
    ASSERT_NE(literalMessage, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(literalMessage->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text = GetTextComponent();
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetTextValueForTest(), "literal alias");
}

TEST_F(ExtendedTextAliasTest, should_route_text_alias_binding_updates_to_canonical_content_property)
{
    std::unique_ptr<JsonAdapter> initialData = JsonAdapter::Parse(R"({
        "path": "/label",
        "value": "first"
    })");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(initialData->GetRoot()));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "text": { "path": "/label" }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text = GetTextComponent();
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetTextValueForTest(), "first");

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "path": "/label",
        "value": "second"
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(update->GetRoot()));

    EXPECT_EQ(text->GetTextValueForTest(), "second");
}

TEST_F(ExtendedTextAliasTest, should_prefer_content_when_content_and_text_are_both_present)
{
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "canonical",
            "text": "alias"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text = GetTextComponent();
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetTextValueForTest(), "canonical");
}

} // namespace
