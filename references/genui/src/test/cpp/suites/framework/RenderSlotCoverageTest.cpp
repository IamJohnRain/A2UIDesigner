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

#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

class RenderSlotCoverageTest : public A2UITest {};

TEST_F(RenderSlotCoverageTest, should_delegate_content_handle_and_fill_mode_to_surface_manager)
{
    RenderSlot slot(820100);
    ArkUI_NodeContentHandle handle = reinterpret_cast<ArkUI_NodeContentHandle>(0x4444);

    slot.SetContentHandle(handle);
    EXPECT_EQ(slot.GetContentHandle(), handle);

    slot.SetRootFillMode(true);
    EXPECT_EQ(slot.GetContentHandle(), handle);
}

TEST_F(RenderSlotCoverageTest, should_return_null_when_surface_manager_is_reset)
{
    RenderSlot slot(820101);
    EXPECT_NE(slot.GetSurfaceManager(), nullptr);
    slot.Dispose();
    EXPECT_EQ(slot.GetSurfaceManager(), nullptr);
    EXPECT_EQ(slot.GetContentHandle(), nullptr);
}

TEST_F(RenderSlotCoverageTest, should_share_api_version_through_system_properties_singleton)
{
    SystemProperties::GetInstance().SetApiVersion(0);
    SystemProperties::GetInstance().SetApiVersion(27);

    RenderSlot slot(820102);
    SurfaceManager manager;
    SurfaceSlot surface;

    EXPECT_EQ(slot.GetApiVersion(), 27);
    EXPECT_EQ(manager.GetApiVersion(), 27);
    EXPECT_EQ(surface.GetApiVersion(), 27);

    SystemProperties::GetInstance().SetApiVersion(0);
    slot.SetApiVersion(31);
    EXPECT_EQ(SystemProperties::GetInstance().GetApiVersion(), 27);
    EXPECT_EQ(manager.GetApiVersion(), 27);
    EXPECT_EQ(surface.GetApiVersion(), 27);
}
