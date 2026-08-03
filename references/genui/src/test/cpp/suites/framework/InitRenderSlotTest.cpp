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

#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/button/ButtonTheme.h"
#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "theme/ThemeManager.h"

#include "NativeEntry.h"
#include "ProtocolVersionPolicy.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceErrorCodes.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

class InitRenderSlotTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    napi_callback_info cbInfo_ = reinterpret_cast<napi_callback_info>(0x200);
    std::vector<int32_t> createdIds_;

    napi_value CreateInt32Arg(int32_t value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateInt32(env_, value, &result);
        return result;
    }

    napi_value CreateStringArg(const std::string& value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateStringUtf8(env_, value.c_str(), value.length(), &result);
        return result;
    }

    napi_value CreateBoolArg(bool value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateBoolean(env_, value, &result);
        return result;
    }

    napi_value CreateCatalogArg(const std::string& catalogId, const std::string& a2UIProtocolVersion)
    {
        napi_value catalog = nullptr;
        mockNapiPtr_->CreateObject(env_, &catalog);
        mockNapiPtr_->SetNamedProperty(env_, catalog, "id", CreateStringArg(catalogId));
        mockNapiPtr_->SetNamedProperty(env_, catalog, "a2UIProtocolVersion", CreateStringArg(a2UIProtocolVersion));
        return catalog;
    }

    napi_value CreateOptionsArg(bool supportsMultipleSurfaces, int32_t maxSurfaceCount)
    {
        napi_value options = nullptr;
        mockNapiPtr_->CreateObject(env_, &options);
        mockNapiPtr_->SetNamedProperty(
            env_, options, "supportsMultipleSurfaces", CreateBoolArg(supportsMultipleSurfaces));
        mockNapiPtr_->SetNamedProperty(env_, options, "maxSurfaceCount", CreateInt32Arg(maxSurfaceCount));
        mockNapiPtr_->SetNamedProperty(env_, options, "isExtend", CreateBoolArg(false));
        return options;
    }

    napi_value CreateProtocolOptionsArg(bool isExtend)
    {
        napi_value options = CreateOptionsArg(false, -1);
        mockNapiPtr_->SetNamedProperty(env_, options, "isExtend", CreateBoolArg(isExtend));
        return options;
    }

    napi_value CreateCatalogWithSingleComponentArg(
        const std::string& catalogId, const std::string& componentName, CatalogCategory category, bool isInnerNative)
    {
        napi_value catalog = CreateCatalogArg(catalogId, "v0.9");
        napi_value components = nullptr;
        mockNapiPtr_->CreateArrayWithLength(env_, 1, &components);

        napi_value item = nullptr;
        mockNapiPtr_->CreateObject(env_, &item);
        mockNapiPtr_->SetNamedProperty(env_, item, "name", CreateStringArg(componentName));
        mockNapiPtr_->SetNamedProperty(env_, item, "category", CreateInt32Arg(static_cast<int32_t>(category)));
        mockNapiPtr_->SetNamedProperty(
            env_, item, "type", CreateInt32Arg(static_cast<int32_t>(CatalogItemType::COMPONENT)));
        mockNapiPtr_->SetNamedProperty(env_, item, "isInnerNative", CreateBoolArg(isInnerNative));
        mockNapiPtr_->SetElement(env_, components, 0, item);
        mockNapiPtr_->SetNamedProperty(env_, catalog, "components", components);
        return catalog;
    }

    void SetUpRenderIdArg(int32_t renderId)
    {
        napi_value arg = CreateInt32Arg(renderId);
        mockNapiPtr_->SetCallbackArgs({ arg });
    }

    void ClearCallbackArgs()
    {
        mockNapiPtr_->SetCallbackArgs({});
    }

    void TrackAndCreateSlot(int32_t renderId)
    {
        RenderManager::GetInstance().CreateRenderSlot(renderId);
        createdIds_.push_back(renderId);
    }

    bool GetBoolProperty(napi_value object, const std::string& key)
    {
        napi_value value = mockNapiPtr_->objectProperties_[object][key];
        return mockNapiPtr_->boolValues_[value];
    }

    int32_t GetInt32Property(napi_value object, const std::string& key)
    {
        napi_value value = mockNapiPtr_->objectProperties_[object][key];
        return static_cast<int32_t>(mockNapiPtr_->numberValues_[value]);
    }

    std::string GetStringProperty(napi_value object, const std::string& key)
    {
        napi_value value = mockNapiPtr_->objectProperties_[object][key];
        return mockNapiPtr_->stringValues_[value];
    }

    void TrackSlotId(int32_t renderId)
    {
        createdIds_.push_back(renderId);
    }

    void TearDown() override
    {
        auto& rm = RenderManager::GetInstance();
        for (int32_t id : createdIds_) {
            if (rm.HasRenderSlot(id)) {
                rm.RemoveRenderSlot(id);
            }
        }
        createdIds_.clear();
        A2UITest::TearDown();
    }
};

/**
 * @tc.name: InitRenderSlotTest000
 * @tc.desc: Verify native protocol version policy only supports the current A2UI protocol version.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest000)
{
    EXPECT_TRUE(IsSupportedA2UIProtocolVersion("v0.9"));
    EXPECT_FALSE(IsSupportedA2UIProtocolVersion("v1.0"));
    EXPECT_FALSE(IsSupportedA2UIProtocolVersion(""));
}

/**
 * @tc.name: InitRenderSlotTest001
 * @tc.desc: Verify the following render slot management behavior: return null and create slot when valid render id.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest001)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(1);
    TrackSlotId(1);
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(1));
    auto* slot = RenderManager::GetInstance().FindRenderSlot(1);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->GetRenderId(), 1);
}

/**
 * @tc.name: InitRenderSlotTest002
 * @tc.desc: Verify the following render slot management behavior: return null when negative render id.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest002)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(-1);
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
    EXPECT_FALSE(RenderManager::GetInstance().HasRenderSlot(-1));
}

/**
 * @tc.name: InitRenderSlotTest003
 * @tc.desc: Verify the following render slot management behavior: create slot when render id zero.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest003)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(0);
    TrackSlotId(0);
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(0));
}

/**
 * @tc.name: InitRenderSlotTest004
 * @tc.desc: Verify the following render slot management behavior: create slot with large render id.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest004)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(99999);
    TrackSlotId(99999);
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(99999));
    auto* slot = RenderManager::GetInstance().FindRenderSlot(99999);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->GetRenderId(), 99999);
}

/**
 * @tc.name: InitRenderSlotTest005
 * @tc.desc: Verify the following render slot management behavior: init render slot with no args.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest005)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    ClearCallbackArgs();
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: InitRenderSlotTest006
 * @tc.desc: Verify the following render slot management behavior: return null when render id argument is null.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest006)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    mockNapiPtr_->SetCallbackArgs({ nullptr });
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 0u);
}

/**
 * @tc.name: InitRenderSlotTest007
 * @tc.desc: Verify the following render slot management behavior: return null when get callback info fails.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest007)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(1);
    mockNapiPtr_->SetGetCbInfoStatus(napi_invalid_arg);
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
    EXPECT_FALSE(RenderManager::GetInstance().HasRenderSlot(1));
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 0u);
}

/**
 * @tc.name: InitRenderSlotTest008
 * @tc.desc: Verify the following render slot management behavior: create slot after resetting get callback info status.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest008)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(1);
    mockNapiPtr_->SetGetCbInfoStatus(napi_invalid_arg);
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_FALSE(RenderManager::GetInstance().HasRenderSlot(1));
    mockNapiPtr_->ResetGetCbInfoStatus();
    TrackSlotId(1);
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(1));
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 1u);
}

/**
 * @tc.name: InitRenderSlotTest009
 * @tc.desc: Verify the following render slot management behavior: return null when get value int32 fails.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest009)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(1);
    mockNapiPtr_->SetGetValueInt32Status(napi_number_expected);
    auto result = InitRenderSlot(env_, cbInfo_);
    EXPECT_EQ(result, nullptr);
    EXPECT_FALSE(RenderManager::GetInstance().HasRenderSlot(1));
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 0u);
}

/**
 * @tc.name: InitRenderSlotTest010
 * @tc.desc: Verify the following render slot management behavior: create slot after resetting get value int32 status.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest010)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(2);
    mockNapiPtr_->SetGetValueInt32Status(napi_number_expected);
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_FALSE(RenderManager::GetInstance().HasRenderSlot(2));
    mockNapiPtr_->ResetGetValueInt32Status();
    TrackSlotId(2);
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(2));
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 1u);
}

/**
 * @tc.name: InitRenderSlotTest011
 * @tc.desc: Verify the following render slot management behavior: create multiple slots.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest011)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    TrackSlotId(1);
    TrackSlotId(2);
    napi_value arg1 = CreateInt32Arg(1);
    mockNapiPtr_->SetCallbackArgs({ arg1 });
    InitRenderSlot(env_, cbInfo_);
    napi_value arg2 = CreateInt32Arg(2);
    mockNapiPtr_->SetCallbackArgs({ arg2 });
    InitRenderSlot(env_, cbInfo_);
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(1));
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(2));
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 2u);
}

/**
 * @tc.name: InitRenderSlotTest012
 * @tc.desc: Verify the following render slot management behavior: not create duplicate slot when init called twice with
 * same render id.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest012)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    SetUpRenderIdArg(3);
    TrackSlotId(3);
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    ASSERT_TRUE(RenderManager::GetInstance().HasRenderSlot(3));
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 1u);
    SetUpRenderIdArg(3);
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 1u);
    auto* slot = RenderManager::GetInstance().FindRenderSlot(3);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->GetRenderId(), 3);
}

/**
 * @tc.name: InitRenderSlotTest013
 * @tc.desc: Verify the following render slot management behavior: have surface manager in created slot.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest013)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    TrackAndCreateSlot(42);
    auto* slot = RenderManager::GetInstance().FindRenderSlot(42);
    ASSERT_NE(slot, nullptr);
    EXPECT_NE(slot->GetSurfaceManager(), nullptr);
}

/**
 * @tc.name: InitRenderSlotTest014
 * @tc.desc: Verify the following render slot management behavior: find slot after direct creation.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest014)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    TrackAndCreateSlot(1);
    auto* slot = RenderManager::GetInstance().FindRenderSlot(1);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->GetRenderId(), 1);
}

/**
 * @tc.name: InitRenderSlotTest015
 * @tc.desc: Verify the following render slot management behavior: not find nonexistent slot.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest015)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    auto* slot = RenderManager::GetInstance().FindRenderSlot(999);
    EXPECT_EQ(slot, nullptr);
}

/**
 * @tc.name: InitRenderSlotTest016
 * @tc.desc: Verify the following render slot management behavior: remove slot.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest016)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    TrackAndCreateSlot(1);
    EXPECT_TRUE(RenderManager::GetInstance().HasRenderSlot(1));
    RenderManager::GetInstance().RemoveRenderSlot(1);
    EXPECT_FALSE(RenderManager::GetInstance().HasRenderSlot(1));
}

/**
 * @tc.name: InitRenderSlotTest017
 * @tc.desc: Verify the following render slot management behavior: report zero count when empty.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest017)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 0u);
}

/**
 * @tc.name: InitRenderSlotTest018
 * @tc.desc: Verify the following render slot management behavior: increase count after creation.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest018)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    TrackAndCreateSlot(1);
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 1u);
    TrackAndCreateSlot(2);
    EXPECT_EQ(RenderManager::GetInstance().GetRenderSlotCount(), 2u);
}

/**
 * @tc.name: InitRenderSlotTest019
 * @tc.desc: Verify catalog version metadata is propagated into the created surface context.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest019)
{
    int32_t renderId = 910;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"ctx-surface","catalogId":"catalog-with-version"}})");
    napi_value catalogArg = CreateCatalogArg("catalog-with-version", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    EXPECT_NE(result, nullptr);

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("ctx-surface");
    ASSERT_NE(surfaceSlot, nullptr);

    const SurfaceContext& context = surfaceSlot->GetSurfaceContext();
    EXPECT_EQ(context.a2UIProtocolVersion, "v0.9");
}

/**
 * @tc.name: InitRenderSlotTest020
 * @tc.desc: Verify unsupported A2UI protocol version returns a dedicated process error.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest020)
{
    int32_t renderId = 911;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v1.0","createSurface":{"surfaceId":"bad-version","catalogId":"catalog-with-version"}})");
    napi_value catalogArg = CreateCatalogArg("catalog-with-version", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);

    EXPECT_FALSE(GetBoolProperty(result, "success"));
    EXPECT_EQ(GetStringProperty(result, "errorCode"), "UNSUPPORTED_PROTOCOL_VERSION");
    EXPECT_EQ(GetStringProperty(result, "errorMessage"), "unsupported A2UI protocol version");
    EXPECT_EQ(GetInt32Property(result, "surfaceResultCode"), SURFACE_RESULT_UNSUPPORTED_PROTOCOL_VERSION);
}

/**
 * @tc.name: InitRenderSlotTest021
 * @tc.desc: Verify v0.9 message dispatcher preserves create and delete surface behavior.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest021)
{
    int32_t renderId = 912;
    TrackAndCreateSlot(renderId);
    napi_value catalogArg = CreateCatalogArg("catalog-with-version", "v0.9");

    napi_value createRenderIdArg = CreateInt32Arg(renderId);
    napi_value createDslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"dispatch-surface","catalogId":"catalog-with-version"}})");
    mockNapiPtr_->SetCallbackArgs({ createRenderIdArg, createDslArg, catalogArg });
    napi_value createResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(createResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(createResult, "success"));
    EXPECT_EQ(GetStringProperty(createResult, "surfaceId"), "dispatch-surface");

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    ASSERT_NE(surfaceManager->FindSurface("dispatch-surface"), nullptr);

    napi_value deleteRenderIdArg = CreateInt32Arg(renderId);
    napi_value deleteDslArg = CreateStringArg(R"({"version":"v0.9","deleteSurface":{"surfaceId":"dispatch-surface"}})");
    mockNapiPtr_->SetCallbackArgs({ deleteRenderIdArg, deleteDslArg, catalogArg });
    napi_value deleteResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(deleteResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(deleteResult, "success"));
    EXPECT_EQ(GetStringProperty(deleteResult, "surfaceId"), "dispatch-surface");
    EXPECT_EQ(surfaceManager->FindSurface("dispatch-surface"), nullptr);
}

/**
 * @tc.name: InitRenderSlotTest022
 * @tc.desc: Verify the following render slot management behavior: not create duplicate slot when same render id.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest022)
{
    /**
     * @tc.steps: step1. Prepare the render slot input or mock state and invoke the target initialization or manager
     * interface.
     * @tc.expected: The returned value and render slot manager state match the expectation.
     */

    TrackAndCreateSlot(1);
    size_t countAfterFirst = RenderManager::GetInstance().GetRenderSlotCount();
    EXPECT_EQ(countAfterFirst, 1u);
    TrackAndCreateSlot(1);
    size_t countAfterSecond = RenderManager::GetInstance().GetRenderSlotCount();
    EXPECT_EQ(countAfterSecond, countAfterFirst);
}

/**
 * @tc.name: InitRenderSlotTest023
 * @tc.desc: Verify createSurface.theme is stored in the surface theme context and primaryColor is mapped to
 * ButtonTheme.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest023)
{
    int32_t renderId = 913;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"theme-surface","catalogId":"catalog-theme","theme":{"primaryColor":"#FF6A00","iconUrl":"https://a2ui.test/icon.png","agentDisplayName":"Theme Test Bot"}}})");
    napi_value catalogArg = CreateCatalogArg("catalog-theme", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("theme-surface");
    ASSERT_NE(surfaceSlot, nullptr);
    std::shared_ptr<ThemeManager> themeManager = surfaceSlot->GetThemeManager();
    ASSERT_NE(themeManager, nullptr);

    const ThemeContext& context = themeManager->GetContext();
    EXPECT_TRUE(context.hasPrimaryColor);
    EXPECT_EQ(context.primaryColorArgb, 0xFFFF6A00u);
    EXPECT_EQ(context.iconUrl, "https://a2ui.test/icon.png");
    EXPECT_EQ(context.agentDisplayName, "Theme Test Bot");
    EXPECT_TRUE(context.hasBrandColor);
    EXPECT_EQ(context.brandColor, 0xFFFF6A00u);

    std::shared_ptr<ButtonTheme> buttonTheme = std::dynamic_pointer_cast<ButtonTheme>(themeManager->GetTheme("Button"));
    ASSERT_NE(buttonTheme, nullptr);
    EXPECT_EQ(buttonTheme->GetBackgroundColor("primary"), 0xFFFF6A00u);
}

/**
 * @tc.name: InitRenderSlotTest024
 * @tc.desc: Verify invalid theme.primaryColor does not fail createSurface and falls back to default brand color state.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest024)
{
    int32_t renderId = 914;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"theme-invalid","catalogId":"catalog-theme","theme":{"primaryColor":"#GG6A00","agentDisplayName":"Theme Test Bot"}}})");
    napi_value catalogArg = CreateCatalogArg("catalog-theme", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("theme-invalid");
    ASSERT_NE(surfaceSlot, nullptr);
    std::shared_ptr<ThemeManager> themeManager = surfaceSlot->GetThemeManager();
    ASSERT_NE(themeManager, nullptr);

    const ThemeContext& context = themeManager->GetContext();
    EXPECT_FALSE(context.hasPrimaryColor);
    EXPECT_FALSE(context.hasBrandColor);
    EXPECT_EQ(context.brandColor, 0u);
    EXPECT_EQ(context.agentDisplayName, "Theme Test Bot");
}

/**
 * @tc.name: InitRenderSlotTest025
 * @tc.desc: Verify createSurface.theme remains isolated per surface when multiple surfaces are enabled.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest025)
{
    int32_t renderId = 915;
    TrackAndCreateSlot(renderId);
    napi_value catalogArg = CreateCatalogArg("catalog-theme", "v0.9");
    napi_value optionsArg = CreateOptionsArg(true, 4);

    napi_value renderIdArg1 = CreateInt32Arg(renderId);
    napi_value dslArg1 = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"surface-a","catalogId":"catalog-theme","theme":{"primaryColor":"#FF6A00","agentDisplayName":"Theme A"}}})");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg1, dslArg1, catalogArg, optionsArg });
    napi_value firstResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(firstResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(firstResult, "success"));

    napi_value renderIdArg2 = CreateInt32Arg(renderId);
    napi_value dslArg2 = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"surface-b","catalogId":"catalog-theme","theme":{"primaryColor":"#0088FF","agentDisplayName":"Theme B"}}})");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg2, dslArg2, catalogArg, optionsArg });
    napi_value secondResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(secondResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(secondResult, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceA = surfaceManager->FindSurface("surface-a");
    SurfaceSlot* surfaceB = surfaceManager->FindSurface("surface-b");
    ASSERT_NE(surfaceA, nullptr);
    ASSERT_NE(surfaceB, nullptr);

    const ThemeContext& contextA = surfaceA->GetThemeManager()->GetContext();
    const ThemeContext& contextB = surfaceB->GetThemeManager()->GetContext();
    EXPECT_EQ(contextA.agentDisplayName, "Theme A");
    EXPECT_EQ(contextA.primaryColorArgb, 0xFFFF6A00u);
    EXPECT_EQ(contextB.agentDisplayName, "Theme B");
    EXPECT_EQ(contextB.primaryColorArgb, 0xFF0088FFu);
}

/**
 * @tc.name: InitRenderSlotTest026
 * @tc.desc: Verify createSurface without theme keeps the surface on the legacy non-theme path.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest026)
{
    int32_t renderId = 916;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"no-theme-surface","catalogId":"catalog-no-theme"}})");
    napi_value catalogArg = CreateCatalogArg("catalog-no-theme", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("no-theme-surface");
    ASSERT_NE(surfaceSlot, nullptr);
    ASSERT_NE(surfaceSlot->GetThemeManager(), nullptr);
}

/**
 * @tc.name: InitRenderSlotTest027
 * @tc.desc: Verify surfaces without theme stay on the legacy path when a sibling surface enables createSurface.theme.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest027)
{
    int32_t renderId = 917;
    TrackAndCreateSlot(renderId);
    napi_value catalogArg = CreateCatalogArg("catalog-theme-mixed", "v0.9");
    napi_value optionsArg = CreateOptionsArg(true, 4);

    napi_value renderIdArg1 = CreateInt32Arg(renderId);
    napi_value dslArg1 = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"plain-surface","catalogId":"catalog-theme-mixed"}})");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg1, dslArg1, catalogArg, optionsArg });
    napi_value plainResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(plainResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(plainResult, "success"));

    napi_value renderIdArg2 = CreateInt32Arg(renderId);
    napi_value dslArg2 = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"themed-surface","catalogId":"catalog-theme-mixed","theme":{"primaryColor":"#0088FF","agentDisplayName":"Theme B"}}})");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg2, dslArg2, catalogArg, optionsArg });
    napi_value themedResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(themedResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(themedResult, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* plainSurface = surfaceManager->FindSurface("plain-surface");
    SurfaceSlot* themedSurface = surfaceManager->FindSurface("themed-surface");
    ASSERT_NE(plainSurface, nullptr);
    ASSERT_NE(themedSurface, nullptr);
    ASSERT_NE(plainSurface->GetThemeManager(), nullptr);
    std::shared_ptr<ThemeManager> themeManager = themedSurface->GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    EXPECT_EQ(themeManager->GetContext().primaryColorArgb, 0xFF0088FFu);
    EXPECT_EQ(themeManager->GetContext().agentDisplayName, "Theme B");
}

/**
 * @tc.name: InitRenderSlotTest028
 * @tc.desc: Verify darkPrimaryColor is parsed and applied as brand color in dark mode.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest028)
{
    int32_t renderId = 918;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"theme-dark-primary","catalogId":"catalog-theme-dark","theme":{"primaryColor":"#123456","darkPrimaryColor":"#0A0B0C"}}})");
    napi_value catalogArg = CreateCatalogArg("catalog-theme-dark", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("theme-dark-primary");
    ASSERT_NE(surfaceSlot, nullptr);
    std::shared_ptr<ThemeManager> themeManager = surfaceSlot->GetThemeManager();
    ASSERT_NE(themeManager, nullptr);

    const ThemeContext& lightContext = themeManager->GetContext();
    EXPECT_TRUE(lightContext.hasDarkPrimaryColor);
    EXPECT_EQ(lightContext.darkPrimaryColorArgb, 0xFF0A0B0Cu);
    EXPECT_TRUE(lightContext.hasBrandColor);
    EXPECT_EQ(lightContext.brandColor, 0xFF123456u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    const ThemeContext& darkContext = themeManager->GetContext();
    EXPECT_EQ(darkContext.colorMode, ThemeMode::DARK);
    EXPECT_TRUE(darkContext.hasBrandColor);
    EXPECT_EQ(darkContext.brandColor, 0xFF0A0B0Cu);
}

/**
 * @tc.name: InitRenderSlotTest029
 * @tc.desc: Verify dark mode falls back to inverted primaryColor when darkPrimaryColor is absent.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest029)
{
    int32_t renderId = 919;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"theme-invert-dark","catalogId":"catalog-theme-invert","theme":{"primaryColor":"#123456"}}})");
    napi_value catalogArg = CreateCatalogArg("catalog-theme-invert", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("theme-invert-dark");
    ASSERT_NE(surfaceSlot, nullptr);
    std::shared_ptr<ThemeManager> themeManager = surfaceSlot->GetThemeManager();
    ASSERT_NE(themeManager, nullptr);

    const ThemeContext& lightContext = themeManager->GetContext();
    EXPECT_FALSE(lightContext.hasDarkPrimaryColor);
    EXPECT_EQ(lightContext.brandColor, 0xFF123456u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    const ThemeContext& darkContext = themeManager->GetContext();
    EXPECT_EQ(darkContext.colorMode, ThemeMode::DARK);
    EXPECT_TRUE(darkContext.hasBrandColor);
    EXPECT_EQ(darkContext.brandColor, 0xFFEDCBA9u);
}

/**
 * @tc.name: InitRenderSlotTest030
 * @tc.desc: Verify createSurface keeps legacy no-theme behavior even after dark mode update.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest030)
{
    int32_t renderId = 920;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"no-theme-after-dark","catalogId":"catalog-no-theme-dark"}})");
    napi_value catalogArg = CreateCatalogArg("catalog-no-theme-dark", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("no-theme-after-dark");
    ASSERT_NE(surfaceSlot, nullptr);
    ASSERT_NE(surfaceSlot->GetThemeManager(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    ASSERT_NE(surfaceSlot->GetThemeManager(), nullptr);
}

/**
 * @tc.name: InitRenderSlotTest031
 * @tc.desc: Verify createSurface applies darkPrimaryColor immediately when current mode is dark.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest031)
{
    int32_t renderId = 921;
    TrackAndCreateSlot(renderId);
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::DARK);

    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value dslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"create-in-dark","catalogId":"catalog-create-dark","theme":{"primaryColor":"#112233","darkPrimaryColor":"#445566"}}})");
    napi_value catalogArg = CreateCatalogArg("catalog-create-dark", "v0.9");
    mockNapiPtr_->SetCallbackArgs({ renderIdArg, dslArg, catalogArg });

    napi_value result = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));

    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("create-in-dark");
    ASSERT_NE(surfaceSlot, nullptr);
    std::shared_ptr<ThemeManager> themeManager = surfaceSlot->GetThemeManager();
    ASSERT_NE(themeManager, nullptr);

    const ThemeContext& context = themeManager->GetContext();
    EXPECT_EQ(context.colorMode, ThemeMode::DARK);
    EXPECT_TRUE(context.hasBrandColor);
    EXPECT_EQ(context.brandColor, 0xFF445566u);
}

/**
 * @tc.name: InitRenderSlotTest032
 * @tc.desc: Verify createSurface.catalogId drives extended protocol routing even when the catalog object id is not
 * extended.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest032)
{
    int32_t renderId = 922;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value createDslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"protocol-extended","catalogId":"ohos.a2ui.extended.catalog"}})");
    napi_value updateDslArg = CreateStringArg(
        R"({"version":"v0.9","updateComponents":{"surfaceId":"protocol-extended","components":[{"id":"root","component":"Text","text":"hello"}]}})");
    napi_value catalogArg =
        CreateCatalogWithSingleComponentArg("catalog-native-entry", "Text", CatalogCategory::OHOS_EXTENDS, false);
    napi_value optionsArg = CreateProtocolOptionsArg(true);

    mockNapiPtr_->SetCallbackArgs({ renderIdArg, createDslArg, catalogArg, optionsArg });
    napi_value createResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(createResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(createResult, "success"));

    mockNapiPtr_->SetCallbackArgs({ renderIdArg, updateDslArg, catalogArg });
    napi_value updateResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(updateResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(updateResult, "success"));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface("protocol-extended");
    ASSERT_NE(surfaceSlot, nullptr);
    std::shared_ptr<Component> component = surfaceSlot->FindComponentById("root");
    ASSERT_NE(component, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedTextComponent>(component), nullptr);
}

/**
 * @tc.name: InitRenderSlotTest034
 * @tc.desc: Verify createSurface is rejected when controller protocol expectation mismatches the catalogId carried in
 * DSL.
 * @tc.type: FUNC
 */
TEST_F(InitRenderSlotTest, InitRenderSlotTest034)
{
    int32_t renderId = 924;
    TrackAndCreateSlot(renderId);
    napi_value renderIdArg = CreateInt32Arg(renderId);
    napi_value extendedCreateDslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"protocol-mismatch-extended","catalogId":"ohos.a2ui.extended.catalog"}})");
    napi_value standardCreateDslArg = CreateStringArg(
        R"({"version":"v0.9","createSurface":{"surfaceId":"protocol-mismatch-standard","catalogId":"standard.catalog"}})");
    napi_value catalogArg =
        CreateCatalogWithSingleComponentArg(A2UI_EXTENDED_CATALOG_ID, "Text", CatalogCategory::OHOS_EXTENDS, false);

    mockNapiPtr_->SetCallbackArgs({ renderIdArg, extendedCreateDslArg, catalogArg, CreateProtocolOptionsArg(false) });
    napi_value standardControllerResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(standardControllerResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(standardControllerResult, "success"));
    EXPECT_EQ(GetStringProperty(standardControllerResult, "errorCode"), "PROTOCOL_MISMATCH");

    mockNapiPtr_->SetCallbackArgs({ renderIdArg, standardCreateDslArg, catalogArg, CreateProtocolOptionsArg(true) });
    napi_value extendedControllerResult = ProcessMessage(env_, cbInfo_);
    ASSERT_NE(extendedControllerResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(extendedControllerResult, "success"));
    EXPECT_EQ(GetStringProperty(extendedControllerResult, "errorCode"), "PROTOCOL_MISMATCH");

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    ASSERT_NE(renderSlot, nullptr);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    EXPECT_EQ(surfaceManager->FindSurface("protocol-mismatch-standard"), nullptr);
}
