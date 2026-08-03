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

import { A2UIMessageType } from '../base/A2UIMessage';
import { CatalogImpl } from '../base/CatalogImpl';
import { CatalogItem } from '../base/CatalogItem';
import { CapabilitiesCore } from '../base/CapabilitiesCore';
import { A2UIValueType } from '../types/types';
import { InnerFunctionItem } from '../base/FunctionItem';

import { SchemaErrorCode, SchemaErrorInfoManager, SchemaItemType } from './SchemaErrorInfoManager';
import { SchemaResourceLoader } from './SchemaResourceLoader';

type SchemaValue = A2UIValueType | ESObject;
type SchemaObject = Record<string, SchemaValue>;

interface SchemaValidationContext {
  documentRoot: SchemaObject;
  itemType: SchemaItemType;
  itemName: string;
}

interface SchemaRefResolution {
  schema: SchemaObject;
  documentRoot: SchemaObject;
}

interface ObjectSchemaMetadata {
  propertySchemas: Map<string, SchemaObject>;
  requiredKeys: Set<string>;
  additionalPropertiesSchema?: SchemaObject;
  hasKnownProperties: boolean;
  allowLooseAdditionalProperties: boolean;
}

interface NormalizationResult {
  value: SchemaValue;
  matched: boolean;
  valid: boolean;
  issueCount: number;
  hasTypeMismatchIssue: boolean;
}

interface MessageDescriptor {
  key: string;
  type: A2UIMessageType;
  body: SchemaObject;
}

interface MessageDescriptorResolutionResult {
  success: boolean;
  descriptor?: MessageDescriptor;
  errorMessage?: string;
}

interface PreparedMessageEnvelopeResult {
  success: boolean;
  root?: SchemaObject;
  descriptor?: MessageDescriptor;
  normalizedVersion?: string;
  errorMessage?: string;
}

interface MessageEnvelopeBodyValidationResult {
  success: boolean;
  body?: SchemaObject;
  errorMessage?: string;
}

export interface SchemaDslValidationResult {
  success: boolean;
  normalizedDsl: string;
  errorMessage?: string;
}

export interface SchemaFunctionValidationResult {
  success: boolean;
  normalizedArgs?: A2UIValueType;
  normalizedReturnType: string;
  errorMessage?: string;
}

export class SchemaValidator {
  private static readonly A2UI_VERSION: string = 'v0.9';
  private static readonly SUPPORTED_MESSAGE_KEYS_TEXT: string =
    'createSurface, updateComponents, updateDataModel, deleteSurface';
  private static readonly MESSAGE_KEYS: Map<string, A2UIMessageType> = new Map([
    ['createSurface', A2UIMessageType.CREATE_SURFACE],
    ['updateComponents', A2UIMessageType.UPDATE_COMPONENTS],
    ['updateDataModel', A2UIMessageType.UPDATE_DATA_MODEL],
    ['deleteSurface', A2UIMessageType.DELETE_SURFACE]
  ]);
  private static readonly schemaCache: Map<string, SchemaObject> = new Map();
  private static commonSchemaCache?: SchemaObject;

  private constructor() {
  }

  public static validateMessageEnvelope(dsl: string): SchemaDslValidationResult {
    SchemaErrorInfoManager.getInstance().clear();

    const prepared: PreparedMessageEnvelopeResult = SchemaValidator.prepareMessageEnvelope(dsl);
    if (!prepared.success || prepared.root === undefined || prepared.descriptor === undefined ||
      prepared.normalizedVersion === undefined) {
      return {
        success: false,
        normalizedDsl: dsl,
        errorMessage: prepared.errorMessage
      };
    }

    const envelopeBodyResult: MessageEnvelopeBodyValidationResult =
      SchemaValidator.validateMessageEnvelopeBody(prepared.descriptor);
    if (!envelopeBodyResult.success || envelopeBodyResult.body === undefined) {
      return {
        success: false,
        normalizedDsl: dsl,
        errorMessage: envelopeBodyResult.errorMessage ?? prepared.errorMessage
      };
    }

    const normalizedRoot: SchemaObject = SchemaValidator.createSchemaObject();
    normalizedRoot['version'] = prepared.normalizedVersion;
    normalizedRoot[prepared.descriptor.key] = envelopeBodyResult.body;
    SchemaValidator.dropUnknownRootFields(prepared.root, prepared.descriptor.key);

    return {
      success: true,
      normalizedDsl: JSON.stringify(normalizedRoot)
    };
  }

  public static validateAndNormalizeDslStrict(dsl: string, _catalog: CatalogImpl): SchemaDslValidationResult {
    return SchemaValidator.validateMessageEnvelope(dsl);
  }

  // Deprecated compatibility entry. The strict/full ArkTS pre-validation pass
  // has been removed, so this now delegates to the default message envelope path.
  public static validateAndNormalizeDsl(dsl: string, _catalog: CatalogImpl): SchemaDslValidationResult {
    return SchemaValidator.validateMessageEnvelope(dsl);
  }

  public static validateAndNormalizeFunctionCall(
    functionName: string,
    args: A2UIValueType,
    returnType: string,
    catalogItem?: InnerFunctionItem
  ): SchemaFunctionValidationResult {
    if (functionName.trim().length === 0) {
      SchemaValidator.recordWarning(
        SchemaErrorCode.REQUIRED_MISS,
        'Property call is required',
        'function.call',
        'function',
        'unknown'
      );
      return {
        success: false,
        normalizedReturnType: returnType,
        errorMessage: 'Function call is required'
      };
    }

    const item: InnerFunctionItem | undefined = catalogItem;
    if (item === undefined) {
      return {
        success: true,
        normalizedArgs: args,
        normalizedReturnType: returnType
      };
    }
    if (item.schemaProvider === undefined) {
      return {
        success: true,
        normalizedArgs: args,
        normalizedReturnType: returnType
      };
    }

    const schemaText: string =
      item.schemaProvider(CapabilitiesCore.getDefaultA2UIProtocolVersion()).trim();
    if (schemaText.length === 0) {
      return {
        success: true,
        normalizedArgs: args,
        normalizedReturnType: returnType
      };
    }

    const schema: SchemaObject | undefined = SchemaValidator.parseSchema(schemaText);
    if (schema === undefined) {
      return {
        success: true,
        normalizedArgs: args,
        normalizedReturnType: returnType
      };
    }

    const requestValue: SchemaObject = SchemaValidator.createSchemaObject();
    requestValue['call'] = functionName;
    requestValue['args'] = args;
    requestValue['returnType'] = returnType;
    const result: NormalizationResult = SchemaValidator.normalizeValue(
      requestValue,
      schema,
      {
        documentRoot: schema,
        itemType: 'function',
        itemName: functionName
      },
      `function.${functionName}`
    );
    if (!SchemaValidator.isRecord(result.value)) {
      return {
        success: false,
        normalizedReturnType: returnType,
        errorMessage: `Function schema normalize failed: ${functionName}`
      };
    }

    const normalizedRequest: SchemaObject = result.value as SchemaObject;
    let normalizedReturnType: string = returnType;
    if (typeof normalizedRequest['returnType'] === 'string') {
      normalizedReturnType = normalizedRequest['returnType'] as string;
    }

    return {
      success: true,
      normalizedArgs: normalizedRequest['args'] as A2UIValueType,
      normalizedReturnType: normalizedReturnType
    };
  }

  private static prepareMessageEnvelope(dsl: string): PreparedMessageEnvelopeResult {
    if (dsl.trim().length === 0) {
      return {
        success: false,
        errorMessage: 'dsl is empty'
      };
    }

    let rootValue: Object;
    try {
      rootValue = JSON.parse(dsl) as Object;
    } catch (_error) {
      SchemaValidator.recordWarning(
        SchemaErrorCode.SCHEMA_PARSE_FAILED,
        'DSL JSON parse failed',
        'root',
        'message',
        'dsl'
      );
      return {
        success: false,
        errorMessage: 'dsl json parse failed'
      };
    }

    if (!SchemaValidator.isRecord(rootValue)) {
      SchemaValidator.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        'Root JSON must be an object',
        'root',
        'message',
        'dsl'
      );
      return {
        success: false,
        errorMessage: 'root json must be an object'
      };
    }

    const root: SchemaObject = rootValue as SchemaObject;
    const descriptorResult: MessageDescriptorResolutionResult = SchemaValidator.resolveMessageDescriptor(root);
    if (!descriptorResult.success || descriptorResult.descriptor === undefined) {
      return {
        success: false,
        errorMessage: descriptorResult.errorMessage ?? 'message descriptor resolve failed'
      };
    }

    return {
      success: true,
      root: root,
      descriptor: descriptorResult.descriptor,
      normalizedVersion: SchemaValidator.normalizeVersion(root['version'])
    };
  }

  private static validateMessageEnvelopeBody(
    descriptor: MessageDescriptor
  ): MessageEnvelopeBodyValidationResult {
    const body: SchemaObject = SchemaValidator.cloneValue(descriptor.body);
    if (!SchemaValidator.ensureMessageSurfaceId(body, descriptor.key)) {
      return {
        success: false,
        errorMessage: `${descriptor.key}.surfaceId is invalid`
      };
    }

    if (descriptor.type === A2UIMessageType.CREATE_SURFACE &&
      !SchemaValidator.ensureCreateSurfaceCatalogId(body, descriptor.key)) {
      return {
        success: false,
        errorMessage: `${descriptor.key}.catalogId is invalid`
      };
    }

    if (descriptor.type === A2UIMessageType.UPDATE_COMPONENTS) {
      const componentsValue: SchemaValue = body['components'];
      if (!Array.isArray(componentsValue)) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          'Message components must be an array',
          `${descriptor.key}.components`,
          'message',
          descriptor.key
        );
        return {
          success: false,
          errorMessage: `${descriptor.key}.components must be an array`
        };
      }
    }

    if (descriptor.type === A2UIMessageType.UPDATE_DATA_MODEL &&
      body['path'] === undefined && body['value'] === undefined) {
      SchemaValidator.recordWarning(
        SchemaErrorCode.REQUIRED_MISS,
        'Message updateDataModel requires path or value, fallback path to "/"',
        `${descriptor.key}.path`,
        'message',
        descriptor.key
      );
      body['path'] = '/';
    }

    return {
      success: true,
      body: body
    };
  }

  private static resolveMessageDescriptor(root: SchemaObject): MessageDescriptorResolutionResult {
    const matchedKeys: string[] = [];

    SchemaValidator.MESSAGE_KEYS.forEach((_messageType: A2UIMessageType, key: string) => {
      if (root[key] !== undefined) {
        matchedKeys.push(key);
      }
    });

    if (matchedKeys.length === 0) {
      const unknownRootKeys: string[] = SchemaValidator.collectUnknownRootKeys(root);
      const errorMessage: string = unknownRootKeys.length > 0
        ? SchemaValidator.buildUnknownMessageOperationError(unknownRootKeys)
        : `Message body is missing, expected exactly one of ${SchemaValidator.getSupportedMessageKeysText()}`;
      SchemaValidator.recordWarning(
        SchemaErrorCode.INVALID_VALUE,
        errorMessage,
        'root',
        'message',
        'dsl'
      );
      return {
        success: false,
        errorMessage: errorMessage
      };
    }

    if (matchedKeys.length > 1) {
      const errorMessage: string = `Message contains multiple bodies, only one is allowed: ${matchedKeys.join(', ')}`;
      SchemaValidator.recordWarning(
        SchemaErrorCode.INVALID_VALUE,
        errorMessage,
        'root',
        'message',
        'dsl'
      );
      return {
        success: false,
        errorMessage: errorMessage
      };
    }

    const resolvedKey: string = matchedKeys[0];
    const resolvedType: A2UIMessageType | undefined = SchemaValidator.MESSAGE_KEYS.get(resolvedKey);
    if (resolvedType === undefined) {
      const errorMessage: string = `Message operation is not supported: ${resolvedKey}`;
      SchemaValidator.recordWarning(
        SchemaErrorCode.INVALID_VALUE,
        errorMessage,
        'root',
        'message',
        'dsl'
      );
      return {
        success: false,
        errorMessage: errorMessage
      };
    }

    const bodyValue: SchemaValue = root[resolvedKey];
    if (!SchemaValidator.isRecord(bodyValue)) {
      const errorMessage: string = `Message ${resolvedKey} body must be an object`;
      SchemaValidator.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        errorMessage,
        resolvedKey,
        'message',
        resolvedKey
      );
      return {
        success: false,
        errorMessage: errorMessage
      };
    }

    return {
      success: true,
      descriptor: {
        key: resolvedKey,
        type: resolvedType,
        body: SchemaValidator.cloneValue(bodyValue as SchemaObject)
      }
    };
  }

  private static normalizeVersion(versionValue: SchemaValue): string {
    if (typeof versionValue === 'string' && versionValue === SchemaValidator.A2UI_VERSION) {
      return versionValue;
    }

    SchemaValidator.recordWarning(
      SchemaErrorCode.INVALID_VALUE,
      `Message version is invalid, fallback to ${SchemaValidator.A2UI_VERSION}`,
      'version',
      'message',
      'dsl'
    );
    return SchemaValidator.A2UI_VERSION;
  }

  private static ensureMessageSurfaceId(body: SchemaObject, messageKey: string): boolean {
    const surfaceIdValue: SchemaValue = body['surfaceId'];
    if (typeof surfaceIdValue === 'string' && surfaceIdValue.trim().length > 0) {
      return true;
    }

    SchemaValidator.recordWarning(
      SchemaErrorCode.REQUIRED_MISS,
      'Message surfaceId is required',
      `${messageKey}.surfaceId`,
      'message',
      messageKey
    );
    return false;
  }

  private static ensureCreateSurfaceCatalogId(body: SchemaObject, messageKey: string): boolean {
    const catalogIdValue: SchemaValue = body['catalogId'];
    if (typeof catalogIdValue === 'string' && catalogIdValue.trim().length > 0) {
      return true;
    }

    SchemaValidator.recordWarning(
      SchemaErrorCode.REQUIRED_MISS,
      'Message catalogId is required',
      `${messageKey}.catalogId`,
      'message',
      messageKey
    );
    return false;
  }

  private static normalizeValue(
    value: SchemaValue,
    schema: SchemaObject,
    context: SchemaValidationContext,
    path: string,
    recordWarnings: boolean = true
  ): NormalizationResult {
    if (typeof schema['$ref'] === 'string') {
      const resolvedRef: SchemaRefResolution | undefined =
        SchemaValidator.resolveSchemaRefWithRoot(schema['$ref'] as string, context.documentRoot);
      if (resolvedRef !== undefined) {
        return SchemaValidator.normalizeValue(
          value,
          resolvedRef.schema,
          {
            documentRoot: resolvedRef.documentRoot,
            itemType: context.itemType,
            itemName: context.itemName
          },
          path,
          recordWarnings
        );
      }
    }

    let normalizedValue: SchemaValue = SchemaValidator.cloneValue(value);
    let matched: boolean = true;
    let valid: boolean = true;
    let issueCount: number = 0;
    let hasTypeMismatchIssue: boolean = false;

    const allOfSchemas: SchemaValue = schema['allOf'];
    if (Array.isArray(allOfSchemas)) {
      const allOfSchemaList: SchemaValue[] = allOfSchemas as SchemaValue[];
      for (let index = 0; index < allOfSchemaList.length; index++) {
        const subSchemaValue: SchemaValue = allOfSchemaList[index];
        if (!SchemaValidator.isRecord(subSchemaValue)) {
          continue;
        }
        const subResult: NormalizationResult =
          SchemaValidator.normalizeValue(
            normalizedValue,
            subSchemaValue as SchemaObject,
            context,
            path,
            recordWarnings
          );
        normalizedValue = subResult.value;
        matched = matched && subResult.matched;
        valid = valid && subResult.valid;
        issueCount += subResult.issueCount;
        hasTypeMismatchIssue = hasTypeMismatchIssue || subResult.hasTypeMismatchIssue;
      }
    }

    const unionResult: NormalizationResult | undefined =
      SchemaValidator.normalizeUnion(normalizedValue, schema, context, path, recordWarnings);
    if (unionResult !== undefined) {
      normalizedValue = unionResult.value;
      matched = matched && unionResult.matched;
      valid = valid && unionResult.valid;
      issueCount += unionResult.issueCount;
      hasTypeMismatchIssue = hasTypeMismatchIssue || unionResult.hasTypeMismatchIssue;
    }

    const conditionalResult: NormalizationResult | undefined =
      SchemaValidator.normalizeConditional(normalizedValue, schema, context, path, recordWarnings);
    if (conditionalResult !== undefined) {
      normalizedValue = conditionalResult.value;
      matched = matched && conditionalResult.matched;
      valid = valid && conditionalResult.valid;
      issueCount += conditionalResult.issueCount;
      hasTypeMismatchIssue = hasTypeMismatchIssue || conditionalResult.hasTypeMismatchIssue;
    }

    const schemaType: string | undefined = typeof schema['type'] === 'string' ? schema['type'] as string : undefined;
    if (schemaType === 'object') {
      const objectResult: NormalizationResult =
        SchemaValidator.normalizeObjectValue(normalizedValue, schema, context, path, recordWarnings);
      return {
        value: objectResult.value,
        matched: matched && objectResult.matched,
        valid: valid && objectResult.valid,
        issueCount: issueCount + objectResult.issueCount,
        hasTypeMismatchIssue: hasTypeMismatchIssue || objectResult.hasTypeMismatchIssue
      };
    }

    if (schemaType === 'array') {
      const arrayResult: NormalizationResult =
        SchemaValidator.normalizeArrayValue(normalizedValue, schema, context, path, recordWarnings);
      return {
        value: arrayResult.value,
        matched: matched && arrayResult.matched,
        valid: valid && arrayResult.valid,
        issueCount: issueCount + arrayResult.issueCount,
        hasTypeMismatchIssue: hasTypeMismatchIssue || arrayResult.hasTypeMismatchIssue
      };
    }

    const scalarResult: NormalizationResult =
      SchemaValidator.normalizeScalarValue(normalizedValue, schema, context, path, recordWarnings);
    return {
      value: scalarResult.value,
      matched: matched && scalarResult.matched,
      valid: valid && scalarResult.valid,
      issueCount: issueCount + scalarResult.issueCount,
      hasTypeMismatchIssue: hasTypeMismatchIssue || scalarResult.hasTypeMismatchIssue
    };
  }

  private static normalizeUnion(
    value: SchemaValue,
    schema: SchemaObject,
    context: SchemaValidationContext,
    path: string,
    recordWarnings: boolean
  ): NormalizationResult | undefined {
    const unionValue: SchemaValue = schema['oneOf'] !== undefined ? schema['oneOf'] : schema['anyOf'];
    if (!Array.isArray(unionValue)) {
      return undefined;
    }

    const branchSchemas: SchemaObject[] = [];
    const branchResults: NormalizationResult[] = [];
    const branchValueList: SchemaValue[] = unionValue as SchemaValue[];
    for (let index = 0; index < branchValueList.length; index++) {
      const branchValue: SchemaValue = branchValueList[index];
      if (!SchemaValidator.isRecord(branchValue)) {
        continue;
      }
      const effectiveBranchSchema: SchemaObject =
        SchemaValidator.createEffectiveUnionBranchSchema(branchValue as SchemaObject, schema, context);
      branchSchemas.push(effectiveBranchSchema);
      branchResults.push(
        SchemaValidator.normalizeValue(value, effectiveBranchSchema, context, path, false)
      );
    }

    if (branchResults.length === 0) {
      return undefined;
    }

    const matchedResults: NormalizationResult[] = [];
    for (let index = 0; index < branchResults.length; index++) {
      const branchResult: NormalizationResult = branchResults[index];
      if (branchResult.matched) {
        matchedResults.push(branchResult);
      }
    }
    const candidateResults: NormalizationResult[] = matchedResults.length > 0 ? matchedResults : branchResults;
    let selectedIndex: number = branchResults.indexOf(candidateResults[0]);
    for (let i = 0; i < branchResults.length; i++) {
      if (candidateResults.indexOf(branchResults[i]) < 0) {
        continue;
      }
      if (branchResults[i].issueCount < branchResults[selectedIndex].issueCount) {
        selectedIndex = i;
      }
    }
    const selectedResult: NormalizationResult =
      SchemaValidator.normalizeValue(value, branchSchemas[selectedIndex], context, path, recordWarnings);

    if (matchedResults.length === 0) {
      const hasSpecificBranchTypeMismatchIssue: boolean = selectedResult.hasTypeMismatchIssue;
      if (recordWarnings && !hasSpecificBranchTypeMismatchIssue &&
        !SchemaValidator.shouldSuppressUnionFallbackWarning(schema, context)) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.INVALID_VALUE,
          'Value does not match any allowed schema branch, fallback to closest branch',
          path,
          context.itemType,
          context.itemName
        );
      }
      return {
        value: selectedResult.value,
        matched: false,
        valid: selectedResult.valid,
        issueCount: selectedResult.issueCount + 1,
        hasTypeMismatchIssue: selectedResult.hasTypeMismatchIssue
      };
    }

    return selectedResult;
  }

  private static createEffectiveUnionBranchSchema(
    branchSchema: SchemaObject,
    parentSchema: SchemaObject,
    context: SchemaValidationContext
  ): SchemaObject {
    const parentType: string | undefined =
      typeof parentSchema['type'] === 'string' ? parentSchema['type'] as string : undefined;
    if (context.itemType !== 'function' || parentType !== 'object' || typeof branchSchema['type'] === 'string' ||
      !SchemaValidator.isObjectConstraintUnionBranch(branchSchema)) {
      return branchSchema;
    }

    const clonedSchema: SchemaValue = SchemaValidator.cloneValue(branchSchema);
    if (!SchemaValidator.isRecord(clonedSchema)) {
      return branchSchema;
    }

    const effectiveSchema: SchemaObject = clonedSchema as SchemaObject;
    effectiveSchema['type'] = 'object';
    SchemaValidator.mergeObjectConstraintBranchWithParent(effectiveSchema, parentSchema);
    return effectiveSchema;
  }

  private static isObjectConstraintUnionBranch(schema: SchemaObject): boolean {
    return Array.isArray(schema['required']) ||
      SchemaValidator.isRecord(schema['properties']) ||
      SchemaValidator.isRecord(schema['additionalProperties']) ||
      schema['additionalProperties'] === true ||
      schema['unevaluatedProperties'] !== undefined;
  }

  private static mergeObjectConstraintBranchWithParent(
    branchSchema: SchemaObject,
    parentSchema: SchemaObject
  ): void {
    if (SchemaValidator.isRecord(parentSchema['properties'])) {
      const branchProperties: SchemaObject = SchemaValidator.isRecord(branchSchema['properties'])
        ? branchSchema['properties'] as SchemaObject
        : SchemaValidator.createSchemaObject();
      const parentProperties: SchemaObject = parentSchema['properties'] as SchemaObject;
      const mergedProperties: SchemaObject = SchemaValidator.cloneValue(parentProperties) as SchemaObject;
      Object.keys(branchProperties).forEach((key: string) => {
        mergedProperties[key] = branchProperties[key];
      });
      branchSchema['properties'] = mergedProperties;
    }

    if (branchSchema['additionalProperties'] === undefined && parentSchema['additionalProperties'] !== undefined) {
      branchSchema['additionalProperties'] = SchemaValidator.cloneValue(
        parentSchema['additionalProperties'] as A2UIValueType
      );
    }

    if (branchSchema['unevaluatedProperties'] === undefined && parentSchema['unevaluatedProperties'] !== undefined) {
      branchSchema['unevaluatedProperties'] = SchemaValidator.cloneValue(
        parentSchema['unevaluatedProperties'] as A2UIValueType
      );
    }

    const mergedRequiredKeys: string[] = [];
    const appendRequiredKeys = (requiredValue: SchemaValue): void => {
      if (!Array.isArray(requiredValue)) {
        return;
      }
      const requiredKeyList: SchemaValue[] = requiredValue as SchemaValue[];
      for (let index = 0; index < requiredKeyList.length; index++) {
        const requiredKeyValue: SchemaValue = requiredKeyList[index];
        if (typeof requiredKeyValue !== 'string' || mergedRequiredKeys.indexOf(requiredKeyValue) >= 0) {
          continue;
        }
        mergedRequiredKeys.push(requiredKeyValue);
      }
    };

    appendRequiredKeys(parentSchema['required']);
    appendRequiredKeys(branchSchema['required']);
    if (mergedRequiredKeys.length > 0) {
      branchSchema['required'] = mergedRequiredKeys;
    }
  }

  private static shouldSuppressUnionFallbackWarning(
    schema: SchemaObject,
    context: SchemaValidationContext
  ): boolean {
    if (context.itemType === 'function') {
      return false;
    }

    const unionValue: SchemaValue = schema['oneOf'] !== undefined ? schema['oneOf'] : schema['anyOf'];
    if (!Array.isArray(unionValue)) {
      return false;
    }

    let hasLiteralBranch: boolean = false;
    let hasDynamicDescriptorBranch: boolean = false;
    for (let index = 0; index < unionValue.length; index++) {
      const branchValue: SchemaValue = unionValue[index];
      if (!SchemaValidator.isRecord(branchValue)) {
        continue;
      }
      const branchSchema: SchemaObject = branchValue as SchemaObject;
      hasLiteralBranch = hasLiteralBranch || SchemaValidator.isLiteralUnionBranch(branchSchema);
      hasDynamicDescriptorBranch =
        hasDynamicDescriptorBranch || SchemaValidator.isDynamicDescriptorUnionBranch(branchSchema);
    }

    return hasLiteralBranch && hasDynamicDescriptorBranch;
  }

  private static isLiteralUnionBranch(schema: SchemaObject): boolean {
    return typeof schema['type'] === 'string';
  }

  private static isDynamicDescriptorUnionBranch(schema: SchemaObject): boolean {
    const ref: string | undefined = typeof schema['$ref'] === 'string' ? schema['$ref'] as string : undefined;
    if (ref === '#/$defs/DataBinding' || ref === '#/$defs/FunctionCall') {
      return true;
    }

    if (!Array.isArray(schema['allOf'])) {
      return false;
    }

    const allOfSchemaList: SchemaValue[] = schema['allOf'] as SchemaValue[];
    for (let index = 0; index < allOfSchemaList.length; index++) {
      const subSchemaValue: SchemaValue = allOfSchemaList[index];
      if (!SchemaValidator.isRecord(subSchemaValue)) {
        continue;
      }
      if (SchemaValidator.isDynamicDescriptorUnionBranch(subSchemaValue as SchemaObject)) {
        return true;
      }
    }
    return false;
  }

  private static normalizeConditional(
    value: SchemaValue,
    schema: SchemaObject,
    context: SchemaValidationContext,
    path: string,
    recordWarnings: boolean
  ): NormalizationResult | undefined {
    if (!SchemaValidator.isRecord(schema['if']) || !SchemaValidator.isRecord(schema['then'])) {
      return undefined;
    }

    const ifResult: NormalizationResult =
      SchemaValidator.normalizeValue(value, schema['if'] as SchemaObject, context, path, false);
    if (!ifResult.matched) {
      return undefined;
    }
    return SchemaValidator.normalizeValue(value, schema['then'] as SchemaObject, context, path, recordWarnings);
  }

  private static normalizeObjectValue(
    value: SchemaValue,
    schema: SchemaObject,
    context: SchemaValidationContext,
    path: string,
    recordWarnings: boolean
  ): NormalizationResult {
    let workingValue: SchemaObject;
    let matched: boolean = true;
    let valid: boolean = true;
    let issueCount: number = 0;
    let hasTypeMismatchIssue: boolean = false;

    if (!SchemaValidator.isRecord(value)) {
      workingValue = SchemaValidator.createFallbackObject(schema, context.documentRoot);
      matched = false;
      valid = false;
      issueCount++;
      hasTypeMismatchIssue = true;
      if (recordWarnings) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          'Object field type mismatch, fallback to default object value',
          path,
          context.itemType,
          context.itemName
        );
      }
    } else {
      workingValue = SchemaValidator.cloneValue(value as SchemaObject);
    }

    const metadata: ObjectSchemaMetadata =
      SchemaValidator.collectObjectMetadata(schema, context.documentRoot, new Set<string>());

    metadata.requiredKeys.forEach((requiredKey: string) => {
      if (workingValue[requiredKey] !== undefined) {
        return;
      }

      const propertySchema: SchemaObject | undefined = metadata.propertySchemas.get(requiredKey);
      matched = false;
      valid = false;
      if (propertySchema === undefined) {
        issueCount++;
        if (recordWarnings) {
          SchemaValidator.recordWarning(
            SchemaErrorCode.REQUIRED_MISS,
            `Property ${requiredKey} is required`,
            SchemaValidator.joinPath(path, requiredKey),
            context.itemType,
            context.itemName
          );
        }
        return;
      }

      workingValue[requiredKey] = SchemaValidator.buildFallbackValue(propertySchema, context.documentRoot);
      issueCount++;
      if (recordWarnings) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${requiredKey} is required, fallback to default value`,
          SchemaValidator.joinPath(path, requiredKey),
          context.itemType,
          context.itemName
        );
      }
    });

    metadata.propertySchemas.forEach((propertySchema: SchemaObject, key: string) => {
      if (workingValue[key] === undefined) {
        return;
      }
      const propertyResult: NormalizationResult = SchemaValidator.normalizeValue(
        workingValue[key],
        propertySchema,
        context,
        SchemaValidator.joinPath(path, key),
        recordWarnings
      );
      workingValue[key] = propertyResult.value;
      matched = matched && propertyResult.matched;
      valid = valid && propertyResult.valid;
      issueCount += propertyResult.issueCount;
      hasTypeMismatchIssue = hasTypeMismatchIssue || propertyResult.hasTypeMismatchIssue;
    });

    Object.keys(workingValue).forEach((key: string) => {
      if (metadata.propertySchemas.has(key)) {
        return;
      }
      if (metadata.additionalPropertiesSchema !== undefined) {
        const extraResult: NormalizationResult = SchemaValidator.normalizeValue(
          workingValue[key],
          metadata.additionalPropertiesSchema,
          context,
          SchemaValidator.joinPath(path, key),
          recordWarnings
        );
        workingValue[key] = extraResult.value;
        matched = matched && extraResult.matched;
        valid = valid && extraResult.valid;
        issueCount += extraResult.issueCount;
        hasTypeMismatchIssue = hasTypeMismatchIssue || extraResult.hasTypeMismatchIssue;
        return;
      }
      if (metadata.allowLooseAdditionalProperties) {
        return;
      }
      if (!metadata.hasKnownProperties) {
        return;
      }

      workingValue[key] = undefined;
      issueCount++;
      if (recordWarnings) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.UNDEFINED_FIELD,
          `Property ${key} is undefined in schema and has been removed`,
          SchemaValidator.joinPath(path, key),
          context.itemType,
          context.itemName
        );
      }
    });

    return {
      value: workingValue,
      matched: matched,
      valid: valid,
      issueCount: issueCount,
      hasTypeMismatchIssue: hasTypeMismatchIssue
    };
  }

  private static normalizeArrayValue(
    value: SchemaValue,
    schema: SchemaObject,
    context: SchemaValidationContext,
    path: string,
    recordWarnings: boolean
  ): NormalizationResult {
    let workingValue: A2UIValueType[] = [];
    let matched: boolean = true;
    let valid: boolean = true;
    let issueCount: number = 0;
    let hasTypeMismatchIssue: boolean = false;

    if (!Array.isArray(value)) {
      workingValue = SchemaValidator.buildFallbackValue(schema) as A2UIValueType[];
      if (!Array.isArray(workingValue)) {
        workingValue = [];
      }
      matched = false;
      valid = false;
      issueCount++;
      hasTypeMismatchIssue = true;
      if (recordWarnings) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          'Array field type mismatch, fallback to default array value',
          path,
          context.itemType,
          context.itemName
        );
      }
    } else {
      workingValue = SchemaValidator.cloneValue(value as A2UIValueType[]);
    }

    const itemSchema: SchemaObject | undefined = SchemaValidator.isRecord(schema['items'])
      ? schema['items'] as SchemaObject
      : undefined;
    if (itemSchema !== undefined) {
      for (let i = 0; i < workingValue.length; i++) {
        const itemResult: NormalizationResult = SchemaValidator.normalizeValue(
          workingValue[i],
          itemSchema,
          context,
          `${path}[${i}]`,
          recordWarnings
        );
        workingValue[i] = itemResult.value as A2UIValueType;
        matched = matched && itemResult.matched;
        valid = valid && itemResult.valid;
        issueCount += itemResult.issueCount;
        hasTypeMismatchIssue = hasTypeMismatchIssue || itemResult.hasTypeMismatchIssue;
      }
    }

    const minItems: number = typeof schema['minItems'] === 'number' ? schema['minItems'] as number : 0;
    let appendedItemCount: number = 0;
    while (itemSchema !== undefined && workingValue.length < minItems) {
      matched = false;
      valid = false;
      workingValue.push(SchemaValidator.buildFallbackValue(itemSchema));
      issueCount++;
      appendedItemCount++;
    }

    if (recordWarnings && appendedItemCount > 0) {
      SchemaValidator.recordWarning(
        SchemaErrorCode.REQUIRED_MISS,
        'Array size is smaller than schema minimum, append default item',
        path,
        context.itemType,
        context.itemName
      );
    }

    return {
      value: workingValue,
      matched: matched,
      valid: valid,
      issueCount: issueCount,
      hasTypeMismatchIssue: hasTypeMismatchIssue
    };
  }

  private static normalizeScalarValue(
    value: SchemaValue,
    schema: SchemaObject,
    context: SchemaValidationContext,
    path: string,
    recordWarnings: boolean
  ): NormalizationResult {
    let workingValue: SchemaValue = SchemaValidator.cloneValue(value);
    let matched: boolean = true;
    let valid: boolean = true;
    let issueCount: number = 0;
    let hasTypeMismatchIssue: boolean = false;
    const schemaType: string | undefined = typeof schema['type'] === 'string' ? schema['type'] as string : undefined;

    if (schemaType !== undefined && !SchemaValidator.matchesType(workingValue, schemaType)) {
      workingValue = SchemaValidator.buildFallbackValue(schema);
      matched = false;
      valid = false;
      issueCount++;
      hasTypeMismatchIssue = true;
      if (recordWarnings) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          `Field type mismatch, fallback to schema default for type ${schemaType}`,
          path,
          context.itemType,
          context.itemName
        );
      }
    }

    if (schema['const'] !== undefined && !SchemaValidator.isDeepEqual(workingValue, schema['const'])) {
      workingValue = SchemaValidator.cloneValue(schema['const'] as A2UIValueType);
      if (value !== undefined) {
        matched = false;
        valid = false;
        issueCount++;
        if (recordWarnings) {
          SchemaValidator.recordWarning(
            SchemaErrorCode.INVALID_VALUE,
            'Field does not match const constraint, fallback to const value',
            path,
            context.itemType,
            context.itemName
          );
        }
      }
    }

    if (Array.isArray(schema['enum']) && !SchemaValidator.enumContains(schema['enum'] as A2UIValueType[], workingValue)) {
      workingValue = SchemaValidator.buildFallbackValue(schema);
      matched = false;
      valid = false;
      issueCount++;
      if (recordWarnings) {
        SchemaValidator.recordWarning(
          SchemaErrorCode.INVALID_VALUE,
          'Field value is not in enum list, fallback to default enum value',
          path,
          context.itemType,
          context.itemName
        );
      }
    }

    const pattern: string | undefined = typeof schema['pattern'] === 'string' ? schema['pattern'] as string : undefined;
    if (pattern !== undefined && typeof workingValue === 'string') {
      try {
        const regex: RegExp = new RegExp(pattern);
        if (!regex.test(workingValue)) {
          workingValue = SchemaValidator.buildFallbackValue(schema);
          matched = false;
          valid = false;
          issueCount++;
          if (recordWarnings) {
            SchemaValidator.recordWarning(
              SchemaErrorCode.INVALID_VALUE,
              'Field value does not match schema pattern, fallback to default value',
              path,
              context.itemType,
              context.itemName
            );
          }
        }
      } catch (_error) {
        issueCount++;
      }
    }

    return {
      value: workingValue,
      matched: matched,
      valid: valid,
      issueCount: issueCount,
      hasTypeMismatchIssue: hasTypeMismatchIssue
    };
  }

  private static collectObjectMetadata(
    schema: SchemaObject,
    documentRoot: SchemaObject,
    visitedRefs: Set<string>
  ): ObjectSchemaMetadata {
    const metadata: ObjectSchemaMetadata = {
      propertySchemas: new Map<string, SchemaObject>(),
      requiredKeys: new Set<string>(),
      hasKnownProperties: false,
      allowLooseAdditionalProperties: false
    };

    if (typeof schema['$ref'] === 'string') {
      const ref: string = schema['$ref'] as string;
      if (visitedRefs.has(ref)) {
        return metadata;
      }
      visitedRefs.add(ref);
      const resolvedRef: SchemaRefResolution | undefined = SchemaValidator.resolveSchemaRefWithRoot(ref, documentRoot);
      if (resolvedRef !== undefined) {
        return SchemaValidator.mergeObjectMetadata(
          metadata,
          SchemaValidator.collectObjectMetadata(resolvedRef.schema, resolvedRef.documentRoot, visitedRefs)
        );
      }
    }

    if (Array.isArray(schema['allOf'])) {
      const allOfSchemaList: SchemaValue[] = schema['allOf'] as SchemaValue[];
      for (let index = 0; index < allOfSchemaList.length; index++) {
        const subSchemaValue: SchemaValue = allOfSchemaList[index];
        if (!SchemaValidator.isRecord(subSchemaValue)) {
          continue;
        }
        SchemaValidator.mergeObjectMetadata(
          metadata,
          SchemaValidator.collectObjectMetadata(
            subSchemaValue as SchemaObject,
            documentRoot,
            new Set<string>(visitedRefs)
          )
        );
      }
    }

    if (SchemaValidator.isRecord(schema['properties'])) {
      metadata.hasKnownProperties = true;
      Object.keys(schema['properties'] as SchemaObject).forEach((key: string) => {
        const propertySchema: SchemaValue = (schema['properties'] as SchemaObject)[key];
        if (SchemaValidator.isRecord(propertySchema)) {
          metadata.propertySchemas.set(key, propertySchema as SchemaObject);
        } else {
          metadata.propertySchemas.set(key, SchemaValidator.createSchemaObject());
        }
      });
    }

    if (Array.isArray(schema['required'])) {
      metadata.hasKnownProperties = true;
      const requiredKeyList: SchemaValue[] = schema['required'] as SchemaValue[];
      for (let index = 0; index < requiredKeyList.length; index++) {
        const requiredKeyValue: SchemaValue = requiredKeyList[index];
        if (typeof requiredKeyValue === 'string') {
          metadata.requiredKeys.add(requiredKeyValue);
        }
      }
    }

    if (SchemaValidator.isRecord(schema['additionalProperties'])) {
      metadata.additionalPropertiesSchema = schema['additionalProperties'] as SchemaObject;
    } else if (schema['additionalProperties'] === true && !metadata.hasKnownProperties) {
      metadata.allowLooseAdditionalProperties = true;
    }

    return metadata;
  }

  private static mergeObjectMetadata(
    target: ObjectSchemaMetadata,
    source: ObjectSchemaMetadata
  ): ObjectSchemaMetadata {
    source.propertySchemas.forEach((schema: SchemaObject, key: string) => {
      target.propertySchemas.set(key, schema);
    });
    source.requiredKeys.forEach((key: string) => {
      target.requiredKeys.add(key);
    });
    if (source.additionalPropertiesSchema !== undefined) {
      target.additionalPropertiesSchema = source.additionalPropertiesSchema;
    }
    target.hasKnownProperties = target.hasKnownProperties || source.hasKnownProperties;
    target.allowLooseAdditionalProperties =
      target.allowLooseAdditionalProperties || source.allowLooseAdditionalProperties;
    return target;
  }

  private static parseSchema(schemaText: string): SchemaObject | undefined {
    const normalizedText: string = schemaText.trim();
    if (normalizedText.length === 0) {
      return undefined;
    }
    const cachedSchema: SchemaObject | undefined = SchemaValidator.schemaCache.get(normalizedText);
    if (cachedSchema !== undefined) {
      return cachedSchema;
    }

    try {
      const schemaValue: Object = JSON.parse(normalizedText) as Object;
      if (!SchemaValidator.isRecord(schemaValue)) {
        return undefined;
      }
      const schemaObject: SchemaObject = schemaValue as SchemaObject;
      SchemaValidator.schemaCache.set(normalizedText, schemaObject);
      return schemaObject;
    } catch (_error) {
      return undefined;
    }
  }

  private static resolveSchemaRefWithRoot(ref: string, documentRoot: SchemaObject): SchemaRefResolution | undefined {
    if (ref.startsWith('#/')) {
      const resolvedSchema: SchemaObject | undefined = SchemaValidator.resolvePointer(documentRoot, ref.substring(1));
      return resolvedSchema !== undefined ? {
        schema: resolvedSchema,
        documentRoot: documentRoot
      } : undefined;
    }
    const commonTypesRefMarker: string = 'common_types.json#/';
    const commonTypesMarkerIndex: number = ref.indexOf(commonTypesRefMarker);
    if (commonTypesMarkerIndex >= 0) {
      const commonSchema: SchemaObject | undefined = SchemaValidator.getCommonSchema();
      if (commonSchema === undefined) {
        return undefined;
      }
      const resolvedSchema: SchemaObject | undefined =
        SchemaValidator.resolvePointer(
          commonSchema,
          ref.substring(commonTypesMarkerIndex + 'common_types.json'.length)
        );
      return resolvedSchema !== undefined ? {
        schema: resolvedSchema,
        documentRoot: commonSchema
      } : undefined;
    }
    return undefined;
  }

  private static resolvePointer(root: SchemaObject, pointer: string): SchemaObject | undefined {
    if (pointer.length === 0 || pointer === '#') {
      return root;
    }

    const normalizedPointer: string = pointer.startsWith('#') ? pointer.substring(1) : pointer;
    const pointerParts: string[] = normalizedPointer.split('/');
    const segments: string[] = [];
    for (let index = 0; index < pointerParts.length; index++) {
      const segment: string = pointerParts[index];
      if (segment.length > 0) {
        segments.push(segment);
      }
    }
    let current: SchemaValue = root;
    for (let index = 0; index < segments.length; index++) {
      const segment: string = segments[index];
      if (!SchemaValidator.isRecord(current)) {
        return undefined;
      }
      const normalizedSegment: string = segment.replace(/~1/g, '/').replace(/~0/g, '~');
      current = (current as SchemaObject)[normalizedSegment];
    }

    return SchemaValidator.isRecord(current) ? current as SchemaObject : undefined;
  }

  private static getCommonSchema(): SchemaObject | undefined {
    if (SchemaValidator.commonSchemaCache !== undefined) {
      return SchemaValidator.commonSchemaCache;
    }
    const commonSchemaText: string = SchemaResourceLoader.loadA2UISchema(
      CapabilitiesCore.getDefaultA2UIProtocolVersion(),
      'common_types.json'
    );
    const commonSchema: SchemaObject | undefined = SchemaValidator.parseSchema(commonSchemaText);
    if (commonSchema !== undefined) {
      SchemaValidator.commonSchemaCache = commonSchema;
    }
    return commonSchema;
  }

  private static buildFallbackValue(
    schema: SchemaObject,
    documentRoot?: SchemaObject,
    visitedRefs: Set<string> = new Set<string>()
  ): A2UIValueType {
    if (schema['default'] !== undefined) {
      return SchemaValidator.cloneValue(schema['default'] as A2UIValueType);
    }
    if (schema['const'] !== undefined) {
      return SchemaValidator.cloneValue(schema['const'] as A2UIValueType);
    }
    const enumValues: SchemaValue = schema['enum'];
    if (Array.isArray(enumValues) && enumValues.length > 0) {
      return SchemaValidator.cloneValue((enumValues as A2UIValueType[])[0]);
    }

    const ref: string | undefined = typeof schema['$ref'] === 'string' ? schema['$ref'] as string : undefined;
    if (ref !== undefined && !visitedRefs.has(ref)) {
      visitedRefs.add(ref);
      const resolvedRef: SchemaRefResolution | undefined =
        SchemaValidator.resolveSchemaRefWithRoot(ref, documentRoot ?? schema);
      if (resolvedRef !== undefined) {
        return SchemaValidator.buildFallbackValue(resolvedRef.schema, resolvedRef.documentRoot, visitedRefs);
      }
    }

    const schemaType: string | undefined = typeof schema['type'] === 'string' ? schema['type'] as string : undefined;
    switch (schemaType) {
      case 'string':
        return '';
      case 'number':
      case 'integer':
        return 0;
      case 'boolean':
        return false;
      case 'array':
        return [];
      case 'object':
        return SchemaValidator.createSchemaObject();
      default:
        break;
    }

    const oneOfSchemas: SchemaValue = schema['oneOf'];
    if (Array.isArray(oneOfSchemas) && oneOfSchemas.length > 0 && SchemaValidator.isRecord(oneOfSchemas[0])) {
      return SchemaValidator.buildFallbackValue(oneOfSchemas[0] as SchemaObject, documentRoot, visitedRefs);
    }
    const anyOfSchemas: SchemaValue = schema['anyOf'];
    if (Array.isArray(anyOfSchemas) && anyOfSchemas.length > 0 && SchemaValidator.isRecord(anyOfSchemas[0])) {
      return SchemaValidator.buildFallbackValue(anyOfSchemas[0] as SchemaObject, documentRoot, visitedRefs);
    }
    if (Array.isArray(schema['allOf'])) {
      const allOfSchemaList: SchemaValue[] = schema['allOf'] as SchemaValue[];
      for (let index = 0; index < allOfSchemaList.length; index++) {
        const subSchemaValue: SchemaValue = allOfSchemaList[index];
        if (SchemaValidator.isRecord(subSchemaValue)) {
          return SchemaValidator.buildFallbackValue(subSchemaValue as SchemaObject, documentRoot, visitedRefs);
        }
      }
    }

    return SchemaValidator.createSchemaObject();
  }

  private static createFallbackObject(schema: SchemaObject, documentRoot?: SchemaObject): SchemaObject {
    const fallbackValue: A2UIValueType = SchemaValidator.buildFallbackValue(schema, documentRoot);
    return SchemaValidator.isRecord(fallbackValue)
      ? fallbackValue as SchemaObject
      : SchemaValidator.createSchemaObject();
  }

  private static dropUnknownRootFields(root: SchemaObject, messageKey: string): void {
    Object.keys(root).forEach((key: string) => {
      if (key === 'version' || key === messageKey) {
        return;
      }
      SchemaValidator.recordWarning(
        SchemaErrorCode.UNDEFINED_FIELD,
        `Root field ${key} is undefined in schema and has been removed`,
        key,
        'message',
        'dsl'
      );
    });
  }

  private static collectUnknownRootKeys(root: SchemaObject): string[] {
    const unknownRootKeys: string[] = [];
    Object.keys(root).forEach((key: string) => {
      if (key === 'version' || SchemaValidator.MESSAGE_KEYS.has(key)) {
        return;
      }
      unknownRootKeys.push(key);
    });
    return unknownRootKeys;
  }

  private static buildUnknownMessageOperationError(unknownRootKeys: string[]): string {
    if (unknownRootKeys.length === 0) {
      return 'Message operation is unknown';
    }
    if (unknownRootKeys.length === 1) {
      return `Message operation is unknown: ${unknownRootKeys[0]}`;
    }
    return `Message operations are unknown: ${unknownRootKeys.join(', ')}`;
  }

  private static getSupportedMessageKeysText(): string {
    return SchemaValidator.SUPPORTED_MESSAGE_KEYS_TEXT;
  }

  private static matchesType(value: SchemaValue, schemaType: string): boolean {
    switch (schemaType) {
      case 'string':
        return typeof value === 'string';
      case 'number':
      case 'integer':
        return typeof value === 'number';
      case 'boolean':
        return typeof value === 'boolean';
      case 'array':
        return Array.isArray(value);
      case 'object':
        return SchemaValidator.isRecord(value);
      default:
        return true;
    }
  }

  private static enumContains(enumValues: A2UIValueType[], value: SchemaValue): boolean {
    for (let index = 0; index < enumValues.length; index++) {
      const enumValue: A2UIValueType = enumValues[index];
      if (SchemaValidator.isDeepEqual(enumValue, value)) {
        return true;
      }
    }
    return false;
  }

  private static isRecord(value: SchemaValue | Object | undefined | null): boolean {
    return value !== null && value !== undefined && typeof value === 'object' && !Array.isArray(value);
  }

  private static cloneValue<T>(value: T): T {
    if (value === undefined || value === null) {
      return value;
    }
    if (typeof value === 'string' || typeof value === 'number' || typeof value === 'boolean') {
      return value;
    }
    return JSON.parse(JSON.stringify(value)) as T;
  }

  private static createSchemaObject(): SchemaObject {
    return JSON.parse('{}') as SchemaObject;
  }

  private static isDeepEqual(left: A2UIValueType, right: SchemaValue): boolean {
    return JSON.stringify(left) === JSON.stringify(right);
  }

  private static joinPath(parent: string, child: string): string {
    if (parent.length === 0) {
      return child;
    }
    return `${parent}.${child}`;
  }

  private static recordWarning(
    code: SchemaErrorCode,
    message: string,
    path: string,
    itemType: SchemaItemType,
    itemName: string
  ): void {
    SchemaErrorInfoManager.getInstance().recordSchemaWarning(code, message, path, itemType, itemName);
  }

  public static validateComponentSchema(componentName: string, schema: string): string[] {
    const errors: string[] = [];
    const parsed: ESObject | null = SchemaValidator.tryParse(schema, errors, 'component');
    if (parsed === null) {
      return errors;
    }

    const constValue: string = SchemaValidator.findConstValue(parsed, 'component');
    if (constValue.length === 0) {
      errors.push(`[SchemaValidator] component schema missing properties.component.const, component=${componentName}`);
    } else if (constValue !== componentName) {
      errors.push(
        `[SchemaValidator] component schema const mismatch, component=${componentName}, schemaConst=${constValue}`
      );
    }

    if (!SchemaValidator.hasRequiredField(parsed, 'component')) {
      errors.push(`[SchemaValidator] component schema required must include component, component=${componentName}`);
    }
    return errors;
  }

  public static validateFunctionSchema(functionName: string, schema: string): string[] {
    const errors: string[] = [];
    const parsed: ESObject | null = SchemaValidator.tryParse(schema, errors, 'function');
    if (parsed === null) {
      return errors;
    }

    const constValue: string = SchemaValidator.findConstValue(parsed, 'call');
    if (constValue.length === 0) {
      errors.push(`[SchemaValidator] function schema missing properties.call.const, function=${functionName}`);
    } else if (constValue !== functionName) {
      errors.push(`[SchemaValidator] function schema const mismatch, function=${functionName}, schemaConst=${constValue}`);
    }

    if (!SchemaValidator.hasRequiredField(parsed, 'call')) {
      errors.push(`[SchemaValidator] function schema required must include call, function=${functionName}`);
    }
    if (!SchemaValidator.hasRequiredField(parsed, 'args')) {
      errors.push(`[SchemaValidator] function schema required must include args, function=${functionName}`);
    }
    if (!SchemaValidator.hasArgsObject(parsed)) {
      errors.push(`[SchemaValidator] function schema properties.args must be object, function=${functionName}`);
    }
    return errors;
  }

  private static hasRequiredField(parsed: ESObject, fieldName: string): boolean {
    if (SchemaValidator.requiredContains(parsed, fieldName)) {
      return true;
    }
    const allOfValue: Object | undefined = (parsed as Record<string, Object>)['allOf'];
    if (!Array.isArray(allOfValue)) {
      return false;
    }
    const allOfList: Object[] = allOfValue as Object[];
    for (let i = 0; i < allOfList.length; i++) {
      const node: Object = allOfList[i];
      if (node === null || Array.isArray(node) || typeof node !== 'object') {
        continue;
      }
      if (SchemaValidator.requiredContains(node as ESObject, fieldName)) {
        return true;
      }
    }
    return false;
  }

  private static requiredContains(node: ESObject, fieldName: string): boolean {
    const requiredValue: Object | undefined = (node as Record<string, Object>)['required'];
    if (!Array.isArray(requiredValue)) {
      return false;
    }
    const requiredList: Object[] = requiredValue as Object[];
    for (let i = 0; i < requiredList.length; i++) {
      if (requiredList[i] === fieldName) {
        return true;
      }
    }
    return false;
  }

  private static hasArgsObject(parsed: ESObject): boolean {
    const propertiesValue: Object | undefined = (parsed as Record<string, Object>)['properties'];
    if (propertiesValue === undefined || propertiesValue === null || Array.isArray(propertiesValue) ||
      typeof propertiesValue !== 'object') {
      return false;
    }
    const argsValue: Object | undefined = (propertiesValue as Record<string, Object>)['args'];
    if (argsValue === undefined || argsValue === null || Array.isArray(argsValue) || typeof argsValue !== 'object') {
      return false;
    }
    const typeValue: Object | undefined = (argsValue as Record<string, Object>)['type'];
    return typeValue === 'object';
  }

  private static tryParse(schema: string, errors: string[], kind: string): ESObject | null {
    const normalized: string = schema.trim();
    if (normalized.length === 0) {
      errors.push(`[SchemaValidator] ${kind} schema is empty`);
      return null;
    }
    if (normalized.length > 65536) {
      errors.push(`[SchemaValidator] ${kind} schema exceeds max length 65536`);
      return null;
    }
    try {
      const parsed: Object = JSON.parse(normalized) as Object;
      if (parsed === null || Array.isArray(parsed) || typeof parsed !== 'object') {
        errors.push(`[SchemaValidator] ${kind} schema root must be object`);
        return null;
      }
      return parsed as ESObject;
    } catch (error) {
      errors.push(`[SchemaValidator] ${kind} schema parse failed: ${JSON.stringify(error)}`);
      return null;
    }
  }

  private static findConstValue(parsed: ESObject, propertyName: string): string {
    const direct: string = SchemaValidator.readConstFromNode(parsed, propertyName);
    if (direct.length > 0) {
      return direct;
    }

    const allOfValue: Object | undefined = (parsed as Record<string, Object>)['allOf'];
    if (!Array.isArray(allOfValue)) {
      return '';
    }
    const allOfList: Object[] = allOfValue as Object[];
    for (let i = 0; i < allOfList.length; i++) {
      const node: Object = allOfList[i];
      if (node === null || Array.isArray(node) || typeof node !== 'object') {
        continue;
      }
      const value: string = SchemaValidator.readConstFromNode(node as ESObject, propertyName);
      if (value.length > 0) {
        return value;
      }
    }
    return '';
  }

  private static readConstFromNode(node: ESObject, propertyName: string): string {
    const propertiesValue: Object | undefined = (node as Record<string, Object>)['properties'];
    if (propertiesValue === undefined || propertiesValue === null || Array.isArray(propertiesValue) ||
      typeof propertiesValue !== 'object') {
      return '';
    }
    const properties: Record<string, Object> = propertiesValue as Record<string, Object>;
    const propertyNode: Object | undefined = properties[propertyName];
    if (propertyNode === undefined || propertyNode === null || Array.isArray(propertyNode) || typeof propertyNode !== 'object') {
      return '';
    }
    const constValue: Object | undefined = (propertyNode as Record<string, Object>)['const'];
    return typeof constValue === 'string' ? constValue : '';
  }
}
