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

#include "catalog/Catalog.h"
#include "catalog/CatalogItem.h"
#include "functions/RuntimeErrorDispatchBridge.h"

#include "A2UIComponentTddTestHelper.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

constexpr int32_t RESOURCE_LIMIT_RENDER_ID = 812999;
constexpr char RESOURCE_LIMIT_SURFACE_ID[] = "resource-limit-tdd";

std::shared_ptr<Catalog> CreateColumnTextCatalog()
{
    auto catalog = std::make_shared<Catalog>("resource-limit-tdd-catalog");
    auto columnItem = std::make_shared<CatalogItem>("Column", CatalogItemType::COMPONENT);
    columnItem->SetInnerNative(true);
    catalog->AddComponent(columnItem);
    auto textItem = std::make_shared<CatalogItem>("Text", CatalogItemType::COMPONENT);
    textItem->SetInnerNative(true);
    catalog->AddComponent(textItem);
    return catalog;
}

std::string BuildDeepComponentJson(int32_t depth)
{
    std::string components;
    for (int32_t i = 1; i <= depth; ++i) {
        if (i > 1) {
            components += ",";
        }
        std::string childRef = (i < depth) ? ("\"c" + std::to_string(i + 1) + "\"") : "\"tip\"";
        components +=
            "{\"id\":\"c" + std::to_string(i) + "\",\"component\":\"Column\",\"children\":[" + childRef + "]}";
    }
    components += ",{\"id\":\"tip\",\"component\":\"Text\",\"text\":\"deep\"}";
    components += ",{\"id\":\"root\",\"component\":\"Column\",\"children\":[\"c1\"]}";
    return "{\"components\":[" + components + "]}";
}

std::string BuildManyComponentsJson(int32_t count)
{
    std::string components;
    for (int32_t i = 0; i < count; ++i) {
        if (i > 0) {
            components += ",";
        }
        components += "{\"id\":\"t" + std::to_string(i) + "\",\"component\":\"Text\",\"text\":\"x\"}";
    }
    components += ",{\"id\":\"root\",\"component\":\"Column\",\"children\":[\"t0\"]}";
    return "{\"components\":[" + components + "]}";
}

std::string BuildNestedDataModelJson(int32_t depth)
{
    std::string json = R"({"a":)";
    for (int32_t i = 1; i < depth; ++i) {
        json += R"({"a":)";
    }
    json += "1";
    for (int32_t i = 1; i < depth; ++i) {
        json += "}";
    }
    json += "}";
    return "{\"value\":" + json + "}";
}

} // namespace

class SurfaceSlotResourceLimitTddTest : public A2UIComponentTddTest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    SurfaceSlot* slot_ = nullptr;

    void SetUp() override
    {
        A2UIComponentTddTest::SetUp();
        RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
        auto& renderManager = RenderManager::GetInstance();
        if (renderManager.HasRenderSlot(RESOURCE_LIMIT_RENDER_ID)) {
            renderManager.RemoveRenderSlot(RESOURCE_LIMIT_RENDER_ID);
        }
        RenderSlot& renderSlot = renderManager.CreateRenderSlot(RESOURCE_LIMIT_RENDER_ID);
        auto surfaceManager = renderSlot.GetSurfaceManager();
        ASSERT_NE(surfaceManager, nullptr);
        slot_ = &surfaceManager->CreateSurface(RESOURCE_LIMIT_SURFACE_ID, nullptr);
        slot_->SetCatalog(CreateColumnTextCatalog());
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(RESOURCE_LIMIT_RENDER_ID);
        slot_ = nullptr;
        A2UIComponentTddTest::TearDown();
    }

    napi_value CreateCallback()
    {
        napi_value callback = nullptr;
        mockNapiPtr_->CreateFunction(env_, "runtimeErrorCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        return callback;
    }

    bool ExtractRuntimeErrorRequest(
        napi_value request, int32_t* renderId, int32_t* errorCode, std::string* errorMessage, std::string* source) const
    {
        auto objectIt = mockNapiPtr_->objectProperties_.find(request);
        if (objectIt == mockNapiPtr_->objectProperties_.end()) {
            return false;
        }

        const auto& properties = objectIt->second;
        auto renderIdIt = properties.find("renderId");
        auto errorCodeIt = properties.find("errorCode");
        auto errorMessageIt = properties.find("errorMessage");
        auto sourceIt = properties.find("source");
        if (renderIdIt == properties.end() || errorCodeIt == properties.end() || errorMessageIt == properties.end() ||
            sourceIt == properties.end()) {
            return false;
        }

        if (renderId != nullptr) {
            auto renderIdValue = mockNapiPtr_->numberValues_.find(renderIdIt->second);
            *renderId =
                renderIdValue != mockNapiPtr_->numberValues_.end() ? static_cast<int32_t>(renderIdValue->second) : 0;
        }
        if (errorCode != nullptr) {
            auto errorCodeValue = mockNapiPtr_->numberValues_.find(errorCodeIt->second);
            *errorCode =
                errorCodeValue != mockNapiPtr_->numberValues_.end() ? static_cast<int32_t>(errorCodeValue->second) : 0;
        }
        if (errorMessage != nullptr) {
            auto errorMessageValue = mockNapiPtr_->stringValues_.find(errorMessageIt->second);
            *errorMessage = errorMessageValue != mockNapiPtr_->stringValues_.end() ? errorMessageValue->second : "";
        }
        if (source != nullptr) {
            auto sourceValue = mockNapiPtr_->stringValues_.find(sourceIt->second);
            *source = sourceValue != mockNapiPtr_->stringValues_.end() ? sourceValue->second : "";
        }

        return true;
    }

    size_t RuntimeErrorDispatchCount() const
    {
        size_t count = 0;
        for (const auto& args : mockNapiPtr_->callFunctionArgsHistory_) {
            if (args.empty()) {
                continue;
            }
            if (ExtractRuntimeErrorRequest(args[0], nullptr, nullptr, nullptr, nullptr)) {
                ++count;
            }
        }
        return count;
    }

    bool FindLatestRuntimeError(
        int32_t* renderId, int32_t* errorCode, std::string* errorMessage, std::string* source) const
    {
        for (auto iter = mockNapiPtr_->callFunctionArgsHistory_.rbegin();
             iter != mockNapiPtr_->callFunctionArgsHistory_.rend(); ++iter) {
            if (iter->empty()) {
                continue;
            }
            if (ExtractRuntimeErrorRequest(iter->front(), renderId, errorCode, errorMessage, source)) {
                return true;
            }
        }
        return false;
    }
};

TEST_F(SurfaceSlotResourceLimitTddTest, L0_should_accept_components_when_count_within_limit)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildManyComponentsJson(3));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateComponents(msg->GetRoot()));
    EXPECT_NE(slot_->GetRootComponent(), nullptr);
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L1_should_continue_without_dispatch_when_component_count_exceeds_1000)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildManyComponentsJson(1001));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateComponents(msg->GetRoot()));
    EXPECT_NE(slot_->GetRootComponent(), nullptr);
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L2_should_accept_components_when_count_exactly_1000)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildManyComponentsJson(999));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateComponents(msg->GetRoot()));
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L0_should_accept_tree_when_depth_within_limit)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildDeepComponentJson(5));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateComponents(msg->GetRoot()));
    EXPECT_NE(slot_->GetRootComponent(), nullptr);
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L1_should_continue_without_dispatch_when_tree_depth_exceeds_50)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildDeepComponentJson(51));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateComponents(msg->GetRoot()));
    EXPECT_NE(slot_->GetRootComponent(), nullptr);
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L2_should_accept_tree_at_exactly_depth_50)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildDeepComponentJson(48));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateComponents(msg->GetRoot()));
    EXPECT_NE(slot_->GetRootComponent(), nullptr);
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L2_should_continue_tree_update_at_depth_51_without_dispatch)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildDeepComponentJson(49));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateComponents(msg->GetRoot()));
    EXPECT_NE(slot_->GetRootComponent(), nullptr);
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L0_should_accept_data_model_when_depth_within_limit)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildNestedDataModelJson(5));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateDataModel(msg->GetRoot()));
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L1_should_continue_without_dispatch_when_data_model_depth_exceeds_20)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildNestedDataModelJson(21));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateDataModel(msg->GetRoot()));
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L2_should_accept_data_model_at_exactly_depth_20)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildNestedDataModelJson(20));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateDataModel(msg->GetRoot()));
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}

TEST_F(SurfaceSlotResourceLimitTddTest, L2_should_continue_data_model_update_at_depth_21_without_dispatch)
{
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();
    auto msg = ParseJson(BuildNestedDataModelJson(21));
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(slot_->UpdateDataModel(msg->GetRoot()));
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount);
}
