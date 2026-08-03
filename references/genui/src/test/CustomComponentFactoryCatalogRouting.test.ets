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

import { afterEach, beforeEach, describe, expect, it } from '@ohos/hypium';
import {
  CustomComponentAttribute,
  CustomComponentDescriptor
} from '../main/ets/core/components/A2UI/CustomComponent';
import {
  buildCustomComponentAttribute,
  CustomComponentDefinition,
  CustomComponentFactory
} from '../main/ets/core/components/A2UI/CustomComponentFactory';
interface CustomComponentFactoryPatchTarget {
  definitions: Map<string, CustomComponentDefinition>;
}

@Builder
function customComponentFactoryPlaceholderBuilder(_attribute: CustomComponentAttribute): void {
}

function createBuilder(): WrappedBuilder<[CustomComponentAttribute]> {
  return wrapBuilder(customComponentFactoryPlaceholderBuilder);
}

function cloneDefinitions(source: Map<string, CustomComponentDefinition>): Map<string, CustomComponentDefinition> {
  let cloned: Map<string, CustomComponentDefinition> = new Map();
  source.forEach((definition: CustomComponentDefinition, type: string) => {
    cloned.set(type, definition);
  });
  return cloned;
}

function createDefinition(
  type: string,
  builder: WrappedBuilder<[CustomComponentAttribute]>,
  schemaText: string
): CustomComponentDefinition {
  return {
    type: type,
    builder: builder,
    schemaProvider: (_version: string): string => schemaText
  };
}

export default function CustomComponentFactoryCatalogRoutingUnitTest() {
  describe('CustomComponentFactoryCatalogRoutingTest', () => {
    let patchTarget: CustomComponentFactoryPatchTarget;
    let originalDefinitions: Map<string, CustomComponentDefinition>;

    beforeEach(() => {
      patchTarget = CustomComponentFactory as Object as CustomComponentFactoryPatchTarget;
      originalDefinitions = cloneDefinitions(patchTarget.definitions);
      patchTarget.definitions = new Map<string, CustomComponentDefinition>();
    });

    afterEach(() => {
      patchTarget.definitions = originalDefinitions;
    });

    it('should_build_custom_component_attribute', 0, () => {
      let descriptor: CustomComponentDescriptor = {
        type: 'Tabs',
        id: 'catalog-route-tabs',
        surfaceId: 'catalog-route-surface',
        renderId: 101
      };

      let attribute: CustomComponentAttribute = buildCustomComponentAttribute(descriptor, {
        builder: createBuilder()
      });

      expect(attribute.type).assertEqual('Tabs');
      expect(attribute.id).assertEqual('catalog-route-tabs');
      expect(attribute.surfaceId).assertEqual('catalog-route-surface');
      expect(attribute.renderId).assertEqual(101);
    });

    it('should_replace_definition_when_same_type_registered_twice', 0, () => {
      let firstBuilder = createBuilder();
      let secondBuilder = createBuilder();

      CustomComponentFactory.registerCustomComponent(createDefinition('Tabs', firstBuilder, 'first-tabs-schema'));
      CustomComponentFactory.registerCustomComponent(createDefinition('Tabs', secondBuilder, 'second-tabs-schema'));

      let routedBuilder = CustomComponentFactory.getBuilder('Tabs');
      let item = CustomComponentFactory.createCatalogItem('Tabs');

      expect(routedBuilder === secondBuilder).assertTrue();
      expect(item.name).assertEqual('Tabs');
      expect(item.schemaProvider('v0.9')).assertEqual('second-tabs-schema');
    });

    it('should_resolve_short_type_when_exact_type_is_not_registered', 0, () => {
      let navigationBuilder = createBuilder();

      CustomComponentFactory.registerCustomComponent(createDefinition('Navigation', navigationBuilder, 'navigation-schema'));

      let routedBuilder = CustomComponentFactory.getBuilder('Extended.Navigation');
      let item = CustomComponentFactory.createCatalogItem('Extended.Navigation');

      expect(routedBuilder === navigationBuilder).assertTrue();
      expect(item.name).assertEqual('Navigation');
      expect(item.schemaProvider('v0.9')).assertEqual('navigation-schema');
    });

    it('should_include_component_type_in_missing_component_error', 0, () => {
      let thrown: boolean = false;
      let message: string = '';

      try {
        CustomComponentFactory.createCatalogItem('MissingCatalogComponent');
      } catch (error) {
        thrown = true;
        message = (error as Error).message;
      }

      expect(thrown).assertTrue();
      expect(message.indexOf('MissingCatalogComponent') >= 0).assertTrue();
    });
  });
}
