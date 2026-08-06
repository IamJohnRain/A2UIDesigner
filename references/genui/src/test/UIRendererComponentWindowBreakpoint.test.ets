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
import { Breakpoint, NativeEngineBridge } from '../main/ets/core/base/NativeEngineBridge';
import { SurfaceControllerImpl as SurfaceController } from '../main/ets/core/base/SurfaceControllerImpl';
import { BreakpointUtils } from '../main/ets/core/common/BreakpointUtils';
import { resolveAreaLengthVpForBreakpoint } from '../main/ets/core/components/UIRendererComponentCore';

interface BreakpointUpdateRecord {
  renderId: number;
  breakpoint: Breakpoint;
}

interface NativeEngineBridgePatchTarget {
  updateBreakpoint: (renderId: number, breakpoint: Breakpoint) => void;
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

export default function UIRendererComponentBreakpointUnitTest() {
  describe('UIRendererComponentBreakpointTest', () => {
    let originalUpdateBreakpoint: (renderId: number, breakpoint: Breakpoint) => void =
      NativeEngineBridge.updateBreakpoint;
    let breakpointUpdates: BreakpointUpdateRecord[] = [];

    beforeEach(() => {
      breakpointUpdates = [];
      (NativeEngineBridge as Object as NativeEngineBridgePatchTarget).updateBreakpoint =
        (renderId: number, breakpoint: Breakpoint): void => {
          breakpointUpdates.push({
            renderId: renderId,
            breakpoint: breakpoint
          });
        };
    });

    afterEach(() => {
      (NativeEngineBridge as Object as NativeEngineBridgePatchTarget).updateBreakpoint = originalUpdateBreakpoint;
    });

    it('should_resolve_container_length_vp_and_guard_invalid_values', 0, () => {
      expect(resolveAreaLengthVpForBreakpoint(360)).assertEqual(360);
      expect(resolveAreaLengthVpForBreakpoint('360')).assertEqual(360);
      expect(resolveAreaLengthVpForBreakpoint('360vp')).assertEqual(360);
      expect(resolveAreaLengthVpForBreakpoint(0) === undefined).assertTrue();
      expect(resolveAreaLengthVpForBreakpoint(NaN) === undefined).assertTrue();
      expect(resolveAreaLengthVpForBreakpoint(undefined) === undefined).assertTrue();
      expect(resolveAreaLengthVpForBreakpoint('') === undefined).assertTrue();
      expect(resolveAreaLengthVpForBreakpoint('100%') === undefined).assertTrue();
      expect(resolveAreaLengthVpForBreakpoint('240px') === undefined).assertTrue();
    });

    it('should_resolve_breakpoint_for_each_threshold_range', 0, () => {
      expect(BreakpointUtils.resolveBreakpoint(319.9)).assertEqual(Breakpoint.XS);
      expect(BreakpointUtils.resolveBreakpoint(320)).assertEqual(Breakpoint.SM);
      expect(BreakpointUtils.resolveBreakpoint(600)).assertEqual(Breakpoint.MD);
      expect(BreakpointUtils.resolveBreakpoint(840)).assertEqual(Breakpoint.LG);
      expect(BreakpointUtils.resolveBreakpoint(1440)).assertEqual(Breakpoint.XL);
    });

    xit('should_skip_invalid_widths_and_update_breakpoint_for_valid_ranges', 0, () => {
      const controller: SurfaceController = createSurfaceController();

      controller.updateBreakpointByWidth(NaN);
      controller.updateBreakpointByWidth(0);
      controller.updateBreakpointByWidth(-1);
      expect(breakpointUpdates.length).assertEqual(0);

      controller.updateBreakpointByWidth(200);
      controller.updateBreakpointByWidth(400);
      controller.updateBreakpointByWidth(700);
      controller.updateBreakpointByWidth(1000);
      controller.updateBreakpointByWidth(1600);

      expect(breakpointUpdates.length).assertEqual(5);
      expect(breakpointUpdates[0].breakpoint).assertEqual(Breakpoint.XS);
      expect(breakpointUpdates[1].breakpoint).assertEqual(Breakpoint.SM);
      expect(breakpointUpdates[2].breakpoint).assertEqual(Breakpoint.MD);
      expect(breakpointUpdates[3].breakpoint).assertEqual(Breakpoint.LG);
      expect(breakpointUpdates[4].breakpoint).assertEqual(Breakpoint.XL);
      controller.destroy();
    });

  });
}
