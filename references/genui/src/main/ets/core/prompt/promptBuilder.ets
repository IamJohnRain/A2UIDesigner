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
import { CatalogItem, CatalogItemType } from '../base/CatalogItem';
import { SchemaResourceLoader } from '../schema/SchemaResourceLoader';
import { SchemaValidator } from '../schema/SchemaValidator';
import { A2UI_BASIC_CATALOG_ID, CapabilitiesCore } from '../base/CapabilitiesCore';
import { InnerFunctionItem } from '../base/FunctionItem';
import { Catalog } from '../../interface/Catalog';

const VALIDATION_FUNCTION_NAMES: string[] = ['required', 'regex', 'length', 'numeric', 'email'];

export class PromptBuilder {
  private constructor() {
  }

  public static buildInstruction(catalog: CatalogImpl, version?: string): string {
    const resolvedVersion = CapabilitiesCore.resolveA2UIProtocolVersion(version);
    if (!resolvedVersion) {
      PromptBuilder.reportResolverErrors([`[PromptBuilder] buildInstruction failed to resolve version=${version}`]);
      return '';
    }

    const catalogId: string = catalog.getCatalogId();
    const protocolMessages: A2UIMessageType[] = PromptBuilder.protocolMessagesForVersion(resolvedVersion);

    console.info(
      `[PromptBuilder] buildInstruction version=${resolvedVersion} catalogId=${catalogId} ` +
      `components=${((catalog as CatalogImpl)?.components ?? []).length} ` +
      `functions=${((catalog as CatalogImpl)?.functions ?? []).length} protocolMessages=${protocolMessages.length}`
    );

    const schemaSection: string = PromptBuilder.buildA2UIJsonSchemaSection(
      (catalog as CatalogImpl)?.components ?? [],
      (catalog as CatalogImpl)?.functions ?? [],
      protocolMessages,
      resolvedVersion,
      catalogId
    );
    const instruction: string = [
      PromptBuilder.buildInstructionRules(resolvedVersion),
      schemaSection,
      PromptBuilder.buildInstructionExamples(resolvedVersion, catalogId)
    ].join('\n\n');
    console.info(`[PromptBuilder] buildInstruction produced ${instruction.length} chars.`);
    PromptBuilder.logPromptContent('instruction', instruction);
    return instruction;
  }

  public static getComponentSchema(componentItems: CatalogItem[], version?: string): string {
    const resolvedVersion: string | null = PromptBuilder.resolveRequestedVersion(version);
    if (resolvedVersion === null) {
      return '';
    }

    const schema: string = PromptBuilder.buildComponentSchemaDocument(componentItems, resolvedVersion);
    if (schema.length === 0) {
      return '';
    }
    return PromptBuilder.fenced(schema, 'COMPONENT SCHEMA');
  }

  public static getFunctionSchema(functionItems: InnerFunctionItem[], version?: string): string {
    const resolvedVersion: string | null = PromptBuilder.resolveRequestedVersion(version);
    if (resolvedVersion === null) {
      return '';
    }

    const catalog: Catalog = new CatalogImpl('', [], functionItems);
    const schema: string =
      PromptBuilder.buildFunctionSchemaDocument((catalog as CatalogImpl)?.functions ?? [], resolvedVersion);
    if (schema.length === 0) {
      return '';
    }
    return PromptBuilder.fenced(schema, 'FUNCTION SCHEMA');
  }

  public static getProtocolSchema(version?: string): string {
    const resolvedVersion: string | null = PromptBuilder.resolveRequestedVersion(version);
    if (resolvedVersion === null) {
      return '';
    }

    const protocolMessages: A2UIMessageType[] = PromptBuilder.protocolMessagesForVersion(resolvedVersion);
    if (protocolMessages.length === 0) {
      return '';
    }
    return PromptBuilder.buildProtocolSchemaSectionByMessages(protocolMessages, resolvedVersion);
  }

  private static buildInstructionRules(protocolVersion: string): string {
    return PromptBuilder.joinLines([
      'You are an AI assistant powered by A2UI. Always return complete, usable JSON, and validate the JSON before output.',
      `A2UI protocol version: ${protocolVersion}.`,
      '',
      'Supported messages are: `createSurface`, `updateComponents`, `updateDataModel`, `deleteSurface`.',
      '- `createSurface`: Create a new surface.',
      '- `updateComponents`: Add or update components in a surface.',
      '- `updateDataModel`: Write data into the surface data model.',
      '- `deleteSurface`: Delete a surface.',
      '',
      'Required fields:',
      '- `createSurface`: `surfaceId` (unique), `catalogId`.',
      '- `updateComponents`: `surfaceId`, `components`. One component MUST have `id: "root"`.',
      '- `updateDataModel`: `surfaceId`, `path`, `value`.',
      '- `deleteSurface`: `surfaceId`.',
      '',
      'Rules:',
      '- Return one valid A2UI JSON object for one supported message type.',
      `- Use exactly one message type per JSON object and keep \`"version": "${protocolVersion}"\`.`,
      '- To create a UI, send `createSurface` first, then `updateComponents` with the same `surfaceId`.',
      '- To update an existing UI, send `updateComponents` with the existing `surfaceId`.',
      '- Include all required component properties, even inside templates or data bindings.',
      '',
      '**REQUIRED PROPERTIES:** You MUST include ALL required properties for every component, even if they are inside a template or will be bound to data.',
      `- For 'Text', you MUST provide 'text'. If dynamic, use { "path": "..." }.`,
      `- For 'Image', you MUST provide 'url'. If dynamic, use { "path": "..." }.`,
      `- For 'Button', you MUST provide 'action'.`,
      `- For 'TextField', 'CheckBox', etc., you MUST provide 'label'.`,
      '',
      'Below are the available component, function, and message definitions:'
    ]);
  }

  private static buildInstructionExamples(protocolVersion: string, catalogId?: string): string {
    const resolvedCatalogId: string = catalogId !== undefined && catalogId.trim().length > 0
      ? catalogId.trim()
      : A2UI_BASIC_CATALOG_ID;

    return PromptBuilder.joinLines([
      '**EXAMPLES:**',
      '',
      '1. Create a surface:',
      '```json',
      '{',
      `  "version": "${protocolVersion}",`,
      '  "createSurface": {',
      '    "surfaceId": "main",',
      `    "catalogId": "${PromptBuilder.escapeJsonString(resolvedCatalogId)}",`,
      '    "sendDataModel": true',
      '  }',
      '}',
      '```',
      '',
      '2. Update components:',
      '```json',
      '{',
      `  "version": "${protocolVersion}",`,
      '  "updateComponents": {',
      '    "surfaceId": "main",',
      '    "components": [',
      '      {',
      '        "id": "root",',
      '        "component": "Column",',
      '        "justify": "start",',
      '        "children": [',
      '          "headerText"',
      '        ]',
      '      },',
      '      {',
      '        "id": "headerText",',
      '        "component": "Text",',
      '        "text": "Hello A2UI",',
      '        "variant": "h2"',
      '      }',
      '    ]',
      '  }',
      '}',
      '```',
      '',
      '3. Update data model:',
      '```json',
      '{',
      `  "version": "${protocolVersion}",`,
      '  "updateDataModel": {',
      '    "surfaceId": "main",',
      '    "path": "/profile/name",',
      '    "value": "Alice"',
      '  }',
      '}',
      '```',
      '',
      '4. Delete a surface:',
      '```json',
      '{',
      `  "version": "${protocolVersion}",`,
      '  "deleteSurface": {',
      '    "surfaceId": "main"',
      '  }',
      '}',
      '```'
    ]);
  }

  private static buildProtocolSchemaSectionByMessages(protocolMessages: A2UIMessageType[], version: string): string {
    const schema: string = PromptBuilder.buildProtocolSchemaDocument(protocolMessages, version);
    if (schema.length === 0) {
      return '';
    }

    return PromptBuilder.fenced(schema, 'PROTOCOL SCHEMA');
  }

  private static buildA2UIJsonSchemaSection(
    componentItems: CatalogItem[],
    functionItems: InnerFunctionItem[],
    protocolMessages: A2UIMessageType[],
    version: string,
    catalogId?: string
  ): string {
    const schema: string = PromptBuilder.buildCombinedA2UIJsonSchema(
      componentItems,
      functionItems,
      protocolMessages,
      version,
      catalogId
    );
    if (schema.length === 0) {
      return '';
    }

    return PromptBuilder.fenced(schema, 'A2UI JSON SCHEMA');
  }

  private static buildComponentSchemaDocument(componentItems: CatalogItem[], version: string): string {
    const componentDefinitions: string = PromptBuilder.buildCatalogDefinitionObjectForDocument(
      componentItems,
      CatalogItemType.COMPONENT,
      'component',
      'defs',
      version
    );
    const componentNames: string[] = PromptBuilder.filterCatalogItems(componentItems, CatalogItemType.COMPONENT)
      .map((item: CatalogItem) => item.name);
    const defsEntries: string[] = [];
    defsEntries.push(PromptBuilder.buildCatalogSpecificDefsContent());
    const commonDefs: string = PromptBuilder.buildCommonDefsContent('defs', version);
    if (commonDefs.length > 0) {
      defsEntries.push(commonDefs);
    }
    const validationFunctionDefs: string =
      PromptBuilder.buildRawFunctionDefinitionEntries(VALIDATION_FUNCTION_NAMES, 'defs', version);
    if (validationFunctionDefs.length > 0) {
      defsEntries.push(validationFunctionDefs);
    }
    defsEntries.push(`"anyComponent": ${PromptBuilder.buildAnyRefSchema(componentNames, '#/components/', 'component')}`);

    return PromptBuilder.joinLines([
      '{',
      '  "$schema": "https://json-schema.org/draft/2020-12/schema",',
      '  "title": "A2UI Component Schema",',
      '  "description": "Unified schema catalog of A2UI components.",',
      `  "protocolVersion": "${PromptBuilder.escapeJsonString(version)}",`,
      '  "components": {',
      PromptBuilder.indent(componentDefinitions, 4),
      '  },',
      '  "$defs": {',
      PromptBuilder.indent(defsEntries.join(',\n'), 4),
      '  }',
      '}',
    ]);
  }

  private static buildFunctionSchemaDocument(functionItems: InnerFunctionItem[], version: string): string {
    const functionDefinitions: string =
      PromptBuilder.buildFunctionDefinitionObjectForDocument(functionItems, 'defs', version);
    const functionNames: string[] = PromptBuilder.filterFunctionItems(functionItems)
      .map((item: InnerFunctionItem) => item.name);
    const defsEntries: string[] = [];
    const commonDefs: string = PromptBuilder.buildCommonDefsContent('defs', version);
    if (commonDefs.length > 0) {
      defsEntries.push(commonDefs);
    }
    const validationFunctionDefs: string =
      PromptBuilder.buildRawFunctionDefinitionEntries(VALIDATION_FUNCTION_NAMES, 'defs', version);
    if (validationFunctionDefs.length > 0) {
      defsEntries.push(validationFunctionDefs);
    }
    defsEntries.push(`"anyFunction": ${PromptBuilder.buildAnyRefSchema(functionNames, '#/functions/')}`);

    return PromptBuilder.joinLines([
      '{',
      '  "$schema": "https://json-schema.org/draft/2020-12/schema",',
      '  "title": "A2UI Function Schema",',
      '  "description": "Unified schema catalog of A2UI local functions.",',
      `  "protocolVersion": "${PromptBuilder.escapeJsonString(version)}",`,
      '  "functions": {',
      PromptBuilder.indent(functionDefinitions, 4),
      '  },',
      '  "$defs": {',
      PromptBuilder.indent(defsEntries.join(',\n'), 4),
      '  }',
      '}',
    ]);
  }

  private static buildProtocolSchemaDocument(protocolMessages: A2UIMessageType[], version: string): string {
    const uniqueMessages: A2UIMessageType[] = PromptBuilder.uniqueProtocolMessages(protocolMessages);
    const messageNames: string[] = uniqueMessages
      .map((messageType: A2UIMessageType) => PromptBuilder.protocolMessageName(messageType));
    const messageDefinitions: string = PromptBuilder.buildMessageDefinitionObject(uniqueMessages);
    const oneOfSections: string[] = messageNames.map((messageName: string) =>
      PromptBuilder.joinLines([
        '{',
        '  "required": [',
        `    "${PromptBuilder.escapeJsonString(messageName)}"`,
        '  ]',
        '}',
      ]));

    const rootProperties: string[] = [
      PromptBuilder.joinLines([
        `"version": {`,
        `  "type": "string",`,
        `  "const": "${PromptBuilder.escapeJsonString(version)}"`,
        '}',
      ]),
    ];

    for (let i = 0; i < messageNames.length; i++) {
      rootProperties.push(
        PromptBuilder.joinLines([
          `"${PromptBuilder.escapeJsonString(messageNames[i])}": {`,
          `  "$ref": "#/messages/${PromptBuilder.escapeJsonPointerToken(messageNames[i])}"`,
          '}',
        ])
      );
    }

    const defs: string = [
      `"anyComponent": ${PromptBuilder.joinLines([
        '{',
        '  "type": "object",',
        '  "description": "See the component schema section for valid component definitions."',
        '}',
      ])}`,
      `"anyMessage": ${PromptBuilder.buildAnyRefSchema(messageNames, '#/messages/')}`,
    ].join(',\n');

    return PromptBuilder.joinLines([
      '{',
      '  "$schema": "https://json-schema.org/draft/2020-12/schema",',
      '  "title": "A2UI Protocol Schema",',
       '  "description": "Describes a JSON payload for an A2UI (Agent to UI) message, which is used to dynamically construct and update user interfaces.",',
      `  "protocolVersion": "${PromptBuilder.escapeJsonString(version)}",`,
      '  "messages": {',
      PromptBuilder.indent(messageDefinitions, 4),
      '  },',
      '  "$defs": {',
      PromptBuilder.indent(defs, 4),
      '  },',
      '  "allOf": [',
      '    {',
      '      "type": "object",',
      '      "title": "A2UI Message Schema",',
       '      "description": "Describes a JSON payload for an A2UI (Agent to UI) message, which is used to dynamically construct and update user interfaces.",',
      '      "properties": {',
      PromptBuilder.indent(rootProperties.join(',\n'), 8),
      '      },',
      '      "required": [',
      '        "version"',
      '      ]',
      '    }',
      '  ],',
      '  "oneOf": [',
      PromptBuilder.indent(oneOfSections.join(',\n'), 4),
      '  ]',
      '}',
    ]);
  }

  private static buildCombinedA2UIJsonSchema(
    componentItems: CatalogItem[],
    functionItems: InnerFunctionItem[],
    protocolMessages: A2UIMessageType[],
    version: string,
    catalogId?: string
  ): string {
    const componentDefinitions: string = PromptBuilder.buildCatalogDefinitionObjectForDocument(
      componentItems,
      CatalogItemType.COMPONENT,
      'component',
      'defs',
      version
    );
    const functionDefinitions: string =
      PromptBuilder.buildFunctionDefinitionObjectForDocument(functionItems, 'defs', version);
    const uniqueMessages: A2UIMessageType[] = PromptBuilder.uniqueProtocolMessages(protocolMessages);
    const messageDefinitions: string = PromptBuilder.buildMessageDefinitionObject(
      uniqueMessages
    );
    const componentNames: string[] = PromptBuilder.filterCatalogItems(componentItems, CatalogItemType.COMPONENT)
      .map((item: CatalogItem) => item.name);
    const functionNames: string[] = PromptBuilder.filterFunctionItems(functionItems)
      .map((item: InnerFunctionItem) => item.name);
    const messageNames: string[] = uniqueMessages
      .map((messageType: A2UIMessageType) => PromptBuilder.protocolMessageName(messageType));
    const properties: string[] = [
      PromptBuilder.joinLines([
        '"version": {',
        '  "type": "string",',
        `  "const": "${PromptBuilder.escapeJsonString(version)}"`,
        '}',
      ])
    ];
    const oneOfSections: string[] = [];
    for (let i = 0; i < messageNames.length; i++) {
      properties.push(PromptBuilder.joinLines([
        `"${PromptBuilder.escapeJsonString(messageNames[i])}": {`,
        `  "$ref": "#/messages/${PromptBuilder.escapeJsonPointerToken(messageNames[i])}"`,
        '}',
      ]));
      oneOfSections.push(PromptBuilder.joinLines([
        '{',
        '  "required": [',
        `    "${PromptBuilder.escapeJsonString(messageNames[i])}"`,
        '  ]',
        '}',
      ]));
    }

    const defsEntries: string[] = [];
    defsEntries.push(PromptBuilder.buildCatalogSpecificDefsContent());
    const commonDefs: string = PromptBuilder.buildCommonDefsContent('functions', version);
    if (commonDefs.length > 0) {
      defsEntries.push(commonDefs);
    }
    defsEntries.push(`"anyComponent": ${PromptBuilder.buildAnyRefSchema(componentNames, '#/components/', 'component')}`);
    defsEntries.push(`"anyFunction": ${PromptBuilder.buildAnyRefSchema(functionNames, '#/functions/')}`);
    defsEntries.push(`"anyMessage": ${PromptBuilder.buildAnyRefSchema(messageNames, '#/messages/')}`);

    const rootLines: string[] = [
      '{',
      '  "$schema": "https://json-schema.org/draft/2020-12/schema",',
    ];
    if (catalogId !== undefined && catalogId.trim().length > 0) {
      rootLines.push(`  "$id": "${PromptBuilder.escapeJsonString(catalogId.trim())}",`);
    }
    rootLines.push(
      '  "title": "A2UI Basic Catalog",',
      '  "description": "Unified catalog of basic A2UI components and functions.",',
      `  "protocolVersion": "${PromptBuilder.escapeJsonString(version)}",`
    );
    if (catalogId !== undefined && catalogId.trim().length > 0) {
      rootLines.push(`  "catalogId": "${PromptBuilder.escapeJsonString(catalogId.trim())}",`);
    }
    rootLines.push(
      '  "components": {',
      PromptBuilder.indent(componentDefinitions, 4),
      '  },',
      '  "functions": {',
      PromptBuilder.indent(functionDefinitions, 4),
      '  },',
      '  "messages": {',
      PromptBuilder.indent(messageDefinitions, 4),
      '  },',
      '  "$defs": {',
      PromptBuilder.indent(defsEntries.join(',\n'), 4),
      '  },',
      '  "allOf": [',
      '    {',
      '      "type": "object",',
      '      "title": "A2UI Message Schema",',
       '      "description": "Describes a JSON payload for an A2UI (Agent to UI) message, which is used to dynamically construct and update user interfaces.",',
      '      "properties": {',
      PromptBuilder.indent(properties.join(',\n'), 8),
      '      },',
      '      "required": [',
      '        "version"',
      '      ]',
      '    }',
      '  ],',
      '  "oneOf": [',
      PromptBuilder.indent(oneOfSections.join(',\n'), 4),
      '  ]',
      '}',
    );

    return PromptBuilder.joinLines(rootLines);
  }

  private static protocolMessageSchema(messageType: A2UIMessageType): string {
    switch (messageType) {
      case A2UIMessageType.CREATE_SURFACE:
        return PromptBuilder.joinLines([
          '{',
          '  "type": "object",',
          '  "description": "Signals the client to create a new surface and begin rendering it. It is an error to send \'createSurface\' for a surfaceId that already exists without first deleting it. When this message is sent, the client will expect \'updateComponents\' and/or \'updateDataModel\' messages for the same surfaceId that define the component tree.",',
          '  "properties": {',
          '    "surfaceId": {',
          '      "type": "string",',
          '      "description": "The unique identifier for the UI surface to be rendered."',
          '    },',
          '    "catalogId": {',
          '      "type": "string",',
          '      "description": "A string that uniquely identifies this catalog. It is recommended to prefix this with an internet domain that you own, to avoid conflicts e.g. mycompany.com:somecatalog\'."',
          '    },',
          '    "theme": {',
          '      "type": "object",',
          '      "description": "Theme parameters for the surface (e.g., {\'primaryColor\': \'#FF0000\'}). These must validate against the \'theme\' schema defined in the catalog.",',
          '      "additionalProperties": true',
          '    },',
          '    "sendDataModel": {',
          '      "type": "boolean",',
          '      "description": "If true, the client will send the full data model of this surface in the metadata of every A2A message sent to the server that created the surface. Defaults to false."',
          '    }',
          '  },',
          '  "required": [',
          '    "surfaceId",',
          '    "catalogId"',
          '  ]',
          '}',
        ]);
      case A2UIMessageType.UPDATE_COMPONENTS: {
        return PromptBuilder.joinLines([
          '{',
          '  "type": "object",',
          '  "description": "Updates a surface with a new set of components. This message can be sent multiple times to update the component tree of an existing surface. One of the components in one of the components lists MUST have an \'id\' of \'root\' to serve as the root of the component tree. The createSurface message MUST have been previously sent with the \'catalogId\' that is in this message.",',
          '  "properties": {',
          '    "surfaceId": {',
          '      "type": "string",',
          '      "description": "The unique identifier for the UI surface to be updated."',
          '    },',
          '    "components": {',
          '      "type": "array",',
          '      "description": "A list containing all UI components for the surface.",',
          '      "items": {',
          '        "$ref": "#/$defs/anyComponent"',
          '      },',
          '      "minItems": 1',
          '    }',
          '  },',
          '  "required": [',
          '    "surfaceId",',
          '    "components"',
          '  ]',
          '}',
        ]);
      }
      case A2UIMessageType.UPDATE_DATA_MODEL:
        return PromptBuilder.joinLines([
          '{',
          '  "type": "object",',
          '  "description": "Updates the data model for an existing surface. This message can be sent multiple times to update the data model. The createSurface message MUST have been previously sent with the \'catalogId\' that is in this message.",',
          '  "properties": {',
          '    "surfaceId": {',
          '      "type": "string",',
          '      "description": "The unique identifier for the UI surface this data model update applies to."',
          '    },',
          '    "path": {',
          '      "type": "string",',
          '      "description": "An optional path to a location within the data model (e.g., \'/user/name\'). If omitted, or set to \'/\', refers to the entire data model.",',
          '      "default": "/"',
          '    },',
          '    "value": {',
          '      "description": "The data to be updated in the data model. If present, the value at \'path\' is replaced (or created). If omitted, the key at \'path\' is removed."',
          '    }',
          '  },',
          '  "required": [',
          '    "surfaceId"',
          '  ]',
          '}',
        ]);
      case A2UIMessageType.DELETE_SURFACE:
        return PromptBuilder.joinLines([
          '{',
          '  "type": "object",',
          '  "description": "Signals the client to delete the surface identified by \'surfaceId\'. The createSurface message MUST have been previously sent with the \'catalogId\' that is in this message.",',
          '  "properties": {',
          '    "surfaceId": {',
          '      "type": "string",',
          '      "description": "The unique identifier for the UI surface to be deleted."',
          '    }',
          '  },',
          '  "required": [',
          '    "surfaceId"',
          '  ]',
          '}',
        ]);
      default:
        return PromptBuilder.joinLines([
          '{',
          '  "type": "object"',
          '}',
        ]);
    }
  }

  private static buildMessageDefinitionObject(protocolMessages: A2UIMessageType[]): string {
    const entries: string[] = [];
    for (let i = 0; i < protocolMessages.length; i++) {
      const messageName: string = PromptBuilder.protocolMessageName(protocolMessages[i]);
      entries.push(`"${PromptBuilder.escapeJsonString(messageName)}": ${PromptBuilder.protocolMessageSchema(protocolMessages[i])}`);
    }

    return entries.join(',\n');
  }

  private static buildCatalogDefinitionObjectForDocument(
    items: CatalogItem[],
    expectedType: CatalogItemType,
    itemLabel: string,
    functionRefMode: 'defs' | 'functions',
    version: string
  ): string {
    const filteredItems: CatalogItem[] = PromptBuilder.filterCatalogItems(items, expectedType);
    const entries: string[] = [];
    for (let i = 0; i < filteredItems.length; i++) {
      entries.push(
        `"${PromptBuilder.escapeJsonString(filteredItems[i].name)}": ` +
        PromptBuilder.catalogItemSchemaForDocument(filteredItems[i], itemLabel, functionRefMode, version)
      );
    }
    return entries.join(',\n');
  }

  private static buildFunctionDefinitionObjectForDocument(
    items: InnerFunctionItem[],
    functionRefMode: 'defs' | 'functions',
    version: string
  ): string {
    const filteredItems: InnerFunctionItem[] = PromptBuilder.filterFunctionItems(items);
    const entries: string[] = [];
    for (let i = 0; i < filteredItems.length; i++) {
      entries.push(
        `"${PromptBuilder.escapeJsonString(filteredItems[i].name)}": ` +
        PromptBuilder.functionItemSchemaForDocument(filteredItems[i], functionRefMode, version)
      );
    }
    return entries.join(',\n');
  }

  private static buildRawFunctionDefinitionEntries(
    functionNames: string[],
    functionRefMode: 'defs' | 'functions',
    version: string
  ): string {
    const entries: string[] = [];
    for (let i = 0; i < functionNames.length; i++) {
      const schema: string = PromptBuilder.rewriteSchemaReferences(
        SchemaResourceLoader.loadA2UISchema(version, `functions/${functionNames[i]}.json`),
        functionRefMode
      );
      if (schema.length > 0) {
        entries.push(`"${PromptBuilder.escapeJsonString(functionNames[i])}": ${schema}`);
      }
    }
    return entries.join(',\n');
  }

  private static buildCommonDefsContent(functionRefMode: 'defs' | 'functions', version: string): string {
    const commonSchema: string = PromptBuilder.rewriteSchemaReferences(
      SchemaResourceLoader.loadA2UISchema(version, 'common_types.json'),
      functionRefMode
    );
    if (commonSchema.length === 0) {
      return '';
    }
    return PromptBuilder.extractObjectPropertyContent(commonSchema, '$defs');
  }

  private static buildCatalogSpecificDefsContent(): string {
    return PromptBuilder.joinLines([
      '"CatalogComponentCommon": {',
      '  "type": "object",',
      '  "properties": {',
      '    "weight": {',
      '      "type": "number",',
      '      "description": "The relative weight of this component within a Row or Column. This is similar to the CSS \'flex-grow\' property. Layout allocation: Children without weight are laid out first and allocated their preferred space; remaining space is then distributed among children with weight, proportionally to their weight values. Note: this may ONLY be set when the component is a direct descendant of a Row or Column."',
      '    }',
      '  }',
      '},',
      '"theme": {',
      '  "type": "object",',
      '  "properties": {',
      '    "primaryColor": {',
      '      "type": "string",',
      '      "description": "The primary brand color used for highlights (e.g., primary buttons, active borders). Renderers may generate variants of this color for different contexts. Format: Hexadecimal code (e.g., \'#00BFFF\').",',
      '      "pattern": "^#[0-9a-fA-F]{6}$"',
      '    },',
      '    "iconUrl": {',
      '      "type": "string",',
      '      "format": "uri",',
      '      "description": "A URL for an image that identifies the agent or tool associated with the surface."',
      '    },',
      '    "agentDisplayName": {',
      '      "type": "string",',
      '      "description": "Text to be displayed next to the surface to identify the agent or tool that created it."',
      '    }',
      '  },',
      '  "additionalProperties": true',
      '}'
    ]);
  }

  private static catalogItemSchemaForDocument(
    item: CatalogItem,
    itemLabel: string,
    functionRefMode: 'defs' | 'functions',
    version: string
  ): string {
    const rawSchema: string = item.schemaProvider(version).trim();
    const validationErrors: string[] = item.type === CatalogItemType.COMPONENT
      ? SchemaValidator.validateComponentSchema(item.name, rawSchema)
      : SchemaValidator.validateFunctionSchema(item.name, rawSchema);
    if (validationErrors.length > 0) {
      for (let i = 0; i < validationErrors.length; i++) {
        PromptBuilder.reportError(validationErrors[i]);
      }
    }
    const schema: string = PromptBuilder.rewriteSchemaReferences(rawSchema, functionRefMode);
    if (schema.length > 0) {
      return schema;
    }

    return PromptBuilder.joinLines([
      '{',
      '  "type": "object",',
      `  "description": "No detailed schema was provided for ${itemLabel} \\"${PromptBuilder.escapeJsonString(item.name)}\\"."`,
      '}',
    ]);
  }

  private static functionItemSchemaForDocument(
    item: InnerFunctionItem,
    functionRefMode: 'defs' | 'functions',
    version: string
  ): string {
    const rawSchema: string = item.schemaProvider(version).trim();
    const validationErrors: string[] = SchemaValidator.validateFunctionSchema(item.name, rawSchema);
    if (validationErrors.length > 0) {
      for (let i = 0; i < validationErrors.length; i++) {
        PromptBuilder.reportError(validationErrors[i]);
      }
    }
    const schema: string = PromptBuilder.rewriteSchemaReferences(rawSchema, functionRefMode);
    if (schema.length > 0) {
      return schema;
    }

    return PromptBuilder.joinLines([
      '{',
      '  "type": "object",',
      `  "description": "No detailed schema was provided for function \\"${PromptBuilder.escapeJsonString(item.name)}\\"."`,
      '}',
    ]);
  }

  private static rewriteSchemaReferences(
    schema: string,
    functionRefMode: 'defs' | 'functions'
  ): string {
    const normalized: string = PromptBuilder.normalizeJsonSchema(schema);
    if (normalized.length === 0) {
      return '';
    }

    let rewritten: string = normalized.replace(/\.\.\/common_types\.json#\/\$defs\//g, '#/$defs/');
    const functionRefPrefix: string = functionRefMode === 'functions' ? '#/functions/' : '#/$defs/';
    rewritten = rewritten.replace(/functions\/([A-Za-z0-9_]+)\.json/g, `${functionRefPrefix}$1`);
    return rewritten;
  }

  private static extractObjectPropertyContent(json: string, propertyName: string): string {
    const propertyToken: string = `"${propertyName}"`;
    const propertyIndex: number = json.indexOf(propertyToken);
    if (propertyIndex < 0) {
      return '';
    }

    const braceIndex: number = json.indexOf('{', propertyIndex + propertyToken.length);
    if (braceIndex < 0) {
      return '';
    }

    return PromptBuilder.extractObjectBody(json, braceIndex);
  }

  private static extractObjectBody(json: string, openBraceIndex: number): string {
    let depth: number = 0;
    let inString: boolean = false;
    let escaped: boolean = false;
    for (let i = openBraceIndex; i < json.length; i++) {
      const current: string = json.charAt(i);
      if (inString) {
        if (escaped) {
          escaped = false;
        } else if (current === '\\') {
          escaped = true;
        } else if (current === '"') {
          inString = false;
        }
        continue;
      }

      if (current === '"') {
        inString = true;
      } else if (current === '{') {
        depth += 1;
      } else if (current === '}') {
        depth -= 1;
        if (depth === 0) {
          return json.substring(openBraceIndex + 1, i).trim();
        }
      }
    }
    return '';
  }

  private static buildAnyRefSchema(
    names: string[],
    prefix: string,
    discriminatorProperty?: string
  ): string {
    if (names.length === 0) {
      return PromptBuilder.joinLines([
        '{',
        '  "type": "object",',
        '  "description": "No schema was provided."',
        '}',
      ]);
    }

    const refEntries: string = names.map((name: string) =>
      PromptBuilder.joinLines([
        '{',
        `  "$ref": "${prefix}${PromptBuilder.escapeJsonPointerToken(name)}"`,
        '}',
      ])).join(',\n');

    const lines: string[] = [
      '{',
      '  "oneOf": [',
      PromptBuilder.indent(refEntries, 4),
      '  ]',
    ];

    if (discriminatorProperty !== undefined && discriminatorProperty.length > 0) {
      lines[lines.length - 1] = '  ],';
      lines.push(
        '  "discriminator": {',
        `    "propertyName": "${PromptBuilder.escapeJsonString(discriminatorProperty)}"`,
        '  }'
      );
    }

    lines.push('}');
    return PromptBuilder.joinLines(lines);
  }

  private static filterCatalogItems(items: CatalogItem[], expectedType: CatalogItemType): CatalogItem[] {
    const filteredItems: CatalogItem[] = [];
    for (let i = 0; i < items.length; i++) {
      if (items[i].type === expectedType) {
        filteredItems.push(items[i]);
      }
    }
    return filteredItems;
  }

  private static filterFunctionItems(items: InnerFunctionItem[]): InnerFunctionItem[] {
    const filteredItems: InnerFunctionItem[] = [];
    for (let i = 0; i < items.length; i++) {
      if (items[i].name.trim().length > 0) {
        filteredItems.push(items[i]);
      }
    }
    return filteredItems;
  }

  private static protocolMessageName(messageType: A2UIMessageType): string {
    switch (messageType) {
      case A2UIMessageType.CREATE_SURFACE:
        return 'createSurface';
      case A2UIMessageType.UPDATE_COMPONENTS:
        return 'updateComponents';
      case A2UIMessageType.UPDATE_DATA_MODEL:
        return 'updateDataModel';
      case A2UIMessageType.DELETE_SURFACE:
        return 'deleteSurface';
      default:
        return 'unknownMessage';
    }
  }

  private static defaultProtocolMessages(): A2UIMessageType[] {
    return [
      A2UIMessageType.CREATE_SURFACE,
      A2UIMessageType.UPDATE_COMPONENTS,
      A2UIMessageType.UPDATE_DATA_MODEL,
      A2UIMessageType.DELETE_SURFACE,
    ];
  }

  private static protocolMessagesForVersion(version: string): A2UIMessageType[] {
    if (CapabilitiesCore.isSupportedA2UIProtocolVersion(version)) {
      return PromptBuilder.defaultProtocolMessages();
    }
    return [];
  }

  private static resolveRequestedVersion(version?: string): string | null {
    return CapabilitiesCore.resolveA2UIProtocolVersion(version);
  }

  private static reportResolverErrors(errors: string[]): void {
    for (let i = 0; i < errors.length; i++) {
      PromptBuilder.reportError(errors[i]);
    }
  }

  private static uniqueProtocolMessages(protocolMessages: A2UIMessageType[]): A2UIMessageType[] {
    const results: A2UIMessageType[] = [];
    for (let i = 0; i < protocolMessages.length; i++) {
      const message: A2UIMessageType = protocolMessages[i];
      if (results.indexOf(message) < 0) {
        results.push(message);
      }
    }

    if (results.length === 0) {
      return PromptBuilder.defaultProtocolMessages();
    }
    return results;
  }

  private static joinLines(lines: string[]): string {
    return lines.join('\n');
  }

  private static indent(value: string, spaces: number): string {
    const prefix: string = ' '.repeat(spaces);
    return value
      .split('\n')
      .map((line: string) => `${prefix}${line}`)
      .join('\n');
  }

  private static fenced(content: string, sectionName: string): string {
    const normalized: string = sectionName.toUpperCase().replace(/ /g, '_');
    return `-----${normalized}_START-----\n${content.trim()}\n-----${normalized}_END-----`;
  }

  private static escapeJsonString(value: string): string {
    return value.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
  }

  private static escapeJsonPointerToken(value: string): string {
    return value.replace(/~/g, '~0').replace(/\//g, '~1');
  }

  private static normalizeJsonSchema(schema: string): string {
    if (schema.trim().length === 0) {
      return '';
    }

    try {
      return JSON.stringify(JSON.parse(schema), null, 2);
    } catch (_error) {
      return schema.trim();
    }
  }

  private static logPromptContent(label: string, content: string): void {
    const normalized: string = label.toUpperCase().replace(/ /g, '_');
    console.info(`-----PROMPT_BUILDER_${normalized}_START-----`);
    const lines: string[] = content.split('\n');
    for (let i = 0; i < lines.length; i++) {
      console.info(lines[i]);
    }
    console.info(`-----PROMPT_BUILDER_${normalized}_END-----`);
  }

  private static reportError(message: string): void {
    console.error(message);
  }
}
