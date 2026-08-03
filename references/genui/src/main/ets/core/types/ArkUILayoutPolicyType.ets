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

// ArkUI_LayoutPolicyType wire values forwarded from the C++ resolver (ApplyLayoutPolicyDimension).
export enum ArkUILayoutPolicyType {
  MATCH_PARENT = 0,
  WRAP_CONTENT = 1,
  FIX_AT_IDEAL_SIZE = 2
}

export interface LayoutPolicyPayload {
  axis: string;
  policy: number;
}
