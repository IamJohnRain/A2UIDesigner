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

import { Breakpoint, SchemaProvider, ThemeMode } from './Types';
import { DynamicValueResolver } from './DynamicValueResolver';

/**
 * Surface-level theme overrides passed to custom components.
 */
export interface ComponentTheme {
  /**
   * Primary brand color used by the current surface.
   */
  primaryColor?: string;

  /**
   * Active color mode of the current surface.
   */
  colorMode?: ThemeMode;

  /**
   * Active responsive breakpoint of the current surface.
   */
  breakpoint?: Breakpoint;

  /**
   * Optional primary color override for dark mode.
   */
  darkPrimaryColor?: string;

  /**
   * Optional icon URL representing the current agent or tool.
   */
  iconUrl?: string;

  /**
   * Optional display name representing the current agent or tool.
   */
  agentDisplayName?: string;
}

/**
 * Reasons for the change in CustomComponentAttribute
 */
export enum ChangeReason {
  /** Component update. */
  UPDATE_COMPONENT,

  /** Theme mode switching. */
  THEME_MODE_CHANGE,

    /** Breakpoint changed. */
  BREAKPOINT_CHANGE
}

/**
 * Attributes delivered to a custom component builder.
 */
export interface CustomComponentAttribute {
  /**
   * Component type name declared in the A2UI payload.
   */
  readonly type: string;

  /**
   * Optional component identifier inside the surface tree.
   */
  readonly id: string;

  /**
   * Surface identifier that owns the current component instance.
   */
  readonly surfaceId: string;

  /**
   * Raw custom property payload defined for the component.
   */
  readonly customProps?: Record<string, Object>;

  /**
   * protocolVersion values inherited from the current surface.
   */
  readonly protocolVersion: string;

  /**
   * catalogId.
   */
  readonly catalogId: string;

  /**
   * Theme values inherited from the current surface.
   */
  readonly componentTheme: ComponentTheme;

  /**
   * Resolver bound to the current custom component instance.
   */
  readonly resolver: DynamicValueResolver;

  /**
   * Reasons for the change in CustomComponentAttribute
   */
  readonly changeReason: ChangeReason;
}

/**
 * ArkUI builder type used to construct a custom component instance.
 */
export type ComponentBuilder = WrappedBuilder<[CustomComponentAttribute]>;

/**
 * Component registration metadata contributed by application code.
 */
export interface CatalogItem {
  /**
   * Stable component name referenced by incoming A2UI messages.
   */
  name: string;

  /**
   * Callback that returns the JSON schema for this component.
   */
  schemaProvider: SchemaProvider;

  /**
   * Optional ArkUI builder used to construct the custom component.
   */
  componentBuilder: ComponentBuilder;
}
