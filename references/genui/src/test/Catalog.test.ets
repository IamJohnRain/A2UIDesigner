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

import { describe, it, expect } from '@ohos/hypium';
import { CatalogImpl } from '../main/ets/core/base/CatalogImpl';
import { CatalogFactoryCore } from '../main/ets/core/base/FactoriesCore';
import { A2UI_EXTENDED_CATALOG_ID } from '../main/ets/core/base/CapabilitiesCore';
import { CatalogItem } from '../main/ets/core/base/CatalogItem';
import { InnerFunctionItem } from '../main/ets/core/base/FunctionItem';
import { ClientFunction, FunctionContext } from '../main/ets/interface/ClientFunction';
import { A2UIValueType } from '../main/ets/interface/Types';
import { SchemaValidator } from '../main/ets/core/schema/SchemaValidator';

const BASIC_COMPONENT_NAMES: string[] = [
  'Button',
  'Text',
  'TextField',
  'Column',
  'Modal',
  'Row',
  'Card',
  'CheckBox',
  'Image',
  'List',
  'Slider',
  'Icon',
  'Video',
  'AudioPlayer',
  'Tabs',
  'Divider',
  'ChoicePicker',
  'DateTimeInput'
];

const EXTENDED_COMPONENT_NAMES: string[] = [
  'Button',
  'Text',
  'TextInput',
  'Row',
  'Column',
  'List',
  'Stack',
  'Grid',
  'Image',
  'Divider',
  'Toggle',
  'Progress',
  'Radio',
  'Checkbox',
  'CheckboxGroup',
  'TabContent',
  'Select',
  'NavContainer',
  'Tabs',
  'Web',
  'If'
];

const EXTENDED_FUNCTION_NAMES: string[] = [
  'getRadioValue',
  'getCheckboxGroupValues',
  'getToggleValue',
  'getSelectValue',
  'setDataModel',
  'setAttributes',
  'break',
  'navigate'
];

function getCatalogItemNames(items: Array<CatalogItem | InnerFunctionItem>): string[] {
  let names: string[] = [];
  for (const item of items) {
    names.push(item.name);
  }
  return names;
}

function hasDuplicateNames(names: string[]): boolean {
  for (let index: number = 0; index < names.length; index++) {
    if (names.indexOf(names[index]) !== index) {
      return true;
    }
  }
  return false;
}

export default function CatalogUnitTest() {
  describe('CatalogTest', () => {
    it('should_keep_basic_extend_flag_as_empty_extended_protocol_catalog', 0, () => {
      let catalog: CatalogImpl = new CatalogImpl(A2UI_EXTENDED_CATALOG_ID, [], []);

      expect(catalog.getCatalogId()).assertEqual(A2UI_EXTENDED_CATALOG_ID);
      expect(catalog.components.length).assertEqual(0);
      expect(catalog.functions.length).assertEqual(0);
    });

    it('should_create_basic_catalog_with_exactly_18_unique_components', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.basic();
      let componentNames: string[] = catalog.getAllCatalogItemNames();

      expect(componentNames.length).assertEqual(BASIC_COMPONENT_NAMES.length);
      expect(hasDuplicateNames(componentNames)).assertFalse();
      for (const componentName of BASIC_COMPONENT_NAMES) {
        expect(componentNames.indexOf(componentName) >= 0).assertTrue();
      }
    });

    it('should_create_extended_catalog_with_extended_components_and_extended_functions', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let componentNames: string[] = getCatalogItemNames(catalog.components);
      let functionNames: string[] = getCatalogItemNames(catalog.functions);
      let basicFunctionNames: string[] = getCatalogItemNames(CatalogFactoryCore.basic().functions);

      expect(catalog.getCatalogId()).assertEqual(A2UI_EXTENDED_CATALOG_ID);
      expect(catalog.components.length).assertEqual(EXTENDED_COMPONENT_NAMES.length);
      expect(hasDuplicateNames(componentNames)).assertFalse();
      expect(catalog.functions.length).assertEqual(basicFunctionNames.length + EXTENDED_FUNCTION_NAMES.length);
      for (const componentName of EXTENDED_COMPONENT_NAMES) {
        expect(componentNames.indexOf(componentName) >= 0).assertTrue();
      }
      for (const functionName of basicFunctionNames) {
        expect(functionNames.indexOf(functionName) >= 0).assertTrue();
      }
      for (const functionName of EXTENDED_FUNCTION_NAMES) {
        expect(functionNames.indexOf(functionName) >= 0).assertTrue();
      }
    });

    it('should_replace_duplicate_component_names_during_catalog_construction', 0, () => {
      let basicCatalog: CatalogImpl = CatalogFactoryCore.basic();
      let firstItem: CatalogItem = basicCatalog.components[0];
      let catalog: CatalogImpl = new CatalogImpl('duplicate-test', [firstItem, firstItem], []);

      expect(catalog.components.length).assertEqual(1);
      expect(catalog.getAllCatalogItemNames()[0]).assertEqual(firstItem.name);
    });

    it('should_exclude_extended_interaction_functions_from_basic_catalog', 0, () => {
      let functionNames: string[] = getCatalogItemNames(CatalogFactoryCore.basic().functions);

      for (const functionName of EXTENDED_FUNCTION_NAMES) {
        expect(functionNames.indexOf(functionName)).assertEqual(-1);
      }
    });

    it('should_wrap_client_function_args_schema_without_dropping_arguments', 0, () => {
      let clientFunction: ClientFunction = {
        name: 'formatName',
        schemaProvider: (_version: string): string => {
          return '{"type":"object","properties":{"name":{"type":"string"}},' +
            '"required":["name"],"additionalProperties":false}';
        },
        functionCall: (params: A2UIValueType, _context: FunctionContext): A2UIValueType => params
      };
      let catalog: CatalogImpl = CatalogFactoryCore.createCatalog('client-schema', [], [clientFunction]);
      let args: Record<string, Object> = { 'name': 'Alice' };

      let result = SchemaValidator.validateAndNormalizeFunctionCall(
        'formatName',
        args,
        'string',
        catalog.functions[0]
      );

      expect(result.success).assertTrue();
      expect(result.normalizedArgs !== undefined).assertTrue();
      let normalizedArgs: Record<string, Object> = result.normalizedArgs as Record<string, Object>;
      expect(normalizedArgs['name']).assertEqual('Alice');
      let schemaText: string = catalog.functions[0].schemaProvider('v0.9');
      expect(schemaText.indexOf('"call"') >= 0).assertTrue();
      expect(schemaText.indexOf('"args"') >= 0).assertTrue();
    });

    it('should_include_text_alias_in_extended_text_schema', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let textItem: CatalogItem | undefined = undefined;
      for (const item of catalog.components) {
        if (item.name === 'Text') {
          textItem = item;
          break;
        }
      }

      expect(textItem !== undefined).assertTrue();
      let schemaText: string = (textItem as CatalogItem).schemaProvider('v0.9');
      expect(schemaText.indexOf('"text": { "$ref": "#/$defs/DynamicString" }') >= 0).assertTrue();
    });
  });
}
