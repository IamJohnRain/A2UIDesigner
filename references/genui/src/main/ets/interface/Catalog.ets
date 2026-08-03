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
import { CatalogItem } from './CatalogItem';
import { ClientFunction } from './ClientFunction';

/**
 * Public catalog declaration used to register custom components and local functions.
 */
export interface Catalog {
  /**
   * Adds a component definition or replaces the existing one with the same name.
   *
   * @param catalogItem Component registration to store in the catalog.
   * @returns `true` when the operation succeeds.
   */
  addCatalogItem(catalogItem: CatalogItem): boolean;

  /**
   * Removes a component definition by name.
   *
   * @param name Component name used in the A2UI DSL.
   * @returns `true` when the component was removed.
   */
  removeCatalogItem(name: string): boolean;

  /**
   * Checks whether a component definition exists in the catalog.
   *
   * @param name Component name used in the A2UI DSL.
   * @returns `true` when the component is registered.
   */
  hasCatalogItem(name: string): boolean;

  /**
   * Returns the names of all registered component definitions.
   *
   * @returns Component name list in the current catalog.
   */
  getAllCatalogItemNames(): string[];

  /**
   * Adds a local function definition or replaces the existing one with the same name.
   *
   * @param functionItem Function registration to store in the catalog.
   * @returns `true` when the operation succeeds.
   */
  addClientFunction(clientFunction: ClientFunction): boolean;

  /**
   * Removes a local function definition by name.
   *
   * @param name Function name used in the A2UI DSL.
   * @returns `true` when the function was removed.
   */
  removeClientFunction(name: string): boolean;

  /**
   * Checks whether a local function definition exists in the catalog.
   *
   * @param name Function name used in the A2UI DSL.
   * @returns `true` when the function is registered.
   */
  hasClientFunction(name: string): boolean;

  /**
   * Returns the names of all registered local functions.
   *
   * @returns Function name list in the current catalog.
   */
  getAllClientFunctionNames(): string[];
}
