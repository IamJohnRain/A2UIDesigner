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
import { SurfaceController } from './SurfaceController';
import { SurfaceErrorCode } from './Types';


/**
 * Controller that manages a stack of surfaces instead of a single active surface.
 *
 * Create instances through `SurfaceControllerFactory.createMultiSurfaceController`.
 */
export interface MultiSurfaceController extends SurfaceController {
  /**
   * Returns whether the controller can navigate back to a previous surface.
   *
   * @returns `true` when more than one surface is currently managed.
   */
  canPop(): boolean;

  /**
   * Removes the latest surface and returns the operation status code.
   *
   * @returns Status code of the pop operation.// 增加错误码说明
   */
  pop(): SurfaceErrorCode;

  /**
   * Returns the identifier of the latest managed surface.
   *
   * @returns Latest surface identifier.
   */
  getLatestSurfaceId(): string;

  /**
   * Returns the identifiers of all managed surfaces in stack order.
   *
   * @returns Managed surface identifier list.
   */
  getSurfaceList(): string[];

  /**
   * Enables or disables back-gesture navigation.
   *
   * @param enable Whether the back gesture should be enabled.
   */
  setBackGestureEnabled(enable: boolean): void;

  /**
   * Returns whether back-gesture navigation is enabled.
   *
   * @returns `true` when the back gesture is enabled.
   */
  getBackGestureEnabled(): boolean;
}
