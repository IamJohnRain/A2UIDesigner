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

import { Catalog } from './Catalog';
import { CatalogImpl } from '../core/base/CatalogImpl';
import { PromptBuilder as PromptBuilderCore } from '../core/prompt/promptBuilder';

/**
 * Utility helpers for generating prompt text and schema snippets from catalog metadata.
 */
export class PromptBuilder {
  /**
   * Builds the full prompt instruction for the supplied catalog.
   *
   * @param catalog Catalog whose components and functions are exposed to the model.
   * @param version Optional protocol version override. For the basic protocol catalog,
   *                use {@link BASIC_CATALOG_PROTOCOL_VERSION_V09}; for the extended
   *                protocol catalog, use {@link EXTENDED_CATALOG_PROTOCOL_VERSION_V100}. For the basic protocol catalog,
   *                use {@link BASIC_CATALOG_PROTOCOL_VERSION_V09}; for the extended
   *                protocol catalog, use {@link EXTENDED_CATALOG_PROTOCOL_VERSION_V100}.
   *                It is recommended to import these constants from {@link ./Version}.
   * @returns Instruction text that can be sent to an AI model.
   */
  static buildInstruction(catalog: Catalog, version?: string): string {
    return PromptBuilderCore.buildInstruction(CatalogImpl.fromPublicOrEmpty(catalog), version);
  }

  /**
   * Builds the component schema section for the provided catalog items.
   *
   * @param componentItems Component registrations to describe.
   * @param version Optional protocol version override. For the basic protocol catalog,
   *                use {@link BASIC_CATALOG_PROTOCOL_VERSION_V09}; for the extended
   *                protocol catalog, use {@link EXTENDED_CATALOG_PROTOCOL_VERSION_V100}. For the basic protocol catalog,
   *                use {@link BASIC_CATALOG_PROTOCOL_VERSION_V09}; for the extended
   *                protocol catalog, use {@link EXTENDED_CATALOG_PROTOCOL_VERSION_V100}.
   * @returns Serialized component schema section.
   */
  static getComponentSchema(catalog: Catalog, version?: string): string {
    return PromptBuilderCore.getComponentSchema(CatalogImpl.fromPublicOrEmpty(catalog).components, version);
  }

  /**
   * Builds the function schema section for the provided catalog items.
   *
   * @param functionItems Function registrations to describe.
   * @param version Optional protocol version override. For the basic protocol catalog,
   *                use {@link BASIC_CATALOG_PROTOCOL_VERSION_V09}; for the extended
   *                protocol catalog, use {@link EXTENDED_CATALOG_PROTOCOL_VERSION_V100}.
   * @returns Serialized function schema section.
   */
  static getFunctionSchema(catalog: Catalog, version?: string): string {
    return PromptBuilderCore.getFunctionSchema(CatalogImpl.fromPublicOrEmpty(catalog).functions, version);
  }

  /**
   * Builds the protocol message schema section for the requested version.
   *
   * @param version Optional protocol version override. For the basic protocol catalog,
   *                use {@link BASIC_CATALOG_PROTOCOL_VERSION_V09}; for the extended
   *                protocol catalog, use {@link EXTENDED_CATALOG_PROTOCOL_VERSION_V100}.
   * @returns Serialized protocol schema section.
   */
  static getProtocolMessageSchema(version?: string): string {
    return PromptBuilderCore.getProtocolSchema(version);
  }
}
