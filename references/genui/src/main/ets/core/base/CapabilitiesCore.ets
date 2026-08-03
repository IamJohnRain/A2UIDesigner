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

export const A2UI_BASIC_CATALOG_ID: string = 'https://a2ui.org/specification/v0_9/catalogs/basic/catalog.json';
export const A2UI_EXTENDED_CATALOG_ID: string = 'ohos.a2ui.extended.catalog';

export interface CapabilityManifestCore {
  supportedA2UIProtocolVersions: string[];
  supportedExtendedProtocolVersions: string[];
  supportedCatalogIds: string[];
}

// Keep in sync with genui/src/main/cpp/SurfaceContext.h.
const SUPPORTED_A2UI_PROTOCOL_VERSIONS: string[] = ['v0.9'];
const SUPPORTED_EXTENDED_PROTOCOL_VERSIONS: string[] = ['v1.0.0'];
const DEFAULT_A2UI_PROTOCOL_VERSION: string = 'v0.9';

export class CapabilitiesCore {
  private constructor() {
  }

  public static getSupportedA2UIProtocolVersions(): string[] {
    return SUPPORTED_A2UI_PROTOCOL_VERSIONS.slice();
  }

  public static getSupportedExtendedProtocolVersions(): string[] {
    return SUPPORTED_EXTENDED_PROTOCOL_VERSIONS.slice();
  }

  public static getCapabilities(): CapabilityManifestCore {
    const manifest: CapabilityManifestCore = {
      supportedA2UIProtocolVersions: CapabilitiesCore.getSupportedA2UIProtocolVersions(),
      supportedExtendedProtocolVersions: CapabilitiesCore.getSupportedExtendedProtocolVersions(),
      supportedCatalogIds: [
        A2UI_BASIC_CATALOG_ID,
        A2UI_EXTENDED_CATALOG_ID
      ]
    };
    return manifest;
  }

  public static resolveA2UIProtocolVersion(version?: string): string | null {
    if (version === undefined) {
      return CapabilitiesCore.getDefaultA2UIProtocolVersion();
    }
    if (CapabilitiesCore.isSupportedA2UIProtocolVersion(version)) {
      return version.trim();
    }
    console.error(`[CapabilitiesCore] Unsupported A2UI protocol version: ${version}`);
    return null;
  }

  public static getDefaultA2UIProtocolVersion(): string {
    return DEFAULT_A2UI_PROTOCOL_VERSION;
  }

  public static isSupportedA2UIProtocolVersion(version: string): boolean {
    return SUPPORTED_A2UI_PROTOCOL_VERSIONS.indexOf(version.trim()) >= 0;
  }
}
