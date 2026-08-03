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
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/custom/CustomComponent.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildCatalog(
    const std::string& catalogId, std::initializer_list<std::pair<const char*, bool>> components)
{
    auto catalog = std::make_shared<Catalog>(catalogId);
    for (const auto& entry : components) {
        if (entry.first == nullptr || entry.first[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(entry.first, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        item->SetInnerNative(entry.second);
        catalog->AddComponent(item);
    }
    return catalog;
}

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

class RenderSlotCleanupGuard {
public:
    explicit RenderSlotCleanupGuard(int32_t renderId) : renderId_(renderId) {}
    ~RenderSlotCleanupGuard()
    {
        RenderManager::GetInstance().RemoveRenderSlot(renderId_);
    }

private:
    int32_t renderId_;
};

} // namespace

class SurfaceSlotCustomComponentCoverageTest : public A2UITest {};

TEST_F(SurfaceSlotCustomComponentCoverageTest, should_cover_static_children_attachment_for_non_tabs_parent)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("surface_children");
    slot.SetRenderId(1102);
    slot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Column", true }, { "Text", true } }));

    std::unique_ptr<JsonAdapter> message = ParseJson(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": ["childA", "childB"]
            },
            {
                "id": "childA",
                "component": "Text",
                "content": "A"
            },
            {
                "id": "childB",
                "component": "Text",
                "content": "B"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = slot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 2u);
    auto childIt = root->GetChildren().begin();
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), "childA");
    ++childIt;
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), "childB");
}
