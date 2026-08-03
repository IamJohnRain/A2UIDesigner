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
import { A2UIValueType, SchemaProvider } from './Types';
import { DynamicValueResolver } from './DynamicValueResolver';

/**
 * Structure for reporting exceptions by custom functions
 */
export type FunctionErrorReporter = (errorMessage: string) => void;

/**
 * Contextual information during the process of custom function handling
 */
export interface FunctionContext {
  /**
   * Resolver bound to the current function invocation context.
   */
  resolver: DynamicValueResolver;

  /**
   * Error messages during the process of custom function handling
   */
  onError: FunctionErrorReporter;
}

/**
 * Placeholder type describing a public local-function implementation binding.
 */
export type FunctionCall = (params: A2UIValueType, context: FunctionContext) => A2UIValueType;

/**
 * Local function registration metadata contributed by application code.
 */
export interface ClientFunction {
  /**
   * Stable function name referenced by incoming A2UI messages.
   */
  name: string;

  /**
   * Callback that returns the JSON schema for this function.
   */
  schemaProvider: SchemaProvider;

  /**
   * Optional implementation descriptor for the local function.
   */
  functionCall: FunctionCall;
}
