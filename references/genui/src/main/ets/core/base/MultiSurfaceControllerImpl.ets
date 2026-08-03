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

import { BackGestureEnabledListener } from './BackGestureTypes';
import { NativeEngineBridge } from './NativeEngineBridge';
import { SurfaceErrorCode } from './SurfaceTypes';
import { SurfaceControllerOption } from '../../interface/SurfaceController';
import { MultiSurfaceController } from '../../interface/MultiSurfaceController';
import { SurfaceControllerImpl } from './SurfaceControllerImpl';

const MULTI_SURFACE_MAX_COUNT: number = 15;

interface NativeSurfacePopResult {
  success: boolean;
  code: number;
  message?: string;
}

export class MultiSurfaceControllerImpl extends SurfaceControllerImpl implements MultiSurfaceController {
  private backGestureEnabled: boolean;
  private backGestureEnabledListeners: BackGestureEnabledListener[];

  constructor(option: SurfaceControllerOption) {
    super(option);
    this.backGestureEnabled = false;
    this.backGestureEnabledListeners = [];
  }

  protected override supportsMultipleSurfaces(): boolean {
    return true;
  }

  protected override getMaxSurfaceCount(): number {
    return MULTI_SURFACE_MAX_COUNT;
  }

  public getSurfaceList(): string[] {
    return this.getCurrentSurfaceIds();
  }

  public setBackGestureEnabled(enable: boolean): void {
    if (this.backGestureEnabled === enable) {
      console.info(
        `[MultiSurfaceController] setBackGestureEnabled noop, renderId=${this.getRenderIdForLog()}, enabled=${enable}, ` +
        `listenerCount=${this.backGestureEnabledListeners.length}`
      );
      return;
    }
    this.backGestureEnabled = enable;
    console.info(
      `[MultiSurfaceController] setBackGestureEnabled, renderId=${this.getRenderIdForLog()}, enabled=${enable}, ` +
      `listenerCount=${this.backGestureEnabledListeners.length}, surfaceCount=${this.getSurfaceList().length}`
    );
    this.notifyBackGestureEnabledChanged();
  }

  public getBackGestureEnabled(): boolean {
    return this.backGestureEnabled;
  }

  public canPop(): boolean {
    return this.getCurrentSurfaceIds().length > 1;
  }

  public override pop(): SurfaceErrorCode {
    this.logSurfaceLifecycle('pop requested');
    let code = this.popTopSurface();
    this.logSurfaceLifecycle('pop finished', `code=${code}, message=${this.getLastSurfaceMessage()}`);
    return code;
  }

  public registerBackGestureEnabledListener(listener: BackGestureEnabledListener): void {
    if (this.backGestureEnabledListeners.includes(listener)) {
      console.info(
        `[MultiSurfaceController] registerBackGestureEnabledListener ignored, renderId=${this.getRenderIdForLog()}, ` +
        `reason=duplicate listener, listenerCount=${this.backGestureEnabledListeners.length}`
      );
      return;
    }
    this.backGestureEnabledListeners.push(listener);
    console.info(
      `[MultiSurfaceController] registerBackGestureEnabledListener, renderId=${this.getRenderIdForLog()}, ` +
      `listenerCount=${this.backGestureEnabledListeners.length}`
    );
  }

  public unregisterBackGestureEnabledListener(listener: BackGestureEnabledListener): void {
    this.backGestureEnabledListeners =
      this.backGestureEnabledListeners.filter((currentListener: BackGestureEnabledListener) => currentListener !== listener);
    console.info(
      `[MultiSurfaceController] unregisterBackGestureEnabledListener, renderId=${this.getRenderIdForLog()}, ` +
      `listenerCount=${this.backGestureEnabledListeners.length}`
    );
  }

  public override destroy(): void {
    this.backGestureEnabledListeners = [];
    super.destroy();
  }

  private notifyBackGestureEnabledChanged(): void {
    this.backGestureEnabledListeners.forEach((listener: BackGestureEnabledListener) => {
      listener(this.backGestureEnabled);
    });
  }

  // Surface stack state is owned by the native SurfaceManager; this only forwards the
  // pop request and translates the native result code (which already matches SurfaceErrorCode).
  private popTopSurface(): SurfaceErrorCode {
    if (this.isDestroyed()) {
      this.recordSurfaceResult(SurfaceErrorCode.MULTI_SURFACE_EMPTY_STACK, 'controller destroyed');
      this.logSurfaceLifecycle('popTopSurface failed', 'reason=controller destroyed');
      return this.getLastSurfaceErrorCode();
    }
    if (!this.isApiSupported()) {
      this.recordSurfaceResult(SurfaceErrorCode.NATIVE_PROCESS_FAILED, this.createUnsupportedApiMessage());
      this.logSurfaceLifecycle('popTopSurface ignored', 'reason=unsupported api version');
      return this.getLastSurfaceErrorCode();
    }
    let popResult = NativeEngineBridge.popSurface(this.getRenderId());
    if (popResult === undefined) {
      this.recordSurfaceResult(SurfaceErrorCode.NATIVE_PROCESS_FAILED, 'native pop surface unavailable');
      this.logSurfaceLifecycle('popTopSurface failed', 'reason=native engine unavailable');
      return this.getLastSurfaceErrorCode();
    }
    let result = popResult as NativeSurfacePopResult;
    if (typeof result.success !== 'boolean' || typeof result.code !== 'number') {
      this.recordSurfaceResult(SurfaceErrorCode.NATIVE_PROCESS_FAILED, 'invalid native pop result');
      this.logSurfaceLifecycle('popTopSurface failed', 'reason=invalid native result');
      return this.getLastSurfaceErrorCode();
    }
    let code = result.code as SurfaceErrorCode;
    this.recordSurfaceResult(code, result.message ?? '');
    this.logSurfaceLifecycle(
      'popTopSurface native result',
      `success=${result.success}, code=${code}, message=${result.message ?? ''}`
    );
    return code;
  }
}
