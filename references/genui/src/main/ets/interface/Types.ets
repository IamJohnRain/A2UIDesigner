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

/**
 * Literal value type supported by A2UI payloads and local functions.
 */
export type A2UIValueType =
  | string
  | number
  | boolean
  | null
  | undefined
  | A2UIValueType[]
  | Record<string, Object>;

/**
 * Surface lifecycle events emitted after A2UI messages are processed.
 */
export enum SurfaceEventType {
  /** Unknown or unsupported surface event. */
  UNKNOWN = -1,

  /** A new surface was created. */
  SURFACE_CREATED = 0,

  /** Surface components were added or updated. */
  SURFACE_COMPONENTS_UPDATED = 1,

  /** Surface data model was updated. */
  SURFACE_DATA_MODEL_UPDATED = 2,

  /** A surface was deleted. */
  SURFACE_DELETED = 3
}

/**
 * Detailed status codes returned by surface operations and error callbacks.
 */
export enum SurfaceErrorCode {
  /** Operation completed successfully. */
  NO_ERROR = 0,

  /** No surface matched the incoming request. */
  NO_SURFACE_MATCHED = 1001,

  /** Native message processing failed. */
  NATIVE_PROCESS_FAILED = 1002,

  /** The requested A2UI protocol version is not supported. */
  UNSUPPORTED_PROTOCOL_VERSION = 1003,

  /** A component was dropped because of invalid parameters. */
  COMPONENT_DROPPED_ON_INVALID_PARAMETER = 1004,

  /** A fallback path completed with warnings. */
  FALLBACK_WARNING = 1101,

  /** Schema validation produced warnings. */
  SCHEMA_WARNING = 2001,

  /** The input DSL is empty. */
  SCHEMA_DSL_EMPTY = 2002,

  /** The input DSL is not valid JSON. */
  SCHEMA_JSON_PARSE_FAILED = 2003,

  /** The JSON root node is not an object. */
  SCHEMA_ROOT_NOT_OBJECT = 2004,

  /** Message operation is missing or invalid. */
  SCHEMA_MESSAGE_OPERATION_INVALID = 2101,

  /** Multiple message bodies were provided in a single payload. */
  SCHEMA_MESSAGE_MULTIPLE_BODIES = 2102,

  /** Message body content is invalid. */
  SCHEMA_MESSAGE_BODY_INVALID = 2103,

  /** The payload is missing a required surface identifier. */
  SCHEMA_SURFACE_ID_MISSING = 2104,

  /** Component data is missing or invalid. */
  SCHEMA_COMPONENTS_INVALID = 2105,

  /** The payload is missing a catalog identifier. */
  SCHEMA_CATALOG_ID_MISSING = 2106,

  /** The payload declares an invalid schema version. */
  SCHEMA_VERSION_INVALID = 2107,

  /** Local action receiver has not been registered. */
  ACTION_NOT_REGISTER = 3001,

  /** Local function execution failed. */
  LOCAL_FUNCTION = 3101,

  /** Action payload parsing failed. */
  ACTION_PARSE_FAILED = 3201,

  /** Dynamic value resolution failed. */
  DYNAMIC_VALUE_RESOLVE_FAILED = 3202,

  /** Referenced global variable name is not supported. */
  GLOBAL_VARIABLE_NOT_FOUND = 3203,

  /** Expression syntax is illegal or uses an unsupported form. */
  ILLEGAL_EXPRESSION = 3204,

  /** Multi-surface mode is disabled for the current controller. */
  MULTI_SURFACE_DISABLED = 11001,

  /** Back navigation is blocked because only one surface remains. */
  MULTI_SURFACE_ONLY_ONE_SURFACE = 11002,

  /** No surface is available in the current stack. */
  MULTI_SURFACE_EMPTY_STACK = 11003,

  /** The configured surface count limit has been reached. */
  MULTI_SURFACE_MAX_SURFACE_LIMIT_REACHED = 11004,

  /** The requested surface already exists. */
  MULTI_SURFACE_ALREADY_EXISTS = 11005,
}

/**
 * Theme mode options supported by the renderer.
 */
export enum ThemeMode {
  /** Light theme mode. */
  LIGHT = 0,

  /** Dark theme mode. */
  DARK = 1
}

/**
 * Breakpoint buckets used for responsive layout decisions.
 */
export enum Breakpoint {
  /** Extra-small screen width. */
  XS = 0,

  /** Small screen width. */
  SM = 1,

  /** Medium screen width. */
  MD = 2,

  /** Large screen width. */
  LG = 3,

  /** Extra-large screen width. */
  XL = 4
}

/**
 * Callback that returns the JSON schema for a specific protocol version.
 */
export type SchemaProvider = (version: string) => string;
