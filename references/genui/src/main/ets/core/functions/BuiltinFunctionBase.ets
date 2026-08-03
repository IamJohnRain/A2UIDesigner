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

import { common } from '@kit.AbilityKit';

import { SchemaResourceLoader } from '../schema/SchemaResourceLoader';
import { SchemaProvider } from '../types/types';
import { LocalFunctionArgs, LocalFunctionContext, LocalFunctionHandler, LocalFunctionResult } from './LocalFunction';
import { InnerFunctionItem } from '../base/FunctionItem';

export class BuiltinFunctionBase {
  private readonly name: string;

  constructor(name: string) {
    this.name = name;
  }

  public asFunctionItem(): InnerFunctionItem {
    return {
      name: this.name,
      isInnerNative: false,
      schemaProvider: this.schemaProvider(),
      functionCall: undefined
    };
  }

  public createHandler(): LocalFunctionHandler {
    return (_args: LocalFunctionArgs, _context: LocalFunctionContext): LocalFunctionResult => {
      return undefined;
    };
  }

  public setHostContext(_hostContext: common.UIAbilityContext | undefined): void {
  }

  protected schemaProvider(): SchemaProvider {
    return (_version: string) => '';
  }

  protected loadFunctionSchema(version: string, fileName: string): string {
    return SchemaResourceLoader.loadA2UISchema(version, `functions/${fileName}`);
  }

  protected loadExtendedFunctionSchema(fileName: string): string {
    return SchemaResourceLoader.loadSchema(`schema/Extended/functions/${fileName}`);
  }
}
