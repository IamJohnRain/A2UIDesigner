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

import { Catalog } from './Catalog';
import { SurfaceErrorCode, SurfaceEventType } from './Types';
import { Breakpoint, ThemeMode } from './Types';

/**
 * Callback invoked when a user action should be sent back to the host application.
 */
export type ActionReceiver = (action: string) => void;

/**
 * Callback invoked when the renderer reports an error.
 */
export type ErrorCallback = (code: SurfaceErrorCode, errorMsg: string) => void;

/**
 * Schema warning information.
 */
export interface SchemaWarningInfo {
  /** Error code delivered to ErrorCallback. */
  code: SurfaceErrorCode;

  /** error Msg. */
  errorMsg: string;
}

/**
 * Callback invoked after a surface lifecycle event has completed.
 */
export type SurfaceEventCallback = (
  eventType: SurfaceEventType,
  controller: SurfaceController,
  surfaceId: string
) => void;

/**
 * Options used when creating a surface controller instance.
 */
export interface SurfaceControllerOption {
  /**
   * UIContext that from @ohos.arkui.UIContext.
   */
  uiContext: UIContext;

  /**
   * Catalog that defines the available components and functions.
   */
  catalog: Catalog;

  /**
   * Lifecycle callback invoked after surface operations complete.
   */
  eventCallback?: SurfaceEventCallback;
}

/**
 * Controller that receives A2UI messages and manages a rendered surface.
 *
 * Create instances through `SurfaceControllerFactory.createSurfaceController`.
 */
export interface SurfaceController {
  /**
   * Releases the controller and any bound renderer resources.
   */
  destroy(): void;

  /**
   * Sends an A2UI DSL payload to the renderer.
   *
   * @param dsl Serialized A2UI message payload.
   */
  handleMessage(dsl: string): void;

  /**
   * Enables or disables schema warning reporting during message processing.
   *
   * @param enableSchemaWarningReport Whether schema warnings should be reported.
   */
  enableSchemaWarningReport(enableSchemaWarningReport: boolean): void;

  /**
   * Registers the callback used to receive outbound actions.
   *
   * @param onAction Action receiver callback.
   */
  registerActionReceiver(onAction: ActionReceiver): void;

  /**
   * Registers the callback used to receive renderer errors.
   *
   * @param onError Error callback.
   */
  registerErrorCallback(onError: ErrorCallback): void;

  /**
   * Reports a schema warning from a custom component.
   *
   * @param schemaWarningInfo Warning information delivered through ErrorCallback.
   */
  reportSchemaWarning(schemaWarningInfo: SchemaWarningInfo): void;

  /**
   * Updates the font scale used when rendering the current surface.
   *
   * @param scale Font scaling factor applied by the renderer.
   */
  setFontSizeScale(scale: number): void;

  /**
   * Updates the active theme mode for the current surface.
   *
   * @param mode Theme mode to apply.
   */
  updateThemeMode(mode: ThemeMode): void;
}
