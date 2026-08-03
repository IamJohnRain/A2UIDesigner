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

import { CatalogImpl } from './CatalogImpl';
import { A2UI_BASIC_CATALOG_ID, A2UI_EXTENDED_CATALOG_ID } from './CapabilitiesCore';
import { allA2UIBasicComponents } from '../components/A2UI/A2UIBasicComponents';
import { allA2UIBasicFunctions } from '../functions/A2UIBasicFunctions';
import { allA2UIExtendedComponents } from '../components/A2UI/A2UIExtendedComponents';
import { allA2UIExtendedFunctions } from '../functions/A2UIBasicFunctions';
import { SurfaceControllerImpl } from './SurfaceControllerImpl';
import { MultiSurfaceControllerImpl } from './MultiSurfaceControllerImpl';
import { SurfaceControllerOption } from '../../interface/SurfaceController';
import { CatalogItem as PublicCatalogItem } from '../../interface/CatalogItem';
import { ClientFunction } from '../../interface/ClientFunction';

export class CatalogFactoryCore {
  static basic(): CatalogImpl {
    return new CatalogImpl(
      A2UI_BASIC_CATALOG_ID,
      allA2UIBasicComponents(),
      allA2UIBasicFunctions()
    );
  }

  static extended(): CatalogImpl {
    return new CatalogImpl(
      A2UI_EXTENDED_CATALOG_ID,
      allA2UIExtendedComponents(),
      allA2UIExtendedFunctions()
    );
  }

  static createCatalog(
    catalogId: string,
    components: PublicCatalogItem[],
    clientFunctions: ClientFunction[]): CatalogImpl {
    return CatalogImpl.fromClientFunctions(catalogId, components, clientFunctions);
  }
}

export class SurfaceControllerFactoryCore {
  static createSurfaceController(option: SurfaceControllerOption): SurfaceControllerImpl {
    return new SurfaceControllerImpl(option);
  }

  static createMultiSurfaceController(option: SurfaceControllerOption): MultiSurfaceControllerImpl {
    return new MultiSurfaceControllerImpl(option);
  }
}
