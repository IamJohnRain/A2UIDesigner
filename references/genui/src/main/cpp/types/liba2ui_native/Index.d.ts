import { NodeContent } from '@kit.ArkUI';

export interface ProcessMessageOptions {
  supportsMultipleSurfaces: boolean;
  maxSurfaceCount: number;
  isExtend?: boolean;
}

export interface ProcessMessageResult {
  success: boolean;
  errorCode?: string;
  errorMessage?: string;
  surfaceId?: string;
  messageType?: number;
  surfaceResultCode?: number;
}

export interface SurfaceResult {
  success: boolean;
  code: number;
  message?: string;
}

export type ExpressionResultType = 'string' | 'number' | 'boolean' | 'object' | 'array' | 'null' | 'undefined';

export interface ExpressionEvaluateResult {
  success: boolean;
  type: ExpressionResultType;
  value?: string | number | boolean | null | Object | Object[];
}

export type NativeValue =
  | string
  | number
  | boolean
  | null
  | undefined
  | NativeValue[]
  | Record<string, Object>;

export interface NativeActionEventRequest {
  renderId: number;
  surfaceId: string;
  sourceComponentId: string;
  name: string;
  context?: NativeValue;
  timestamp?: string;
}

export interface NativeRuntimeErrorRequest {
  renderId: number;
  surfaceId: string;
  componentId: string;
  errorCode: number;
  errorMessage: string;
  source?: string;
}

export interface NativeSchemaWarningRequest {
  renderId: number;
  surfaceId: string;
  componentId: string;
  code: string;
  message: string;
  path?: string;
  itemType?: 'message' | 'component' | 'function' | 'schema';
  itemName?: string;
}

export interface CrossLanguageAttributeRequest {
  renderId: number;
  componentId: string;
  nodeUniqueId: number;
  componentType: string;
  attributeName: string;
  floatValue: number;
  stringValue: string;
  payloadJson: string;
  reset: boolean;
}

export interface LocalFunctionRequest {
  renderId: number;
  surfaceId: string;
  componentId: string;
  functionName: string;
  args?: NativeValue;
  returnType: string;
  normalizeOnly?: boolean;
}

export interface LocalFunctionResponse {
  success: boolean;
  returnType: string;
  value?: NativeValue;
  normalizedArgs?: NativeValue;
  normalizedReturnType?: string;
  errorCode?: number;
  errorMessage?: string;
}

export interface CommonStyleProps {
  size?: string;
  width?: number;
  height?: number;
  weight?: number;
  margin?: string;
  accessibilityLabel?: string;
  accessibilityDescription?: string;
}

export interface ComponentTheme {
  primaryColor?: string;
  primaryColorArgb?: number;
  brandColorArgb?: number;
  colorMode?: number;
  breakpoint?: number;
  darkPrimaryColor?: string;
  darkPrimaryColorArgb?: number;
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

export interface CustomComponentResult {
  content?: Object;
  childSlot?: NodeContent;
  childSlots?: Map<string, NodeContent>;
  childSlotsObject?: Record<string, NodeContent>;
}

export interface ValidateCustomComponentChecksResult {
  valid: boolean;
  message: string;
}

export interface SyncComponentBoundDataModelResult {
  success: boolean;
  errorCode?: string;
  errorMessage?: string;
}

export interface ResolveCustomComponentDynamicValueResult {
  success: boolean;
  errorCode?: string;
  errorMessage?: string;
}

export interface EvaluateDynamicValueResult {
  success: boolean;
  value?: NativeValue;
  errorCode?: string;
  errorMessage?: string;
}

export type InvokeLocalFunctionCallback = (request: LocalFunctionRequest) => LocalFunctionResponse;
export type DispatchActionCallback = (request: NativeActionEventRequest) => void;
export type DispatchRuntimeErrorCallback = (request: NativeRuntimeErrorRequest) => void;
export type DispatchSchemaWarningCallback = (request: NativeSchemaWarningRequest) => void;
export type CreateCustomComponentCallback = (value: CustomComponentDescriptor) => CustomComponentResult;
export type UpdateCustomComponentCallback = (
  customComponent: never,
  childSlot: never,
  childSlots: Map<string, NodeContent> | Record<string, NodeContent> | undefined,
  value: CustomComponentDescriptor
) => CustomComponentResult;
export type CrossLanguageAttributeCallback = (request: CrossLanguageAttributeRequest) => void;

export const initRenderSlot: (renderId: number) => void;

export const destroyRenderSlot: (renderId: number) => void;

export const registerLocale: (localeOrProvider: string | (() => string)) => void;

export const processMessage: (
  renderId: number,
  dsl: string,
  catalog: Object,
  options?: ProcessMessageOptions
) => ProcessMessageResult;

export const bindSurfaceToRender: (renderId: number, content: Object) => void;

export const unbindSurfaceFromRender: (renderId: number) => void;

export const popSurface: (renderId: number) => SurfaceResult;

export const getSurfaceIds: (renderId: number) => string[];

export const getLatestSurfaceId: (renderId: number) => string | undefined;

export const registerInvokeLocalFunction: (callback: InvokeLocalFunctionCallback) => void;

export const registerDispatchAction: (callback: DispatchActionCallback) => void;

export const registerDispatchRuntimeError: (callback: DispatchRuntimeErrorCallback) => void;

export const registerDispatchSchemaWarning: (callback: DispatchSchemaWarningCallback) => void;

export const registerCreateCustomComponent: (callback: CreateCustomComponentCallback) => void;

export const registerUpdateCustomComponent: (callback: UpdateCustomComponentCallback) => void;

export const validateCustomComponentChecks: (
  renderId: number,
  surfaceId: string,
  componentId: string,
  valueJson: string
) => ValidateCustomComponentChecksResult | undefined;

export const syncComponentBoundDataModel: (
  renderId: number,
  surfaceId: string,
  componentId: string,
  propertyName: string,
  valueJson: string
) => SyncComponentBoundDataModelResult;

export const resolveCustomComponentDynamicValue: (
  customComponentHandle: number,
  key: string,
  descriptorJson: string,
  callback: (value: NativeValue) => void
) => ResolveCustomComponentDynamicValueResult;

export const clearCustomComponentDynamicValue: (
  customComponentHandle: number,
  key: string
) => ResolveCustomComponentDynamicValueResult;

export const evaluateDynamicValue: (
  renderId: number,
  surfaceId: string,
  componentId: string,
  descriptorJson: string,
  allowExpression: boolean
) => EvaluateDynamicValueResult;

export const dispatchCustomComponentAction: (
  renderId: number,
  surfaceId: string,
  componentId: string,
  eventName: string,
  contextJson: string
) => void;

export const setFontSizeScale: (renderId: number, scale: number) => void;

export const setApiVersion: (renderId: number, apiVersion: number) => void;

export const registerCrossLanguageAttributeCallback: (callback: CrossLanguageAttributeCallback) => void;

export const setRootFillMode: (renderId: number, forceFill: boolean) => void;

export const updateThemeMode: (renderId: number, mode: number) => void;

export const updateBreakpoint: (renderId: number, breakpoint: number) => void;

export const setDisplayDensity: (renderId: number, densityPixels: number) => void;

export const evaluateExpression: (expression: string) => ExpressionEvaluateResult;

export interface NativeEngineModule {
  initRenderSlot: typeof initRenderSlot;
  destroyRenderSlot: typeof destroyRenderSlot;
  registerLocale: typeof registerLocale;
  processMessage: typeof processMessage;
  bindSurfaceToRender: typeof bindSurfaceToRender;
  unbindSurfaceFromRender: typeof unbindSurfaceFromRender;
  popSurface: typeof popSurface;
  getSurfaceIds: typeof getSurfaceIds;
  getLatestSurfaceId: typeof getLatestSurfaceId;
  registerInvokeLocalFunction: typeof registerInvokeLocalFunction;
  registerDispatchAction: typeof registerDispatchAction;
  registerDispatchRuntimeError: typeof registerDispatchRuntimeError;
  registerDispatchSchemaWarning: typeof registerDispatchSchemaWarning;
  registerCreateCustomComponent: typeof registerCreateCustomComponent;
  registerUpdateCustomComponent: typeof registerUpdateCustomComponent;
  validateCustomComponentChecks: typeof validateCustomComponentChecks;
  dispatchCustomComponentAction: typeof dispatchCustomComponentAction;
  resolveCustomComponentDynamicValue: typeof resolveCustomComponentDynamicValue;
  clearCustomComponentDynamicValue: typeof clearCustomComponentDynamicValue;
  evaluateDynamicValue: typeof evaluateDynamicValue;
  setFontSizeScale: typeof setFontSizeScale;
  setApiVersion: typeof setApiVersion;
  registerCrossLanguageAttributeCallback: typeof registerCrossLanguageAttributeCallback;
  syncComponentBoundDataModel: typeof syncComponentBoundDataModel;
  setRootFillMode: typeof setRootFillMode;
  updateThemeMode: typeof updateThemeMode;
  updateBreakpoint: typeof updateBreakpoint;
  setDisplayDensity: typeof setDisplayDensity;
  evaluateExpression?: typeof evaluateExpression;
}

declare const nativeEngine: NativeEngineModule;

export default nativeEngine;
