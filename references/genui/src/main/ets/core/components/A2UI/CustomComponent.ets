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

import { ComponentContent, NodeContent } from '@kit.ArkUI';
import { Breakpoint, ThemeMode } from '../../base/NativeEngineBridge';
import {
  ComponentTheme as PublicComponentTheme,
  CustomComponentAttribute as PublicCustomComponentAttribute
} from '../../types/types';

export interface CommonStyleProps {
  size?: string;
  width?: number;
  height?: number;
  weight?: number;
  margin?: string;
  accessibilityLabel?: string;
  accessibilityDescription?: string;
}

export interface ComponentTheme extends PublicComponentTheme {
  primaryColor?: string;
  brandColor?: string;
  colorMode?: ThemeMode;
  breakpoint?: Breakpoint;
  darkPrimaryColor?: string;
  iconUrl?: string;
  agentDisplayName?: string;
}

export interface CustomComponentDescriptor {
  type: string;
  id?: string;
  surfaceId?: string;
  renderId?: number;
  protocolVersion?: string;
  catalogId?: string;
  customComponentHandle?: number;
  properties?: CommonStyleProps;
  componentTheme?: ComponentTheme;
  customProps?: Record<string, Object>;
  dataModelJson?: string;
}

export interface CustomComponentAttribute extends PublicCustomComponentAttribute {
  renderId?: number;
  properties?: CommonStyleProps;
  componentTheme: ComponentTheme;
  dataModelJson?: string;
  dataModel?: ESObject;
  builder?: WrappedBuilder<[CustomComponentAttribute]>;
  slot?: NodeContent;
  slots?: Map<string, NodeContent>;
}

export interface CustomComponentResult {
  content?: ComponentContent<CustomComponentAttribute>;
  childSlot?: NodeContent;
  childSlots?: Map<string, NodeContent>;
  childSlotsObject?: Record<string, NodeContent>;  // Plain object for NAPI compatibility
}

function resolveThemeColor(value: string | undefined): string | undefined {
  return typeof value === 'string' && value.length > 0 ? value : undefined;
}

export function resolveComponentBrandColor(attribute: CustomComponentAttribute | undefined): string | undefined {
  let brandColor: string | undefined = resolveThemeColor(attribute?.componentTheme?.brandColor);
  if (brandColor !== undefined) {
    return brandColor;
  }
  return resolveThemeColor(attribute?.componentTheme?.primaryColor);
}
