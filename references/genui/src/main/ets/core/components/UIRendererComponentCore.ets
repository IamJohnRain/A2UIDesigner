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

import { SurfaceController } from '../../interface/SurfaceController';
import { SurfaceErrorCode } from '../../interface/Types';
import { ThemeMode } from '../types/types';
import { SurfaceControllerImpl } from '../base/SurfaceControllerImpl';
import { BackGestureEnabledListener } from '../base/BackGestureTypes';
import { CustomComponentDefinition, CustomComponentFactory } from '../components/A2UI/CustomComponentFactory';
import { CrossLanguageAttributeBridge } from '../functions/CrossLanguageAttributeBridge';
import { FunctionBridge } from '../functions/FunctionBridge';
import { NativeEngineBridge } from '../base/NativeEngineBridge';
import { SchemaResourceLoader } from '../schema/SchemaResourceLoader';
import { createExtendedRowDefinition } from '../components/extended/ExtendedRow';
import { display, NodeContent } from '@kit.ArkUI';
import { common, EnvironmentCallback, Configuration, ConfigurationConstant } from '@kit.AbilityKit';
import { deviceInfo } from '@kit.BasicServicesKit';
import { i18n } from '@kit.LocalizationKit';

const BUILT_IN_CUSTOM_COMPONENT_DEFINITIONS: CustomComponentDefinition[] = [
  createExtendedRowDefinition()
];

export function resolveAreaLengthVpForBreakpoint(value: Length | undefined): number | undefined {
  if (typeof value === 'number') {
    if (!Number.isFinite(value) || value <= 0) {
      return undefined;
    }
    return value;
  }
  if (typeof value !== 'string') {
    return undefined;
  }
  let normalizedValue = value.trim();
  if (normalizedValue.length === 0) {
    return undefined;
  }
  let matched = normalizedValue.match(/^(-?\d+(?:\.\d+)?)(vp)?$/);
  if (matched === null) {
    return undefined;
  }
  let resolvedValue = Number(matched[1]);
  if (!Number.isFinite(resolvedValue) || resolvedValue <= 0) {
    return undefined;
  }
  return resolvedValue;
}

interface DisplayDensityNativeEngine {
  setDisplayDensity?: (renderId: number, densityPixels: number, fpToVpScale?: number) => void;
}

@Component
export struct UIRendererComponentCore {
  surfaceController: SurfaceController | null = null;

  private getSurfaceControllerBound(): SurfaceControllerImpl | null {
    return SurfaceControllerImpl.fromPublicOrNull(this.surfaceController);
  }
  isRootFillContainer: boolean = false;
  @State private backGestureDirection: PanDirection = PanDirection.None;
  private rootSlot: NodeContent = new NodeContent();
  private readonly backGestureTriggerDistance: number = 80;
  private renderCheck: boolean = true;
  private static readonly GET_ID_MIN_SUPPORTED_API_VERSION: number = 22;
  private backGestureEnabledListener?: BackGestureEnabledListener;
  private registeredBackGestureController: SurfaceControllerImpl | null = null;
  private environmentCallbackId: number | undefined = undefined;
  private containerWidthVp: number | undefined = undefined;

  private resolveThemeModeFromSystemColorMode(
    colorMode: ConfigurationConstant.ColorMode | undefined
  ): ThemeMode {
    return colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK ?
      ThemeMode.DARK : ThemeMode.LIGHT;
  }

  private environmentCallback: EnvironmentCallback = {
    onConfigurationUpdated: (config: Configuration): void => {
      this.getSurfaceControllerBound()?.updateAndCheckThemeMode(
        this.resolveThemeModeFromSystemColorMode(config.colorMode));
      this.syncBreakpointFromContainer('environment');
      let renderId = this.getSurfaceControllerBound()?.getRenderId() ?? -1;
      this.pushDisplayDensity(renderId);
    },
    onMemoryLevel: (_level: number): void => {
    }
  };

  private resolveHostContextSafely(): common.UIAbilityContext | undefined {
    try {
      let hostContext = this.getUIContext().getHostContext();
      if (hostContext === undefined || hostContext === null) {
        return undefined;
      }
      return hostContext as common.UIAbilityContext;
    } catch (error) {
      console.warn(`[UIRendererComponent] getHostContext failed: ${JSON.stringify(error)}`);
      return undefined;
    }
  }

  private registerEnvironmentListener(hostContext: common.UIAbilityContext): number | undefined {
    try {
      return hostContext.getApplicationContext().on('environment', this.environmentCallback);
    } catch (error) {
      console.warn(`[UIRendererComponent] add environment listener failed: ${JSON.stringify(error)}`);
      return undefined;
    }
  }

  private unregisterEnvironmentListener(): void {
    if (this.environmentCallbackId === undefined) {
      return;
    }
    let hostContext = this.resolveHostContextSafely();
    if (hostContext === undefined) {
      this.environmentCallbackId = undefined;
      return;
    }
    try {
      hostContext.getApplicationContext().off('environment', this.environmentCallbackId);
    } catch (error) {
      console.warn(`[UIRendererComponent] remove environment listener failed: ${JSON.stringify(error)}`);
    }
    this.environmentCallbackId = undefined;
  }

  private installCustomComponents(): void {
    let controller = this.getSurfaceControllerBound();
    CustomComponentFactory.InstallCustomComponent(this.getUIContext(), BUILT_IN_CUSTOM_COMPONENT_DEFINITIONS);
    if (controller === null) {
      return;
    }
    let catalog = controller.getCatalog();
    CustomComponentFactory.registerCatalogComponents(catalog.components);
  }

  aboutToAppear() {
    if (deviceInfo.sdkApiVersion >= UIRendererComponentCore.GET_ID_MIN_SUPPORTED_API_VERSION &&
      this.getSurfaceControllerBound()?.getUIContext().getId() !== this.getUIContext().getId()) {
      this.renderCheck = false;
      return;
    }
    if (!this.canUseNativeRenderer('aboutToAppear')) {
      return;
    }
    let hostContext = this.resolveHostContextSafely();
    this.installCustomComponents();
    if (hostContext !== undefined) {
      FunctionBridge.setHostContext(hostContext)
      SchemaResourceLoader.setHostContext(hostContext)
    } else {
      SchemaResourceLoader.setHostContext(undefined)
      console.warn('[UIRendererComponent] hostContext is undefined, skip host-dependent setup.');
    }
    let renderId = this.getSurfaceControllerBound()?.getRenderId() ?? -1;
    CrossLanguageAttributeBridge.setUIContext(renderId, this.getUIContext())
    this.pushDisplayDensity(renderId);
    this.getSurfaceControllerBound()?.bindToRender(this.rootSlot);
    this.getSurfaceControllerBound()?.setRootFillMode(this.isRootFillContainer);
    if (hostContext !== undefined) {
      this.getSurfaceControllerBound()?.updateAndCheckThemeMode(
        this.resolveThemeModeFromSystemColorMode(hostContext.config?.colorMode));
    }
    this.syncBreakpointFromContainer('aboutToAppear');
    this.backGestureEnabledListener = (enabled: boolean): void => {
      this.backGestureDirection = enabled ? PanDirection.Horizontal : PanDirection.None;
    };
    this.registeredBackGestureController = this.getSurfaceControllerBound();
    this.registeredBackGestureController?.registerBackGestureEnabledListener(this.backGestureEnabledListener);
    this.syncBackGestureDirection();
    if (hostContext !== undefined) {
      this.environmentCallbackId = this.registerEnvironmentListener(hostContext);
    } else {
      this.environmentCallbackId = undefined;
    }
  }

  aboutToDisappear(): void {
    let renderId = this.getSurfaceControllerBound()?.getRenderId() ?? -1;
    this.clearContainerMetrics();
    if (this.canUseNativeRenderer('aboutToDisappear')) {
      CrossLanguageAttributeBridge.clearUIContext(renderId);
      this.registeredBackGestureController?.unbindFromRender(this.rootSlot);
    }
    if (this.backGestureEnabledListener !== undefined) {
      this.registeredBackGestureController?.unregisterBackGestureEnabledListener(this.backGestureEnabledListener);
    }
    this.registeredBackGestureController = null;
    this.backGestureEnabledListener = undefined;
    this.unregisterEnvironmentListener();
  }

  private flushCrossLanguageAttributes(renderId: number): void {
    if (renderId < 0) {
      return;
    }
    if (!this.canUseNativeRenderer('flushCrossLanguageAttributes')) {
      return;
    }
    CrossLanguageAttributeBridge.setUIContext(renderId, this.getUIContext());
    CrossLanguageAttributeBridge.flushPending(renderId);
  }

  private canUseNativeRenderer(stage: string): boolean {
    let controller = this.getSurfaceControllerBound();
    if (controller === null || controller.isApiSupported()) {
      return true;
    }
    console.warn(
      `[UIRendererComponent] ${stage} skipped: apiVersion=${controller.getApiVersion()}, ` +
        `minSupported=${SurfaceControllerImpl.getMinSupportedApiVersion()}, renderId=${controller.getRenderId()}`
    );
    return false;
  }

  private clearContainerMetrics(): void {
    this.containerWidthVp = undefined;
  }

  private syncBreakpointFromContainer(reason: string): void {
    let controller = this.getSurfaceControllerBound();
    if (controller === null || this.containerWidthVp === undefined ||
      !Number.isFinite(this.containerWidthVp) || this.containerWidthVp <= 0) {
      return;
    }
    console.info(
      `[UIRendererComponent] sync breakpoint from container, renderId=${controller.getRenderId()}, ` +
        `reason=${reason}, containerWidthVp=${this.containerWidthVp}`
    );
    controller.updateBreakpointByWidth(this.containerWidthVp);
  }

  private resolveAreaLengthVp(value: Length | undefined): number | undefined {
    return resolveAreaLengthVpForBreakpoint(value);
  }

  private handleContainerAreaChange(_oldValue: SizeOptions, newValue: SizeOptions): void {
    this.containerWidthVp = this.resolveAreaLengthVp(newValue.width);
    this.syncBreakpointFromContainer('containerAreaChange');
  }

  private pushDisplayDensity(renderId: number): void {
    try {
      let densityPixels = display.getDefaultDisplaySync().densityPixels;
      if (!Number.isFinite(densityPixels) || densityPixels <= 0) {
        return;
      }
      let engineObject = NativeEngineBridge.getEngine() as Object | null;
      if (engineObject === null) {
        return;
      }
      let densityEngine = engineObject as DisplayDensityNativeEngine;
      if (typeof densityEngine.setDisplayDensity !== 'function') {
        return;
      }
      densityEngine.setDisplayDensity(renderId, densityPixels, this.resolveFpToVpScale());
    } catch (error) {
      console.warn(`UIRendererComponent pushDisplayDensity failed: ${JSON.stringify(error)}`);
    }
  }

  private resolveFpToVpScale(): number {
    let oneFpPx = fp2px(1);
    if (!Number.isFinite(oneFpPx) || oneFpPx <= 0) {
      return 0;
    }
    let oneFpVp = px2vp(oneFpPx);
    if (!Number.isFinite(oneFpVp) || oneFpVp <= 0) {
      return 0;
    }
    return oneFpVp;
  }

  private syncBackGestureDirection(): void {
    let backGestureController = this.getSurfaceControllerBound();
    this.backGestureDirection = backGestureController !== null && backGestureController.getBackGestureEnabled()
      ? PanDirection.Horizontal
      : PanDirection.None;
  }

  private getSystemLocale(): string {
    try {
      return i18n.System.getSystemLocaleInstance().toString();
    } catch (e) {
      try {
        return i18n.System.getSystemLocale();
      } catch (e2) {
        return 'zh-Hans';
      }
    }
  }

  private isMirrorLocale(): boolean {
    try {
      return i18n.isRTL(this.getSystemLocale());
    } catch (error) {
      console.warn(`UIRendererComponent resolve locale direction failed: ${error}`);
      return false;
    }
  }

  private shouldTriggerBackGesture(event: GestureEvent): boolean {
    if (Math.abs(event.offsetX) < Math.abs(event.offsetY)) {
      return false;
    }
    if (this.isMirrorLocale()) {
      return event.offsetX < -this.backGestureTriggerDistance;
    }
    return event.offsetX > this.backGestureTriggerDistance;
  }

  private tryHandleBackGesture(event: GestureEvent): void {
    let backGestureController = this.getSurfaceControllerBound();
    if (backGestureController === null || !backGestureController.getBackGestureEnabled()) {
      return;
    }
    if (!backGestureController.canPop() || !this.shouldTriggerBackGesture(event)) {
      return;
    }

    let code = backGestureController.pop();
    if (code !== SurfaceErrorCode.NO_ERROR &&
      code !== SurfaceErrorCode.MULTI_SURFACE_ONLY_ONE_SURFACE &&
      code !== SurfaceErrorCode.MULTI_SURFACE_EMPTY_STACK) {
      console.warn(
        `UIRendererComponent back gesture failed, code=${code}, message=${backGestureController.getLastSurfaceMessage()}`
      );
    }
  }

  build(): void {
    if (this.renderCheck) {
      Column() {
        ContentSlot(this.rootSlot)
      }
      .width('100%')
      .onAppear(() => {
        if (!this.canUseNativeRenderer('onAppear')) {
          return;
        }
        let renderId = this.getSurfaceControllerBound()?.getRenderId() ?? -1;
        this.flushCrossLanguageAttributes(renderId);
        this.syncBreakpointFromContainer('onAppear');
      })
      .onSizeChange((oldValue, newValue) => {
        this.handleContainerAreaChange(oldValue, newValue);
      })
      .gesture(
        PanGesture({
          direction: this.backGestureDirection,
          distance: 5
        })
          .onActionEnd((event: GestureEvent) => {
            this.tryHandleBackGesture(event);
          })
      )
    }
  }
}
