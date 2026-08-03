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
  'Web'
];

const EXTENDED_FUNCTION_NAMES: string[] = [
  'getRadioValue',
  'getCheckboxGroupValues',
  'getToggleValue',
  'getSelectValue',
  'navigate'
];

function getCatalogItemNames(items: Array<CatalogItem | InnerFunctionItem>): string[] {
  let names: string[] = [];
  for (const item of items) {
    names.push(item.name);
  }
  return names;
}

export default function CatalogUnitTest() {
  describe('CatalogTest', () => {
    it('should_keep_basic_extend_flag_as_empty_extended_protocol_catalog', 0, () => {
      let catalog: CatalogImpl = new CatalogImpl(A2UI_EXTENDED_CATALOG_ID, [], []);

      expect(catalog.getCatalogId()).assertEqual(A2UI_EXTENDED_CATALOG_ID);
      expect(catalog.components.length).assertEqual(0);
      expect(catalog.functions.length).assertEqual(0);
    });

    it('should_create_extended_catalog_with_extended_components_and_extended_functions', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let componentNames: string[] = getCatalogItemNames(catalog.components);
      let functionNames: string[] = getCatalogItemNames(catalog.functions);
      let basicFunctionNames: string[] = getCatalogItemNames(CatalogFactoryCore.basic().functions);

      expect(catalog.getCatalogId()).assertEqual(A2UI_EXTENDED_CATALOG_ID);
      expect(catalog.components.length).assertEqual(EXTENDED_COMPONENT_NAMES.length);
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

    it('should_exclude_extended_interaction_functions_from_basic_catalog', 0, () => {
      let functionNames: string[] = getCatalogItemNames(CatalogFactoryCore.basic().functions);

      for (const functionName of EXTENDED_FUNCTION_NAMES) {
        expect(functionNames.indexOf(functionName)).assertEqual(-1);
      }
    });
  });
}
