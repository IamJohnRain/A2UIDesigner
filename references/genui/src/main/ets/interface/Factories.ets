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

import { CatalogFactoryCore, SurfaceControllerFactoryCore } from '../core/base/FactoriesCore';
import { Catalog } from './Catalog';
import { CatalogItem } from './CatalogItem';
import { ClientFunction } from './ClientFunction';
import { MultiSurfaceController } from './MultiSurfaceController';
import { SurfaceController, SurfaceControllerOption } from './SurfaceController';

/**
 * Factory helpers for creating predefined catalog instances.
 */
export class CatalogFactory {
  /**
   * Creates the basic A2UI catalog.
   *
   * @returns Catalog configured for the basic protocol set.
   */
  static basic(): Catalog {
    return CatalogFactoryCore.basic();
  }

  /**
   * Creates the extended A2UI catalog.
   *
   * @returns Catalog configured for the extended protocol set.
   */
  static extended(): Catalog {
    return CatalogFactoryCore.extended();
  }

  /**
   * Creates a catalog with the provided identifier and initial entries.
   *
   * @param catalogId Unique identifier exposed to the A2UI runtime.
   * @param components Component registrations included in the catalog.
   * @param functions Local function registrations included in the catalog.
   * @returns Catalog instance that can be passed to a controller.
   */
  static createCatalog(catalogId: string, components: CatalogItem[], clientFunctions: ClientFunction[]): Catalog {
    return CatalogFactoryCore.createCatalog(catalogId, components, clientFunctions);
  }
}

/**
 * Factory helpers for creating surface controllers.
 */
export class SurfaceControllerFactory {
  /**
   * Creates a single-surface controller instance.
   *
   * @param option Controller initialization options.
   * @returns Controller bound to a single active surface.
   */
  static createSurfaceController(option: SurfaceControllerOption): SurfaceController {
    return SurfaceControllerFactoryCore.createSurfaceController(option);
  }

  /**
   * Creates a controller that can manage multiple stacked surfaces.
   *
   * @param option Controller initialization options.
   * @returns Controller with multi-surface navigation support.
   */
  static createMultiSurfaceController(option: SurfaceControllerOption): MultiSurfaceController {
    return SurfaceControllerFactoryCore.createMultiSurfaceController(option);
  }
}
