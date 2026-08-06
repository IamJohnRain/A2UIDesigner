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

import { Catalog } from '../../interface/Catalog';
import { CatalogItem as PublicCatalogItem } from '../../interface/CatalogItem';
import { InnerFunctionItem } from './FunctionItem';
import { FunctionCall as InternalFunctionCall, SchemaProvider } from '../types/types';
import { CatalogItem } from './CatalogItem';
import { ClientFunction } from '../../interface/ClientFunction';

export class CatalogImpl implements Catalog {
  private id: string;
  public components: CatalogItem[];
  private functionItems: Map<string, InnerFunctionItem> = new Map<string, InnerFunctionItem>();

  constructor(
    id: string,
    components: CatalogItem[],
    functions: InnerFunctionItem[],
  ) {
    this.id = id;
    this.components = CatalogImpl.normalizeComponentItems(components);
    this.functionItems = CatalogImpl.normalizeFunctionItems(functions);
  }

  public get functions(): InnerFunctionItem[] {
    const items: InnerFunctionItem[] = [];
    this.functionItems.forEach((item: InnerFunctionItem) => {
      items.push(item);
    });
    return items;
  }

  public static fromPublicOrEmpty(catalog: Catalog | null | undefined): CatalogImpl {
    if (catalog instanceof CatalogImpl) {
      return catalog;
    }
    console.error('[CatalogImpl] Catalog is not an instance of CatalogImpl, returning empty catalog.');
    return new CatalogImpl('', [], []);
  }

  public static fromClientFunctions(
    id: string,
    components: PublicCatalogItem[],
    clientFunctions: ClientFunction[]
  ): CatalogImpl {
    const catalog = new CatalogImpl(id, [], []);
    components.forEach((component: PublicCatalogItem) => {
      catalog.addCatalogItem(component);
    });
    clientFunctions.forEach((clientFunction: ClientFunction) => {
      catalog.addClientFunction(clientFunction);
    });
    return catalog;
  }

  public getCatalogId(): string {
    return this.id;
  }

  addCatalogItem(catalogItem: PublicCatalogItem): boolean {
    const normalizedItem: CatalogItem | undefined = CatalogImpl.normalizeComponentItem(catalogItem);
    if (normalizedItem === undefined) {
      return false;
    }
    CatalogImpl.addOrReplaceItem(this.components, normalizedItem);
    return true;
  }

  removeCatalogItem(name: string): boolean {
    return CatalogImpl.removeItemByName(this.components, name);
  }

  hasCatalogItem(name: string): boolean {
    return this.components.find((item: CatalogItem) => item.name === name) !== undefined;
  }

  getAllCatalogItemNames(): string[] {
    return this.components.map((item: CatalogItem) => item.name);
  }

  addClientFunction(clientFunction: ClientFunction): boolean {
    const normalizedItem: InnerFunctionItem | undefined = CatalogImpl.normalizeClientFunctionItem(clientFunction);
    if (normalizedItem === undefined) {
      return false;
    }
    this.functionItems.set(normalizedItem.name, normalizedItem);
    return true;
  }

  removeClientFunction(name: string): boolean {
    if (!this.functionItems.has(name)) {
      return false;
    }
    this.functionItems.delete(name);
    return true;
  }

  hasClientFunction(name: string): boolean {
    return this.functionItems.has(name);
  }

  getAllClientFunctionNames(): string[] {
    const names: string[] = [];
    this.functionItems.forEach((_item: InnerFunctionItem, name: string) => {
      names.push(name);
    });
    return names;
  }

  private static addOrReplaceItem(items: CatalogItem[], item: CatalogItem): void {
    let index: number = items.findIndex((current: CatalogItem) => current.name === item.name);
    if (index >= 0) {
      items[index] = item;
      return;
    }
    items.push(item);
  }

  private static removeItemByName(items: CatalogItem[], name: string): boolean {
    let index: number = items.findIndex((item: CatalogItem) => item.name === name);
    if (index < 0) {
      return false;
    }
    items.splice(index, 1);
    return true;
  }

  private static normalizeComponentItems(items: CatalogItem[]): CatalogItem[] {
    const normalizedItems: CatalogItem[] = [];
    items.forEach((item: CatalogItem) => {
      if (item !== undefined && item !== null && item.name.length > 0) {
        CatalogImpl.addOrReplaceItem(normalizedItems, item);
      }
    });
    return normalizedItems;
  }

  private static normalizeFunctionItems(items: InnerFunctionItem[]): Map<string, InnerFunctionItem> {
    const normalizedItems: Map<string, InnerFunctionItem> = new Map<string, InnerFunctionItem>();
    items.forEach((item: InnerFunctionItem) => {
      const normalizedItem: InnerFunctionItem | undefined = CatalogImpl.normalizeFunctionItem(item);
      if (normalizedItem !== undefined) {
        normalizedItems.set(normalizedItem.name, normalizedItem);
      }
    });
    return normalizedItems;
  }

  private static normalizeComponentItem(item: PublicCatalogItem | null | undefined): CatalogItem | undefined {
    return CatalogItem.fromComponentItem(item);
  }

  private static normalizeFunctionItem(item: InnerFunctionItem): InnerFunctionItem | undefined {
    if (item === undefined || item === null || item.name.length === 0) {
      return undefined;
    }
    const functionCall: InternalFunctionCall = (item.functionCall as InternalFunctionCall) ??
      ((_params, _context) => undefined);
    return {
      name: item.name,
      isInnerNative: item.isInnerNative ?? false,
      schemaProvider: item.schemaProvider,
      functionCall: functionCall
    };
  }

  private static normalizeClientFunctionItem(item: ClientFunction): InnerFunctionItem | undefined {
    if (item === undefined || item === null || item.name.length === 0) {
      return undefined;
    }
    return {
      name: item.name,
      isInnerNative: false,
      schemaProvider: CatalogImpl.wrapClientFunctionSchemaProvider(item.name, item.schemaProvider),
      functionCall: item.functionCall
    };
  }

  private static wrapClientFunctionSchemaProvider(name: string, schemaProvider: SchemaProvider): SchemaProvider {
    return (version: string): string => {
      const schemaText: string = schemaProvider(version).trim();
      if (schemaText.length === 0 || CatalogImpl.isFullFunctionSchema(schemaText)) {
        return schemaText;
      }

      let argsSchema: Object;
      try {
        argsSchema = JSON.parse(schemaText) as Object;
      } catch (_error) {
        return schemaText;
      }

      const callSchema: Record<string, Object> = { 'const': name };
      const returnTypeSchema: Record<string, Object> = {
        'type': 'string',
        'enum': ['string', 'number', 'boolean', 'array', 'object', 'null', 'any', 'void'],
        'default': 'any'
      };
      const properties: Record<string, Object> = {
        'call': callSchema,
        'args': argsSchema,
        'returnType': returnTypeSchema
      };
      const wrapper: Record<string, Object> = {
        'type': 'object',
        'properties': properties,
        'required': ['call', 'args'],
        'additionalProperties': false
      };
      const argsRecord: Record<string, Object> = argsSchema as Record<string, Object>;
      if (argsRecord['$defs'] !== undefined) {
        wrapper['$defs'] = argsRecord['$defs'];
      }
      if (argsRecord['definitions'] !== undefined) {
        wrapper['definitions'] = argsRecord['definitions'];
      }
      return JSON.stringify(wrapper);
    };
  }

  private static isFullFunctionSchema(schemaText: string): boolean {
    try {
      const schema: Record<string, Object> = JSON.parse(schemaText) as Record<string, Object>;
      const propertiesValue: Object | undefined = schema['properties'];
      if (propertiesValue === undefined || propertiesValue === null || Array.isArray(propertiesValue)) {
        return false;
      }
      const properties: Record<string, Object> = propertiesValue as Record<string, Object>;
      return properties['call'] !== undefined && properties['args'] !== undefined;
    } catch (_error) {
      return false;
    }
  }
}
