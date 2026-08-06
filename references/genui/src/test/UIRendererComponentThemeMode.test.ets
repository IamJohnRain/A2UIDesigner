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

import { UIContext } from '@kit.ArkUI';
import { afterEach, beforeEach, describe, expect, it, xit } from '@ohos/hypium';
import { CatalogFactory } from '../main/ets/interface/Factories';
import { SurfaceController as PublicSurfaceController, SurfaceControllerOption } from '../main/ets/interface/SurfaceController';
import { SurfaceEventType } from '../main/ets/interface/Types';
import { NativeEngineBridge, ThemeMode } from '../main/ets/core/base/NativeEngineBridge';
import { SurfaceControllerImpl as SurfaceController } from '../main/ets/core/base/SurfaceControllerImpl';

interface ThemeModeUpdateRecord {
  renderId: number;
  mode: ThemeMode;
}

interface NativeEngineBridgePatchTarget {
  updateThemeMode: (renderId: number, mode: ThemeMode) => void;
}

const EMPTY_UI_CONTEXT: UIContext = {} as UIContext;

function createSurfaceController(): SurfaceController {
  const option: SurfaceControllerOption = {
    uiContext: EMPTY_UI_CONTEXT,
    catalog: CatalogFactory.basic(),
    eventCallback: (_eventType: SurfaceEventType, _controller: PublicSurfaceController): void => {
    }
  };
  return new SurfaceController(option);
}

export default function UIRendererComponentThemeModeUnitTest() {
  describe('UIRendererComponentThemeModeTest', () => {
    let originalUpdateThemeMode: (renderId: number, mode: ThemeMode) => void =
      NativeEngineBridge.updateThemeMode;
    let themeModeUpdates: ThemeModeUpdateRecord[] = [];

    beforeEach(() => {
      themeModeUpdates = [];
      (NativeEngineBridge as Object as NativeEngineBridgePatchTarget).updateThemeMode =
        (renderId: number, mode: ThemeMode): void => {
          themeModeUpdates.push({
            renderId: renderId,
            mode: mode
          });
        };
    });

    afterEach(() => {
      (NativeEngineBridge as Object as NativeEngineBridgePatchTarget).updateThemeMode = originalUpdateThemeMode;
    });

    xit('should_follow_system_theme_before_manual_override', 0, () => {
      let controller = createSurfaceController();

      controller.updateAndCheckThemeMode(ThemeMode.DARK);

      expect(controller.getThemeMode()).assertEqual(ThemeMode.DARK);
      expect(themeModeUpdates.length).assertEqual(1);
      expect(themeModeUpdates[0].mode).assertEqual(ThemeMode.DARK);
      controller.destroy();
    });

    xit('should_ignore_system_theme_updates_after_manual_override', 0, () => {
      let controller = createSurfaceController();

      controller.updateThemeMode(ThemeMode.DARK);
      controller.updateAndCheckThemeMode(ThemeMode.LIGHT);

      expect(controller.getThemeMode()).assertEqual(ThemeMode.DARK);
      expect(themeModeUpdates.length).assertEqual(1);
      expect(themeModeUpdates[0].mode).assertEqual(ThemeMode.DARK);
      controller.destroy();
    });

    xit('should_skip_native_update_when_manual_mode_matches_current_mode', 0, () => {
      let controller = createSurfaceController();
      controller.updateThemeMode(ThemeMode.LIGHT);

      expect(controller.getThemeMode()).assertEqual(ThemeMode.LIGHT);
      expect(themeModeUpdates.length).assertEqual(0);
      controller.destroy();
    });

    xit('should_allow_multiple_manual_theme_changes', 0, () => {
      let controller = createSurfaceController();

      controller.updateThemeMode(ThemeMode.DARK);
      controller.updateThemeMode(ThemeMode.LIGHT);

      expect(controller.getThemeMode()).assertEqual(ThemeMode.LIGHT);
      expect(themeModeUpdates.length).assertEqual(2);
      expect(themeModeUpdates[0].mode).assertEqual(ThemeMode.DARK);
      expect(themeModeUpdates[1].mode).assertEqual(ThemeMode.LIGHT);
      controller.destroy();
    });
  });
}
