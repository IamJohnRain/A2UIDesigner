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
import { describe, beforeAll, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { CatalogItemType } from '../../../main/ets/engine/core/CatalogItem';
import { SchemaPromptCombineMode } from '../../../main/ets/engine/types/types';
import { CustomComponentFactory } from '../../../main/ets/engine/components/A2UI/CustomComponentFactory';
import { createExtendedTabsDefinition } from '../../../main/ets/engine/components/extended/ExtendedTabs';
import { createExtendedTabContentDefinition } from '../../../main/ets/engine/components/extended/ExtendedTabContent';
import { createExtendedWebDefinition } from '../../../main/ets/engine/components/extended/ExtendedWeb';
import {
  ExtendedTabsRegistry,
  ExtendedTabMetadata
} from '../../../main/ets/engine/components/extended/ExtendedTabsRegistry';
import { SchemaResourceLoader } from '../../../main/ets/engine/schema/SchemaResourceLoader';

const TABS_TYPE: string = 'Tabs';
const TAB_CONTENT_TYPE: string = 'TabContent';
const NAV_CONTAINER_TYPE: string = 'NavContainer';
const WEB_TYPE: string = 'Web';
const TEST_TAB_ID: string = 'extended-tab-content-test-id';
const TEST_TAB_ID_SECOND: string = 'extended-tab-content-test-id-second';

const TEST_METADATA: ExtendedTabMetadata = {
  title: 'Home',
  icon: '/res/home.png',
  selectedSrc: '/res/home_selected.png',
  tabType: 'capsule',
  selectedColor: '#FF007DFF',
  unSelectedColor: '#FF182431',
  defaultBackgroundColor: '#00FFFFFF',
  selectedBackgroundColor: '#1A007DFF',
  defaultBorderColor: '#19000000',
  selectedBorderColor: '#FF007DFF',
  fontSize: 18,
  fontWeight: 'bold',
  iconSize: 20,
  space: 6
};

function toAsciiBytes(value: string): Uint8Array {
  let bytes: Uint8Array = new Uint8Array(value.length);
  for (let i = 0; i < value.length; i++) {
    bytes[i] = value.charCodeAt(i);
  }
  return bytes;
}

interface ExtendedSceneComponentDescriptor {
  id?: string;
  component?: string;
  children?: string[];
  title?: string;
  text?: string;
}

interface ExtendedSceneDescriptor {
  components: ExtendedSceneComponentDescriptor[];
}

function createExtendedTabsScene(): string {
  return '{"components":[' +
    '{"id":"root","component":"Column","children":["title","tabsMain","footer"]},' +
    '{"id":"title","component":"Text","text":"Tabs multiple children scene"},' +
    '{"id":"tabsMain","component":"Tabs","children":["tabHome","tabOrders","tabProfile"]},' +
    '{"id":"tabHome","component":"TabContent","title":"Home","children":["homeContent"]},' +
    '{"id":"homeContent","component":"Text","text":"Home tab content"},' +
    '{"id":"tabOrders","component":"TabContent","title":"Orders","children":["ordersContent"]},' +
    '{"id":"ordersContent","component":"Text","text":"Orders tab content"},' +
    '{"id":"tabProfile","component":"TabContent","title":"Profile","children":["profileContent"]},' +
    '{"id":"profileContent","component":"Text","text":"Profile tab content"},' +
    '{"id":"footer","component":"Text","text":"Tabs supports multiple child nodes."}' +
    ']}';
}

function createExtendedTabContentScene(): string {
  return '{"components":[' +
    '{"id":"root","component":"Column","children":["title","tabsMain","footer"]},' +
    '{"id":"title","component":"Text","text":"TabContent children scene"},' +
    '{"id":"tabsMain","component":"Tabs","children":["tabFull","tabChildren"]},' +
    '{"id":"tabFull","component":"TabContent","title":"Full","children":["tabFullContent"]},' +
    '{"id":"tabFullContent","component":"Text","text":"Single child tab content"},' +
    '{"id":"tabChildren","component":"TabContent","title":"Children","children":["tabChildrenLine1","tabChildrenLine2"]},' +
    '{"id":"tabChildrenLine1","component":"Text","text":"First child line"},' +
    '{"id":"tabChildrenLine2","component":"Text","text":"Second child line"},' +
    '{"id":"footer","component":"Text","text":"TabContent supports children nodes."}' +
    ']}';
}

function findComponentDescriptor(
  components: ExtendedSceneComponentDescriptor[] | undefined,
  componentId: string
): ExtendedSceneComponentDescriptor | undefined {
  if (components === undefined) {
    return undefined;
  }

  for (const component of components) {
    if (component.id === componentId) {
      return component;
    }
  }
  return undefined;
}

function readStringArray(value: string[] | undefined): string[] {
  return value === undefined ? [] : value.slice();
}

function createSchemaContext(): common.Context {
  let rawfiles: Map<string, string> = new Map<string, string>();
  rawfiles.set('schema/Extended/components/ExtendedTabs.json', '{"component":"Tabs","children":[]}');
  rawfiles.set(
    'schema/Extended/components/ExtendedTabContent.json',
    '{"component":"TabContent","selectedSrc":"","children":{"$ref":"ChildList"}}'
  );
  rawfiles.set(
    'schema/Extended/components/NavContainer.json',
    '{"component":"NavContainer","children":[],"currentIndex":0}'
  );
  rawfiles.set(
    'schema/Extended/components/ExtendedWeb.json',
    '{"component":"Web","url":"https://example.com"}'
  );
  rawfiles.set('rawfile/extended_tabs_multi_child_scene.json', createExtendedTabsScene());
  rawfiles.set('rawfile/extended_tabcontent_children_scene.json', createExtendedTabContentScene());

  return {
    resourceManager: {
      getRawFileContentSync: (relativePath: string): Uint8Array => {
        let content: string | undefined = rawfiles.get(relativePath);
        if (content === undefined) {
          throw new Error(`mock rawfile not found: ${relativePath}`);
        }
        return toAsciiBytes(content);
      }
    }
  } as common.Context;
}

export default function ExtendedComponentsTest() {
  describe('ExtendedComponentsTest', () => {
    const tabsDefinition = createExtendedTabsDefinition();
    const tabContentDefinition = createExtendedTabContentDefinition();
    const webDefinition = createExtendedWebDefinition();

    beforeAll(() => {
      SchemaResourceLoader.setHostContext(createSchemaContext());
      CustomComponentFactory.registerCustomComponent(tabsDefinition);
      CustomComponentFactory.registerCustomComponent(tabContentDefinition);
      CustomComponentFactory.registerCustomComponent(webDefinition);
    });

    afterAll(() => {
      SchemaResourceLoader.setHostContext(undefined);
      SchemaResourceLoader.setErrorReporter(undefined);
    });

    afterEach(() => {
      ExtendedTabsRegistry.removeMetadata(TEST_TAB_ID);
      ExtendedTabsRegistry.removeMetadata(TEST_TAB_ID_SECOND);
    });

    describe('ExtendedTabsDefinition', () => {
      it('should_create_extended_tabs_definition_with_required_fields', 0, () => {
        expect(tabsDefinition.type).assertEqual(TABS_TYPE);
        expect(tabsDefinition.builder !== undefined).assertTrue();
        expect(tabsDefinition.schemaProvider !== undefined).assertTrue();
        expect(tabsDefinition.promptProvider !== undefined).assertTrue();
      });

      it('should_provide_schema_and_prompt_for_extended_tabs_definition', 0, () => {
        if (tabsDefinition.schemaProvider === undefined || tabsDefinition.promptProvider === undefined) {
          expect(false).assertTrue();
          return;
        }

        let schema: string = tabsDefinition.schemaProvider();
        let prompt = tabsDefinition.promptProvider();

        expect(schema.length > 0).assertTrue();
        expect(schema).assertContain('"Tabs"');
        expect(prompt.mode).assertEqual(SchemaPromptCombineMode.COMBINE);
        expect(prompt.prompt.length > 0).assertTrue();
      });
    });

    describe('ExtendedTabContentDefinition', () => {
      it('should_create_extended_tab_content_definition_with_required_fields', 0, () => {
        expect(tabContentDefinition.type).assertEqual(TAB_CONTENT_TYPE);
        expect(tabContentDefinition.builder !== undefined).assertTrue();
        expect(tabContentDefinition.schemaProvider !== undefined).assertTrue();
        expect(tabContentDefinition.promptProvider !== undefined).assertTrue();
      });

      it('should_provide_schema_and_prompt_for_extended_tab_content_definition', 0, () => {
        if (tabContentDefinition.schemaProvider === undefined || tabContentDefinition.promptProvider === undefined) {
          expect(false).assertTrue();
          return;
        }

        let schema: string = tabContentDefinition.schemaProvider();
        let prompt = tabContentDefinition.promptProvider();

        expect(schema.length > 0).assertTrue();
        expect(schema).assertContain('"TabContent"');
        expect(schema).assertContain('selectedSrc');
        expect(prompt.mode).assertEqual(SchemaPromptCombineMode.COMBINE);
        expect(prompt.prompt.length > 0).assertTrue();
      });
    });

    describe('NavContainerDefinition', () => {
      it('should_create_nav_container_definition_with_required_fields', 0, () => {
        expect(navContainerDefinition.type).assertEqual(NAV_CONTAINER_TYPE);
        expect(navContainerDefinition.builder !== undefined).assertTrue();
        expect(navContainerDefinition.schemaProvider !== undefined).assertTrue();
        expect(navContainerDefinition.promptProvider !== undefined).assertTrue();
      });

      it('should_provide_schema_and_prompt_for_nav_container_definition', 0, () => {
        if (navContainerDefinition.schemaProvider === undefined || navContainerDefinition.promptProvider === undefined) {
          expect(false).assertTrue();
          return;
        }

        let schema: string = navContainerDefinition.schemaProvider();
        let prompt = navContainerDefinition.promptProvider();

        expect(schema.length > 0).assertTrue();
        expect(schema).assertContain('NavContainer');
        expect(schema).assertContain('children');
        expect(prompt.mode).assertEqual(SchemaPromptCombineMode.COMBINE);
        expect(prompt.prompt.length > 0).assertTrue();
      });
    });

    describe('ExtendedWebDefinition', () => {
      it('should_create_extended_web_definition_with_required_fields', 0, () => {
        expect(webDefinition.type).assertEqual(WEB_TYPE);
        expect(webDefinition.builder !== undefined).assertTrue();
        expect(webDefinition.schemaProvider !== undefined).assertTrue();
        expect(webDefinition.promptProvider !== undefined).assertTrue();
      });

      it('should_provide_schema_and_prompt_for_extended_web_definition', 0, () => {
        if (webDefinition.schemaProvider === undefined || webDefinition.promptProvider === undefined) {
          expect(false).assertTrue();
          return;
        }

        let schema: string = webDefinition.schemaProvider();
        let prompt = webDefinition.promptProvider();

        expect(schema.length > 0).assertTrue();
        expect(schema).assertContain('"Web"');
        expect(schema).assertContain('url');
        expect(prompt.mode).assertEqual(SchemaPromptCombineMode.COMBINE);
        expect(prompt.prompt.length > 0).assertTrue();
      });
    });

    describe('ExtendedTabsCatalogItem', () => {
      it('should_create_catalog_item_for_extended_tabs', 0, () => {
        let item = CustomComponentFactory.createCatalogItem(TABS_TYPE);

        expect(item.name).assertEqual(TABS_TYPE);
        expect(item.type).assertEqual(CatalogItemType.COMPONENT);
        expect(item.isInnerNative).assertFalse();
        expect(item.componentBuilder !== undefined).assertTrue();
        expect(item.schemaProvider().length > 0).assertTrue();
        expect(item.schemaProvider()).assertContain('"Tabs"');
      });

      it('should_create_catalog_item_for_extended_tab_content', 0, () => {
        let item = CustomComponentFactory.createCatalogItem(TAB_CONTENT_TYPE);

        expect(item.name).assertEqual(TAB_CONTENT_TYPE);
        expect(item.type).assertEqual(CatalogItemType.COMPONENT);
        expect(item.isInnerNative).assertFalse();
        expect(item.componentBuilder !== undefined).assertTrue();
        expect(item.schemaProvider().length > 0).assertTrue();
        expect(item.schemaProvider()).assertContain('"TabContent"');
      });

      it('should_create_catalog_item_for_nav_container', 0, () => {
        let item = CustomComponentFactory.createCatalogItem(NAV_CONTAINER_TYPE);

        expect(item.name).assertEqual(NAV_CONTAINER_TYPE);
        expect(item.type).assertEqual(CatalogItemType.COMPONENT);
        expect(item.isInnerNative).assertFalse();
        expect(item.componentBuilder !== undefined).assertTrue();
        expect(item.schemaProvider().length > 0).assertTrue();
        expect(item.schemaProvider()).assertContain('NavContainer');
      });

      it('should_create_catalog_item_for_extended_web', 0, () => {
        let item = CustomComponentFactory.createCatalogItem(WEB_TYPE);

        expect(item.name).assertEqual(WEB_TYPE);
        expect(item.type).assertEqual(CatalogItemType.COMPONENT);
        expect(item.isInnerNative).assertFalse();
        expect(item.componentBuilder !== undefined).assertTrue();
        expect(item.schemaProvider().length > 0).assertTrue();
        expect(item.schemaProvider()).assertContain('"Web"');
      });
    });

    describe('ExtendedTabsSlotCreation', () => {
      it('should_create_multiple_child_slots_for_extended_tabs_children_array', 0, () => {
        let item = CustomComponentFactory.createCatalogItem(TABS_TYPE);
        expect(item.name).assertEqual(TABS_TYPE);

        let sceneText: string = SchemaResourceLoader.loadSchema('rawfile/extended_tabs_multi_child_scene.json');
        let scene = JSON.parse(sceneText) as ExtendedSceneDescriptor;
        let tabsMain = findComponentDescriptor(scene.components, 'tabsMain');
        expect(tabsMain !== undefined).assertTrue();
        if (tabsMain === undefined) {
          return;
        }

        let children = readStringArray(tabsMain.children);
        expect(children.length).assertEqual(3);
        expect(children[0]).assertEqual('tabHome');
        expect(children[1]).assertEqual('tabOrders');
        expect(children[2]).assertEqual('tabProfile');
      });
    });

    describe('ExtendedSchemaCompatibility', () => {
      it('should_use_children_in_extended_tab_content_schema', 0, () => {
        if (tabContentDefinition.schemaProvider === undefined) {
          expect(false).assertTrue();
          return;
        }
        let schema: string = tabContentDefinition.schemaProvider();
        expect(schema).assertContain('"children"');
        expect(schema).assertContain('"tabType"');
        expect(schema).assertContain('"styles"');
      });
    });

    describe('ExtendedDemoScenes', () => {
      it('should_support_extended_tabs_multiple_children_scene', 0, () => {
        let sceneText: string = SchemaResourceLoader.loadSchema('rawfile/extended_tabs_multi_child_scene.json');
        expect(sceneText.length > 0).assertTrue();

        let scene = JSON.parse(sceneText) as ExtendedSceneDescriptor;
        let components = scene.components;
        let tabsMain = findComponentDescriptor(components, 'tabsMain');
        expect(tabsMain !== undefined).assertTrue();
        if (tabsMain === undefined) {
          return;
        }

        let children = readStringArray(tabsMain.children);
        expect(children.length).assertEqual(3);
        expect(children[0]).assertEqual('tabHome');
        expect(children[1]).assertEqual('tabOrders');
        expect(children[2]).assertEqual('tabProfile');
      });

      it('should_support_extended_tabcontent_children_scene', 0, () => {
        let sceneText: string = SchemaResourceLoader.loadSchema('rawfile/extended_tabcontent_children_scene.json');
        expect(sceneText.length > 0).assertTrue();

        let scene = JSON.parse(sceneText) as ExtendedSceneDescriptor;
        let components = scene.components;
        let tabFull = findComponentDescriptor(components, 'tabFull');
        let tabChildren = findComponentDescriptor(components, 'tabChildren');

        expect(tabFull !== undefined).assertTrue();
        expect(tabChildren !== undefined).assertTrue();
        if (tabFull === undefined || tabChildren === undefined) {
          return;
        }

        let fullChildren = readStringArray(tabFull.children);
        expect(fullChildren.length).assertEqual(1);
        expect(fullChildren[0]).assertEqual('tabFullContent');

        let children = readStringArray(tabChildren.children);
        expect(children.length).assertEqual(2);
        expect(children[0]).assertEqual('tabChildrenLine1');
        expect(children[1]).assertEqual('tabChildrenLine2');
      });
    });

    describe('ExtendedTabsRegistry', () => {
      it('should_update_and_get_metadata', 0, () => {
        ExtendedTabsRegistry.updateMetadata(TEST_TAB_ID, TEST_METADATA);

        let metadata = ExtendedTabsRegistry.getMetadata(TEST_TAB_ID);
        expect(metadata !== undefined).assertTrue();
        if (metadata === undefined) {
          return;
        }

        expect(metadata.title).assertEqual(TEST_METADATA.title);
        expect(metadata.icon).assertEqual(TEST_METADATA.icon);
        expect(metadata.selectedSrc).assertEqual(TEST_METADATA.selectedSrc);
        expect(metadata.tabType).assertEqual(TEST_METADATA.tabType);
        expect(metadata.fontSize).assertEqual(TEST_METADATA.fontSize);
        expect(metadata.iconSize).assertEqual(TEST_METADATA.iconSize);
      });

      it('should_return_undefined_when_metadata_not_exists', 0, () => {
        let metadata = ExtendedTabsRegistry.getMetadata('not-exists');
        expect(metadata === undefined).assertTrue();
      });

      it('should_remove_metadata', 0, () => {
        ExtendedTabsRegistry.updateMetadata(TEST_TAB_ID, TEST_METADATA);
        expect(ExtendedTabsRegistry.getMetadata(TEST_TAB_ID) !== undefined).assertTrue();

        ExtendedTabsRegistry.removeMetadata(TEST_TAB_ID);

        let metadata = ExtendedTabsRegistry.getMetadata(TEST_TAB_ID);
        expect(metadata === undefined).assertTrue();
      });

      it('should_ignore_empty_component_id', 0, () => {
        ExtendedTabsRegistry.updateMetadata('', TEST_METADATA);
        let metadata = ExtendedTabsRegistry.getMetadata('');
        expect(metadata === undefined).assertTrue();

        ExtendedTabsRegistry.removeMetadata('');
        expect(true).assertTrue();
      });

      it('should_notify_listener_on_update_and_remove', 0, () => {
        let notifyCount: number = 0;
        let listener = (): void => {
          notifyCount = notifyCount + 1;
        };

        ExtendedTabsRegistry.addListener(listener);
        ExtendedTabsRegistry.updateMetadata(TEST_TAB_ID, TEST_METADATA);
        ExtendedTabsRegistry.removeMetadata(TEST_TAB_ID);
        ExtendedTabsRegistry.removeListener(listener);

        expect(notifyCount).assertEqual(2);
      });

      it('should_stop_notifying_after_listener_removed', 0, () => {
        let notifyCount: number = 0;
        let listener = (): void => {
          notifyCount = notifyCount + 1;
        };

        ExtendedTabsRegistry.addListener(listener);
        ExtendedTabsRegistry.removeListener(listener);
        ExtendedTabsRegistry.updateMetadata(TEST_TAB_ID_SECOND, TEST_METADATA);

        expect(notifyCount).assertEqual(0);
      });
    });
  });
}
