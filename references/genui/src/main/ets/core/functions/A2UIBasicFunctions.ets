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

import { BuiltinFunctionBase } from './BuiltinFunctionBase';
import { LocalFunctionHandler } from './LocalFunction';
import { openUrlFunction } from './OpenUrlFunction';
import { getCheckboxGroupValuesFunction } from './component/GetCheckboxGroupValuesFunction';
import { getRadioValueFunction } from './component/GetRadioValueFunction';
import { getSelectValueFunction } from './component/GetSelectValueFunction';
import { getToggleValueFunction } from './component/GetToggleValueFunction';
import { formatCurrencyFunction } from './format/FormatCurrencyFunction';
import { formatDateFunction } from './format/FormatDateFunction';
import { formatNumberFunction } from './format/FormatNumberFunction';
import { formatStringFunction } from './format/FormatStringFunction';
import { pluralizeFunction } from './format/PluralizeFunction';
import { andFunction } from './logic/AndFunction';
import { notFunction } from './logic/NotFunction';
import { orFunction } from './logic/OrFunction';
import { navigateFunction } from './extended/NavigateFunction';
import { emailFunction } from './validation/EmailFunction';
import { lengthFunction } from './validation/LengthFunction';
import { numericFunction } from './validation/NumericFunction';
import { regexFunction } from './validation/RegexFunction';
import { requiredFunction } from './validation/RequiredFunction';
import { InnerFunctionItem } from '../base/FunctionItem';

const standardBuiltinFunctions: BuiltinFunctionBase[] = [
  openUrlFunction,
  requiredFunction,
  regexFunction,
  lengthFunction,
  numericFunction,
  emailFunction,
  formatStringFunction,
  formatNumberFunction,
  formatCurrencyFunction,
  formatDateFunction,
  pluralizeFunction,
  andFunction,
  orFunction,
  notFunction
];

const extendedCatalogFunctions: BuiltinFunctionBase[] = [
  getRadioValueFunction,
  getCheckboxGroupValuesFunction,
  getToggleValueFunction,
  getSelectValueFunction,
  navigateFunction
];

function getAllBuiltinFunctions(): BuiltinFunctionBase[] {
  return standardBuiltinFunctions.concat(extendedCatalogFunctions);
}

function getRuntimeBuiltinFunctions(): BuiltinFunctionBase[] {
  return standardBuiltinFunctions.concat([
    getRadioValueFunction,
    getCheckboxGroupValuesFunction,
    getToggleValueFunction,
    getSelectValueFunction
  ]);
}

export function allA2UIBasicFunctions(): InnerFunctionItem[] {
  return standardBuiltinFunctions.map((builtinFunction: BuiltinFunctionBase) => builtinFunction.asFunctionItem());
}

export function allA2UIExtendedFunctions(): InnerFunctionItem[] {
  return getAllBuiltinFunctions().map((builtinFunction: BuiltinFunctionBase) => builtinFunction.asFunctionItem());
}

export function applyHostContextToBuiltinFunctions(hostContext: common.UIAbilityContext | undefined): void {
  const builtinFunctions: BuiltinFunctionBase[] = getAllBuiltinFunctions();
  for (const builtinFunction of builtinFunctions) {
    builtinFunction.setHostContext(hostContext);
  }
}

export function registerA2UIBuiltinFunctionHandlers(
  register: (name: string, handler: LocalFunctionHandler) => void
): void {
  const builtinFunctions: BuiltinFunctionBase[] = getRuntimeBuiltinFunctions();
  for (const builtinFunction of builtinFunctions) {
    register(builtinFunction.asFunctionItem().name, builtinFunction.createHandler());
  }
}
