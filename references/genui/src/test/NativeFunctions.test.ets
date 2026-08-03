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

/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { InnerFunctionItem } from '../main/ets/core/base/FunctionItem';
import { requiredFunction } from '../main/ets/core/functions/validation/RequiredFunction';
import { regexFunction } from '../main/ets/core/functions/validation/RegexFunction';
import { lengthFunction } from '../main/ets/core/functions/validation/LengthFunction';
import { numericFunction } from '../main/ets/core/functions/validation/NumericFunction';
import { emailFunction } from '../main/ets/core/functions/validation/EmailFunction';
import { formatStringFunction } from '../main/ets/core/functions/format/FormatStringFunction';
import { formatNumberFunction } from '../main/ets/core/functions/format/FormatNumberFunction';
import { formatCurrencyFunction } from '../main/ets/core/functions/format/FormatCurrencyFunction';
import { formatDateFunction } from '../main/ets/core/functions/format/FormatDateFunction';
import { pluralizeFunction } from '../main/ets/core/functions/format/PluralizeFunction';
import { andFunction } from '../main/ets/core/functions/logic/AndFunction';
import { orFunction } from '../main/ets/core/functions/logic/OrFunction';
import { notFunction } from '../main/ets/core/functions/logic/NotFunction';
import { navigateFunction } from '../main/ets/core/functions/extended/NavigateFunction';
import { allA2UIBasicFunctions, allA2UIExtendedFunctions } from '../main/ets/core/functions/A2UIBasicFunctions';
import { openUrlFunction } from '../main/ets/core/functions/OpenUrlFunction';
import { BuiltinFunctionBase } from '../main/ets/core/functions/BuiltinFunctionBase';
import { getCheckboxGroupValuesFunction } from '../main/ets/core/functions/component/GetCheckboxGroupValuesFunction';
import { getRadioValueFunction } from '../main/ets/core/functions/component/GetRadioValueFunction';
import { getSelectValueFunction } from '../main/ets/core/functions/component/GetSelectValueFunction';
import { getToggleValueFunction } from '../main/ets/core/functions/component/GetToggleValueFunction';
import {
  SchemaErrorCode,
  SchemaErrorInfo,
  SchemaErrorInfoManager
} from '../main/ets/core/schema/SchemaErrorInfoManager';
import { SchemaValidator } from '../main/ets/core/schema/SchemaValidator';

const STANDARD_FUNCTION_NAMES: string[] = [
  'openUrl', 'required', 'regex', 'length', 'numeric', 'email',
  'formatString', 'formatNumber', 'formatCurrency', 'formatDate',
  'pluralize', 'and', 'or', 'not'
];

const EXTENDED_INTERACTION_FUNCTION_NAMES: string[] = [
  'getRadioValue',
  'getCheckboxGroupValues',
  'getToggleValue',
  'getSelectValue',
  'navigate'
];

const FORMAT_CURRENCY_SCHEMA_WITH_DYNAMIC_STRING_REF: string = `{
  "$defs": {
    "DynamicString": {
      "oneOf": [
        {
          "type": "string"
        },
        {
          "type": "object",
          "properties": {
            "path": {
              "type": "string"
            }
          },
          "required": [
            "path"
          ],
          "additionalProperties": false
        }
      ]
    }
  },
  "type": "object",
  "properties": {
    "call": {
      "const": "formatCurrency"
    },
    "args": {
      "type": "object",
      "properties": {
        "value": {
          "type": "number"
        },
        "currency": {
          "$ref": "#/$defs/DynamicString"
        }
      },
      "required": [
        "currency",
        "value"
      ]
    },
    "returnType": {
      "const": "string"
    }
  },
  "required": [
    "call",
    "args"
  ]
}`;

const NUMERIC_SCHEMA_WITH_ANY_OF: string = `{
  "type": "object",
  "properties": {
    "call": {
      "const": "numeric"
    },
    "args": {
      "type": "object",
      "properties": {
        "value": {
          "type": "number"
        },
        "min": {
          "type": "number"
        },
        "max": {
          "type": "number"
        }
      },
      "required": [
        "value"
      ],
      "anyOf": [
        {
          "required": [
            "min"
          ]
        },
        {
          "required": [
            "max"
          ]
        }
      ]
    },
    "returnType": {
      "const": "boolean"
    }
  },
  "required": [
    "call",
    "args"
  ]
}`;

const LENGTH_SCHEMA_WITH_ANY_OF: string = `{
  "type": "object",
  "properties": {
    "call": {
      "const": "length"
    },
    "args": {
      "type": "object",
      "properties": {
        "value": {
          "type": "string"
        },
        "min": {
          "type": "number",
          "default": 0
        },
        "max": {
          "type": "number"
        }
      },
      "required": [
        "value"
      ],
      "anyOf": [
        {
          "required": [
            "min"
          ]
        },
        {
          "required": [
            "max"
          ]
        }
      ]
    },
    "returnType": {
      "const": "boolean"
    }
  },
  "required": [
    "call",
    "args"
  ]
}`;

function countSchemaWarnings(path: string, code: SchemaErrorCode): number {
  let warnings: SchemaErrorInfo[] = SchemaErrorInfoManager.getInstance().getWarnings();
  let count: number = 0;
  for (let i = 0; i < warnings.length; i++) {
    if (warnings[i].path === path && warnings[i].code === code) {
      count++;
    }
  }
  return count;
}

function createSchemaTestFunctionItem(name: string, schemaText: string): InnerFunctionItem {
  return {
    name: name,
    isInnerNative: false,
    schemaProvider: (_version: string): string => schemaText
  };
}

export default function NativeFunctionsUnitTest() {
  describe('NativeFunctionsTest', () => {
    describe('CatalogRegistration', () => {
      it('should_register_all_14_basic_a2ui_functions_in_catalog', 0, () => {
        let items = allA2UIBasicFunctions();
        expect(items.length).assertEqual(STANDARD_FUNCTION_NAMES.length);
        let names: string[] = items.map((item) => item.name);
        for (let name of STANDARD_FUNCTION_NAMES) {
          expect(names.indexOf(name) >= 0).assertTrue();
        }
      });

      it('should_mark_native_functions_with_inner_native_true', 0, () => {
        let nativeNames: string[] = [
          'openUrl', 'required', 'regex', 'length', 'numeric', 'email',
          'formatString', 'formatNumber', 'formatCurrency', 'formatDate', 'pluralize',
          'and', 'or', 'not'
        ];
        let items = allA2UIBasicFunctions();
        for (let item of items) {
          if (nativeNames.indexOf(item.name) >= 0) {
            expect(item.isInnerNative).assertTrue();
          }
        }
      });

      it('should_have_unique_names_for_all_functions', 0, () => {
        let items = allA2UIBasicFunctions();
        let names: string[] = items.map((item) => item.name);
        let uniqueNames: Set<string> = new Set(names);
        expect(uniqueNames.size).assertEqual(names.length);
      });
    });

    describe('RequiredFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = requiredFunction.asFunctionItem();
      });

      it('required_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('required');
      });

      it('required_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });

    });

    describe('RegexFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = regexFunction.asFunctionItem();
      });
      it('regex_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('regex');
      });

      it('regex_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });

    });
    describe('LengthFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = lengthFunction.asFunctionItem();
      });
      it('length_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('length');
      });

      it('length_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('NumericFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = numericFunction.asFunctionItem();
      });
      it('numeric_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('numeric');
      });

      it('numeric_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('EmailFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = emailFunction.asFunctionItem();
      });
      it('email_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('email');
      });

      it('email_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('FormatStringFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = formatStringFunction.asFunctionItem();
      });
      it('formatString_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('formatString');
      });

      it('formatString_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('FormatNumberFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = formatNumberFunction.asFunctionItem();
      });
      it('formatNumber_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('formatNumber');
      });

      it('formatNumber_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('FormatCurrencyFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = formatCurrencyFunction.asFunctionItem();
      });
      it('formatCurrency_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('formatCurrency');
      });

      it('formatCurrency_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('FormatDateFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = formatDateFunction.asFunctionItem();
      });
      it('formatDate_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('formatDate');
      });

      it('formatDate_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('PluralizeFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = pluralizeFunction.asFunctionItem();
      });
      it('pluralize_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('pluralize');
      });

      it('pluralize_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('AndFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = andFunction.asFunctionItem();
      });
      it('and_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('and');
      });

      it('and_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('OrFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = orFunction.asFunctionItem();
      });
      it('or_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('or');
      });

      it('or_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('NotFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = notFunction.asFunctionItem();
      });
      it('not_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('not');
      });

      it('not_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });
    });
    describe('OpenUrlFunction', () => {
      let functionItem: InnerFunctionItem;

      beforeEach(() => {
        functionItem = openUrlFunction.asFunctionItem();
      });
      it('openUrl_should_have_correct_catalog_item_name', 0, () => {
        expect(functionItem.name).assertEqual('openUrl');
      });

      it('openUrl_should_be_marked_as_inner_native', 0, () => {
        expect(functionItem.isInnerNative).assertTrue();
      });

    });

    describe('SchemaAndPromptValidation', () => {
      it('should_have_schema_provider_for_all_native_functions', 0, () => {
        let functions: BuiltinFunctionBase[] = [
          openUrlFunction, requiredFunction, regexFunction, lengthFunction, numericFunction, emailFunction,
          formatStringFunction, formatNumberFunction, formatCurrencyFunction, formatDateFunction,
          pluralizeFunction, andFunction, orFunction, notFunction,
          getRadioValueFunction, getCheckboxGroupValuesFunction, getToggleValueFunction, getSelectValueFunction,
          navigateFunction
        ];
        for (let fn of functions) {
          let item: InnerFunctionItem = fn.asFunctionItem();
          expect(item.schemaProvider !== undefined).assertTrue();
        }
      });

      it('should_not_expose_ets_handlers_for_inner_native_functions', 0, () => {
        let functions: BuiltinFunctionBase[] = [
          openUrlFunction, requiredFunction, regexFunction, lengthFunction, numericFunction, emailFunction,
          formatStringFunction, formatNumberFunction, formatCurrencyFunction, formatDateFunction,
          pluralizeFunction, andFunction, orFunction, notFunction,
          getRadioValueFunction, getCheckboxGroupValuesFunction, getToggleValueFunction, getSelectValueFunction,
          navigateFunction
        ];
        for (let fn of functions) {
          let item: InnerFunctionItem = fn.asFunctionItem();
          expect(item.isInnerNative).assertTrue();
          expect(item.functionCall === undefined).assertTrue();
        }
      });
    });

    describe('OpenUrlMarkedNative', () => {
      it('should_mark_openUrl_as_inner_native', 0, () => {
        let items: InnerFunctionItem[] = allA2UIBasicFunctions();
        let openUrlItem: InnerFunctionItem | undefined = items.find((item: InnerFunctionItem) => item.name === 'openUrl');
        expect(openUrlItem !== undefined).assertTrue();
        expect(openUrlItem!.isInnerNative).assertTrue();
      });
    });

    describe('FunctionSchemaValidation', () => {
      beforeEach(() => {
        SchemaErrorInfoManager.getInstance().clear();
      });

      afterEach(() => {
        SchemaErrorInfoManager.getInstance().clear();
      });

      it('should_fill_empty_string_without_extra_warnings_when_required_dynamic_string_ref_is_missing', 0, () => {
        let functionItem: InnerFunctionItem =
          createSchemaTestFunctionItem('formatCurrency', FORMAT_CURRENCY_SCHEMA_WITH_DYNAMIC_STRING_REF);
        let formatCurrencyArgs: Record<string, Object> = new Object() as Record<string, Object>;
        formatCurrencyArgs['value'] = 88 as Object;

        let result = SchemaValidator.validateAndNormalizeFunctionCall(
          'formatCurrency',
          formatCurrencyArgs,
          'number',
          functionItem
        );
        let normalizedArgs: Record<string, Object> = result.normalizedArgs as Record<string, Object>;

        expect(result.success).assertTrue();
        expect(normalizedArgs['currency'] as string).assertEqual('');
        expect(result.normalizedReturnType).assertEqual('string');
        expect(countSchemaWarnings(
          'function.formatCurrency.args.currency',
          SchemaErrorCode.REQUIRED_MISS
        )).assertEqual(1);
        expect(countSchemaWarnings(
          'function.formatCurrency.args.currency',
          SchemaErrorCode.TYPE_MISMATCH
        )).assertEqual(0);
        expect(countSchemaWarnings(
          'function.formatCurrency.args.currency',
          SchemaErrorCode.INVALID_VALUE
        )).assertEqual(0);
      });

      it('should_not_report_parent_union_warning_when_numeric_child_type_mismatch_is_reported', 0, () => {
        let functionItem: InnerFunctionItem = createSchemaTestFunctionItem('numeric', NUMERIC_SCHEMA_WITH_ANY_OF);
        let minArgs: Record<string, Object> = new Object() as Record<string, Object>;
        minArgs['value'] = 5 as Object;
        minArgs['min'] = '0' as Object;
        minArgs['max'] = 10 as Object;
        let maxArgs: Record<string, Object> = new Object() as Record<string, Object>;
        maxArgs['value'] = 5 as Object;
        maxArgs['min'] = 0 as Object;
        maxArgs['max'] = '10' as Object;

        let minResult = SchemaValidator.validateAndNormalizeFunctionCall(
          'numeric',
          minArgs,
          'boolean',
          functionItem
        );
        let maxResult = SchemaValidator.validateAndNormalizeFunctionCall(
          'numeric',
          maxArgs,
          'boolean',
          functionItem
        );

        expect(minResult.success).assertTrue();
        expect(maxResult.success).assertTrue();
        expect(countSchemaWarnings(
          'function.numeric.args.min',
          SchemaErrorCode.TYPE_MISMATCH
        )).assertEqual(1);
        expect(countSchemaWarnings(
          'function.numeric.args.max',
          SchemaErrorCode.TYPE_MISMATCH
        )).assertEqual(1);
        expect(countSchemaWarnings(
          'function.numeric.args',
          SchemaErrorCode.INVALID_VALUE
        )).assertEqual(0);
      });

      it('should_report_parent_union_warning_when_length_branch_only_has_required_miss', 0, () => {
        let functionItem: InnerFunctionItem = createSchemaTestFunctionItem('length', LENGTH_SCHEMA_WITH_ANY_OF);
        let lengthArgs: Record<string, Object> = new Object() as Record<string, Object>;
        lengthArgs['value'] = 'abc' as Object;

        let result = SchemaValidator.validateAndNormalizeFunctionCall(
          'length',
          lengthArgs,
          'boolean',
          functionItem
        );
        let normalizedArgs: Record<string, Object> = result.normalizedArgs as Record<string, Object>;

        expect(result.success).assertTrue();
        expect(normalizedArgs['min'] as number).assertEqual(0);
        expect(countSchemaWarnings(
          'function.length.args.min',
          SchemaErrorCode.REQUIRED_MISS
        )).assertEqual(1);
        expect(countSchemaWarnings(
          'function.length.args',
          SchemaErrorCode.INVALID_VALUE
        )).assertEqual(1);
      });
    });
  });
}
