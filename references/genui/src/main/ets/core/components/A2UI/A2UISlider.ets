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

import { CatalogCategory, CatalogItem, EMPTY_COMPONENT_BUILDER } from '../../base/CatalogItem';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { SchemaProvider } from '../../types/types';

export class A2UISlider {
  private constructor() {
  }

  private static type: string = 'Slider';

  private static schemaProvider: SchemaProvider = (version: string) => {
    return SchemaResourceLoader.loadA2UISchema(version, 'components/Slider.json');
  }

  public static asCatalogItem(): CatalogItem {
    let catalogItem =
      CatalogItem.forComponent(A2UISlider.type, A2UISlider.schemaProvider, EMPTY_COMPONENT_BUILDER);
    catalogItem.markCategory(CatalogCategory.A2UI_STANDARD).markInnerNative(true);
    return catalogItem;
  }
}
