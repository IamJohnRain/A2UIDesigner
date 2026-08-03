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
import { CapabilitiesCore } from '../core/base/CapabilitiesCore';

/**
 * Represents the capability manifest for GenUI.
 */
export interface CapabilityManifest {
  /**
   * List of supported standard A2UI protocol versions.
   */
  supportedA2UIProtocolVersions: string[];

  /**
   * List of supported extended protocol versions.
   */
  supportedExtendedProtocolVersions: string[];

  /**
   * List of supported catalog IDs.
   */
  supportedCatalogIds: string[];
}

/**
 * Capability query helpers exposed by the GenUI public API.
 */
export class Capabilities {
  /**
   * Returns the supported standard A2UI protocol versions.
   *
   * @returns Supported A2UI protocol version list, format as follows: ["v0.9", "v1.0"].
   */
  static getSupportedA2UIProtocolVersions(): string[] {
    return CapabilitiesCore.getSupportedA2UIProtocolVersions();
  }

  /**
   * Returns the supported extended protocol versions.
   *
   * @returns Supported extended protocol version list, format as follows: ["v0.9"].
   */
  static getSupportedExtendedProtocolVersions(): string[] {
    return CapabilitiesCore.getSupportedExtendedProtocolVersions();
  }

  /**
   * Returns the capability manifest available for GenUI.
   *
   * @returns Capability manifest.
   */
  static getCapabilities(): CapabilityManifest {
    return CapabilitiesCore.getCapabilities();
  }
}
