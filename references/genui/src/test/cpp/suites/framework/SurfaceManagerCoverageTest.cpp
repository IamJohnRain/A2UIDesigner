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

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

constexpr int32_t SURFACE_MANAGER_TEST_RENDER_ID = 810100;
constexpr char SURFACE_MANAGER_TEST_SURFACE_A[] = "surface-a";
constexpr char SURFACE_MANAGER_TEST_SURFACE_B[] = "surface-b";
constexpr char SURFACE_MANAGER_TEST_SURFACE_C[] = "surface-c";

} // namespace

class SurfaceManagerCoverageTest : public A2UITest {
protected:
    void TearDown() override
    {
        auto& renderManager = RenderManager::GetInstance();
        if (renderManager.HasRenderSlot(SURFACE_MANAGER_TEST_RENDER_ID)) {
            renderManager.RemoveRenderSlot(SURFACE_MANAGER_TEST_RENDER_ID);
        }
        A2UITest::TearDown();
    }
};

TEST_F(SurfaceManagerCoverageTest, should_return_null_when_surface_is_missing)
{
    SurfaceManager manager;
    EXPECT_EQ(manager.FindSurface("missing"), nullptr);
    EXPECT_FALSE(manager.HasSurface("missing"));
}

TEST_F(SurfaceManagerCoverageTest, should_keep_latest_surface_and_transfer_content_handle_when_replaced)
{
    SurfaceManager manager;
    manager.SetRenderId(SURFACE_MANAGER_TEST_RENDER_ID);

    ArkUI_NodeContentHandle firstHandle = reinterpret_cast<ArkUI_NodeContentHandle>(0x1111);
    ArkUI_NodeContentHandle secondHandle = reinterpret_cast<ArkUI_NodeContentHandle>(0x2222);

    SurfaceSlot& first = manager.CreateSurface(SURFACE_MANAGER_TEST_SURFACE_A, firstHandle);
    EXPECT_EQ(first.GetContentHandle(), firstHandle);
    EXPECT_EQ(manager.GetLatestSurfaceId(), SURFACE_MANAGER_TEST_SURFACE_A);
    EXPECT_EQ(manager.GetLatestSurface(), &first);

    manager.SetContentHandle(secondHandle);
    EXPECT_EQ(manager.GetContentHandle(), secondHandle);
    EXPECT_EQ(manager.GetLatestSurface()->GetContentHandle(), secondHandle);

    SurfaceSlot& second = manager.CreateSurface(SURFACE_MANAGER_TEST_SURFACE_B);
    EXPECT_EQ(manager.GetLatestSurfaceId(), SURFACE_MANAGER_TEST_SURFACE_B);
    EXPECT_EQ(manager.GetLatestSurface(), &second);
    EXPECT_EQ(second.GetContentHandle(), nullptr);
    EXPECT_EQ(first.GetContentHandle(), nullptr);
}

TEST_F(SurfaceManagerCoverageTest, should_update_force_root_fill_for_existing_surfaces)
{
    SurfaceManager manager;
    manager.SetRenderId(SURFACE_MANAGER_TEST_RENDER_ID);

    manager.CreateSurface(SURFACE_MANAGER_TEST_SURFACE_A);
    manager.CreateSurface(SURFACE_MANAGER_TEST_SURFACE_B);

    manager.SetRootFillMode(true);
    EXPECT_EQ(manager.GetLatestSurfaceId(), SURFACE_MANAGER_TEST_SURFACE_B);
    EXPECT_TRUE(manager.HasSurface(SURFACE_MANAGER_TEST_SURFACE_A));
    EXPECT_TRUE(manager.HasSurface(SURFACE_MANAGER_TEST_SURFACE_B));

    manager.SetRootFillMode(true);
    manager.SetRootFillMode(false);
}

TEST_F(SurfaceManagerCoverageTest, should_remove_latest_surface_and_back_to_previous_one)
{
    SurfaceManager manager;
    manager.SetRenderId(SURFACE_MANAGER_TEST_RENDER_ID);

    ArkUI_NodeContentHandle handle = reinterpret_cast<ArkUI_NodeContentHandle>(0x3333);
    manager.CreateSurface(SURFACE_MANAGER_TEST_SURFACE_A);
    manager.CreateSurface(SURFACE_MANAGER_TEST_SURFACE_B, handle);

    EXPECT_TRUE(manager.Back());
    EXPECT_EQ(manager.GetLatestSurfaceId(), SURFACE_MANAGER_TEST_SURFACE_A);
    EXPECT_EQ(manager.GetSurfaceCount(), 1u);
    EXPECT_EQ(manager.GetContentHandle(), nullptr);
    EXPECT_TRUE(manager.Back());
    EXPECT_TRUE(manager.GetLatestSurfaceId().empty());
    EXPECT_FALSE(manager.Back());

    manager.RemoveSurface(SURFACE_MANAGER_TEST_SURFACE_A);
    EXPECT_EQ(manager.GetSurfaceCount(), 0u);
    EXPECT_TRUE(manager.GetLatestSurfaceId().empty());
    EXPECT_EQ(manager.GetLatestSurface(), nullptr);
}

TEST_F(SurfaceManagerCoverageTest, should_ignore_remove_for_missing_surface)
{
    SurfaceManager manager;
    manager.SetRenderId(SURFACE_MANAGER_TEST_RENDER_ID);
    manager.RemoveSurface("missing");
    EXPECT_EQ(manager.GetSurfaceCount(), 0u);
    EXPECT_FALSE(manager.Back());
}

TEST_F(SurfaceManagerCoverageTest, should_accept_content_handle_when_no_surface_is_active)
{
    SurfaceManager manager;
    manager.SetRenderId(SURFACE_MANAGER_TEST_RENDER_ID);

    ArkUI_NodeContentHandle handle = reinterpret_cast<ArkUI_NodeContentHandle>(0x5555);
    manager.SetContentHandle(handle);
    EXPECT_EQ(manager.GetContentHandle(), handle);
    EXPECT_EQ(manager.GetLatestSurface(), nullptr);
}
