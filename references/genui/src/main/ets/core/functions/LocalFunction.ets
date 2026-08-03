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

import { A2UIValueType, FunctionContext } from '../types/types';

export interface LocalFunctionRequest {
  renderId?: number;
  surfaceId: string;
  componentId: string;
  functionName: string;
  args?: A2UIValueType;
  returnType: string;
  normalizeOnly?: boolean;
}

export interface LocalFunctionResponse {
  success: boolean;
  returnType: string;
  value?: A2UIValueType;
  normalizedArgs?: A2UIValueType;
  normalizedReturnType?: string;
  errorCode?: number;
  errorMessage?: string;
}

export interface LocalFunctionContext extends FunctionContext {
  surfaceId: string;
  componentId: string;
  renderId?: number;
  returnType: string;
}

export type LocalFunctionArgs = A2UIValueType;
export type LocalFunctionResult = A2UIValueType;
export type LocalFunctionHandler = (args: A2UIValueType, context: LocalFunctionContext) => LocalFunctionResult;
