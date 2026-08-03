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
import { SurfaceErrorCode } from './SurfaceTypes';

export interface BackGestureController {
  getBackGestureEnabled(): boolean;

  canPop(): boolean;

  pop(): SurfaceErrorCode;
}

export type BackGestureEnabledListener = (enabled: boolean) => void;

export interface BackGestureObservableController extends BackGestureController {
  registerBackGestureEnabledListener(listener: BackGestureEnabledListener): void;

  unregisterBackGestureEnabledListener(listener: BackGestureEnabledListener): void;
}
