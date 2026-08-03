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

import { A2UIValueType } from './Types';

/**
 * Receives resolved dynamic values for a custom component binding.
 *
 * @param param Latest value resolved from the bound path or function call.
 */
export type ResolveValueCallback<T> = (param: T) => void;

/**
 * Resolves dynamic A2UI values in the current custom component or local function context.
 */
export interface DynamicValueResolver {
  /**
   * Resolves the provided dynamic value and delivers updates through the callback.
   *
   * @param key Logical property key, for example `styles.color`.
   * @param value Dynamic value descriptor to resolve.
   * @param callback Callback invoked with the current and future resolved values.
   */
  resolvePropertyValue<T>(
    key: string,
    value: A2UIValueType,
    callback: ResolveValueCallback<T>
  ): void;

  /**
   * Casts the given A2UIValueType to the specified generic type T.
   *
   * @param value - The A2UI value to be cast.
   * @returns The value casted as type T.
   */
  evaluateValue<T>(value: A2UIValueType): T;
}
