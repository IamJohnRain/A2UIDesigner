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

import { CatalogItem, EMPTY_COMPONENT_BUILDER } from '../../base/CatalogItem';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { CustomComponentDefinition, CustomComponentFactory } from './CustomComponentFactory';
import { createExtendedSelectDefinition } from '../extended/ExtendedSelect';
import { createExtendedTabContentDefinition } from '../extended/ExtendedTabContent';
import { createExtendedTabsDefinition } from '../extended/ExtendedTabs';
import { createExtendedWebDefinition } from '../extended/ExtendedWeb';

const EXTENDED_NATIVE_COMPONENT_NAMES: string[] = [
  'Button',
  'Text',
  'TextInput',
  'Row',
  'Column',
  'List',
  'Stack',
  'Grid',
  'Image',
  'NavContainer',
  'Divider',
  'Toggle',
  'Progress',
  'Radio',
  'Checkbox',
  'CheckboxGroup',
  'If'
];

const EXTENDED_NATIVE_SCHEMA_FILES: Record<string, string> = {
  'Button': 'ExtendedButton.json',
  'Column': 'ExtendedColumn.json',
  'Grid': 'ExtendedGrid.json',
  'List': 'ExtendedList.json',
  'NavContainer': 'NavContainer.json',
  'Row': 'ExtendedRow.json',
  'Stack': 'ExtendedStack.json',
  'TextInput': 'ExtendedTextInput.json',
  'Toggle': 'ExtendedToggle.json',
  'Radio': 'ExtendedRadio.json',
  'Checkbox': 'ExtendedCheckBox.json',
  'CheckboxGroup': 'ExtendedCheckboxGroup.json'
};

function createMinimalComponentSchema(componentName: string): string {
  return [
    '{',
    '  "type": "object",',
    '  "properties": {',
    `    "component": { "const": "${componentName}" }`,
    '  },',
    '  "required": ["component"],',
    '  "additionalProperties": true',
    '}'
  ].join('\n');
}

function createExtendedNativeSchema(componentName: string): string {
  const fileName: string | undefined = EXTENDED_NATIVE_SCHEMA_FILES[componentName];
  if (fileName === undefined) {
    return createMinimalComponentSchema(componentName);
  }
  return SchemaResourceLoader.loadSchema(`schema/Extended/components/${fileName}`);
}

function createExtendedTextSchema(): string {
  return `{
  "type": "object",
  "$defs": {
    "DataBinding": {
      "type": "object",
      "properties": {
        "path": { "type": "string", "default": "" }
      },
      "required": ["path"],
      "additionalProperties": false
    },
    "FunctionCall": {
      "type": "object",
      "properties": {
        "call": { "type": "string", "default": "" },
        "args": { "type": "object", "additionalProperties": {} },
        "returnType": {
          "type": "string",
          "enum": ["string","number","boolean","array","object","any","void"],
          "default": "any"
        }
      },
      "required": ["call"],
      "additionalProperties": {}
    },
    "DynamicStringFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "string" } } }
      ]
    },
    "DynamicNumberFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "number" } } }
      ]
    },
    "DynamicObjectFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "object" } } }
      ]
    },
    "DynamicString": {
      "oneOf": [
        { "type": "string", "default": "" },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicStringFunction" }
      ]
    },
    "DynamicNumber": {
      "oneOf": [
        { "type": "number", "default": 0 },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicNumberFunction" }
      ]
    },
    "DynamicStringOrNumber": {
      "oneOf": [
        { "type": "string", "default": "" },
        { "type": "number", "default": 0 },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicStringFunction" },
        { "$ref": "#/$defs/DynamicNumberFunction" }
      ]
    },
    "DynamicBoolean": {
      "oneOf": [
        { "type": "boolean", "default": false },
        { "$ref": "#/$defs/DataBinding" }
      ]
    },
    "TextOverflowLiteral": {
      "type": "string",
      "enum": ["none","clip","ellipsis","marquee"],
      "default": "clip"
    },
    "TextAlignLiteral": {
      "type": "string",
      "enum": ["start","center","end","justify"],
      "default": "start"
    },
    "WordBreakLiteral": {
      "type": "string",
      "enum": ["normal","breakAll","breakWord","hyphenation"],
      "default": "breakWord"
    },
    "FontScaleModeLiteral": {
      "type": "string",
      "enum": ["followSystem","custom"],
      "default": "followSystem"
    },
    "DynamicFontScaleMode": {
      "oneOf": [
        { "$ref": "#/$defs/FontScaleModeLiteral" },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicStringFunction" }
      ]
    },
    "DecorationLiteral": {
      "type": "object",
      "properties": {
        "type": {
          "type": "string",
          "enum": ["none","underline","overline","linethrough"],
          "default": "none"
        },
        "color": { "$ref": "#/$defs/DynamicStringOrNumber" },
        "style": {
          "type": "string",
          "enum": ["solid","double","dotted","dashed","wavy"],
          "default": "solid"
        },
        "thicknessScale": { "$ref": "#/$defs/DynamicStringOrNumber" }
      },
      "required": ["type"],
      "additionalProperties": {}
    },
    "DynamicDecoration": {
      "oneOf": [
        { "$ref": "#/$defs/DecorationLiteral" },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicObjectFunction" }
      ]
    }
  },
  "properties": {
    "component": { "const": "Text" },
    "id": { "type": "string", "default": "" },
    "content": { "$ref": "#/$defs/DynamicString" },
    "styles": {
      "type": "object",
      "properties": {
        "fontSize": { "$ref": "#/$defs/DynamicStringOrNumber" },
        "fontWeight": { "$ref": "#/$defs/DynamicStringOrNumber" },
        "fontColor": { "$ref": "#/$defs/DynamicString" },
        "textAlign": {
          "oneOf": [
            { "$ref": "#/$defs/TextAlignLiteral" },
            { "$ref": "#/$defs/DataBinding" },
            { "$ref": "#/$defs/DynamicStringFunction" }
          ]
        },
        "maxLines": { "$ref": "#/$defs/DynamicNumber" },
        "textOverflow": {
          "oneOf": [
            { "$ref": "#/$defs/TextOverflowLiteral" },
            { "$ref": "#/$defs/DataBinding" },
            { "$ref": "#/$defs/DynamicStringFunction" }
          ]
        },
        "wordBreak": {
          "oneOf": [
            { "$ref": "#/$defs/WordBreakLiteral" },
            { "$ref": "#/$defs/DataBinding" },
            { "$ref": "#/$defs/DynamicStringFunction" }
          ]
        },
        "maxFontSize": { "$ref": "#/$defs/DynamicNumber" },
        "minFontSize": { "$ref": "#/$defs/DynamicNumber" },
        "fontScaleMode": { "$ref": "#/$defs/DynamicFontScaleMode" },
        "minFontScale": { "$ref": "#/$defs/DynamicNumber" },
        "maxFontScale": { "$ref": "#/$defs/DynamicNumber" },
        "decoration": { "$ref": "#/$defs/DynamicDecoration" }
      },
      "additionalProperties": true
    }
  },
  "required": ["component"],
  "additionalProperties": true
}`.trim();
}

function createExtendedImageSchema(): string {
  return `{
  "type": "object",
  "$defs": {
    "DataBinding": {
      "type": "object",
      "properties": { "path": { "type": "string", "default": "" } },
      "required": ["path"],
      "additionalProperties": false
    },
    "FunctionCall": {
      "type": "object",
      "properties": {
        "call": { "type": "string", "default": "" },
        "args": { "type": "object", "additionalProperties": {} },
        "returnType": {
          "type": "string",
          "enum": ["string","number","boolean","array","object","any","void"],
          "default": "any"
        }
      },
      "required": ["call"],
      "additionalProperties": {}
    },
    "DynamicStringFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "string" } } }
      ]
    },
    "DynamicNumberFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "number" } } }
      ]
    },
    "DynamicString": {
      "oneOf": [
        { "type": "string", "default": "" },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicStringFunction" }
      ]
    },
    "DynamicNumber": {
      "oneOf": [
        { "type": "number", "default": 0 },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicNumberFunction" }
      ]
    },
    "ObjectFitLiteral": {
      "type": "string",
      "enum": [
        "fill","contain","cover","auto","none","scaleDown",
        "topStart","top","topEnd","start","center","end",
        "bottomStart","bottom","bottomEnd","matrix"
      ],
      "default": "cover"
    }
  },
  "properties": {
    "component": { "const": "Image" },
    "id": { "type": "string", "default": "" },
    "src": {
      "$ref": "#/$defs/DynamicString",
      "description": "Image source URL or local resource path. Base64-encoded SVG data URIs are not supported."
    },
    "styles": {
      "type": "object",
      "properties": {
        "aspectRatio": { "$ref": "#/$defs/DynamicNumber" },
        "fillColor": {
          "type": "string",
          "pattern": "^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$",
          "description": "Static SVG fill color in #RRGGBB or #AARRGGBB format."
        },
        "objectFit": {
          "oneOf": [
            { "$ref": "#/$defs/ObjectFitLiteral" },
            { "$ref": "#/$defs/DataBinding" },
            { "$ref": "#/$defs/DynamicStringFunction" }
          ]
        }
      },
      "additionalProperties": true
    }
  },
  "required": ["component", "src"],
  "additionalProperties": true
}`.trim();
}

function createExtendedDividerSchema(): string {
  return `{
  "type": "object",
  "$defs": {
    "DataBinding": {
      "type": "object",
      "properties": { "path": { "type": "string", "default": "" } },
      "required": ["path"],
      "additionalProperties": false
    },
    "FunctionCall": {
      "type": "object",
      "properties": {
        "call": { "type": "string", "default": "" },
        "args": { "type": "object", "additionalProperties": {} },
        "returnType": {
          "type": "string",
          "enum": ["string","number","boolean","array","object","any","void"],
          "default": "any"
        }
      },
      "required": ["call"],
      "additionalProperties": {}
    },
    "DynamicStringFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "string" } } }
      ]
    },
    "DynamicNumberFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "number" } } }
      ]
    },
    "DynamicString": {
      "oneOf": [
        { "type": "string", "default": "" },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicStringFunction" }
      ]
    },
    "DynamicNumber": {
      "oneOf": [
        { "type": "number", "default": 0 },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicNumberFunction" }
      ]
    },
    "DynamicBoolean": {
      "oneOf": [
        { "type": "boolean", "default": false },
        { "$ref": "#/$defs/DataBinding" }
      ]
    },
    "DynamicStrokeWidth": {
      "oneOf": [
        { "type": "number" },
        { "type": "string" },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicStringFunction" },
        { "$ref": "#/$defs/DynamicNumberFunction" }
      ]
    }
  },
  "properties": {
    "component": { "const": "Divider" },
    "id": { "type": "string", "default": "" },
    "styles": {
      "type": "object",
      "properties": {
        "strokeWidth": { "$ref": "#/$defs/DynamicStrokeWidth" },
        "vertical": { "$ref": "#/$defs/DynamicBoolean" },
        "color": { "$ref": "#/$defs/DynamicString" }
      },
      "additionalProperties": true
    }
  },
  "required": ["component"],
  "additionalProperties": true
}`.trim();
}

function createExtendedProgressSchema(): string {
  return `{
  "type": "object",
  "$defs": {
    "DataBinding": {
      "type": "object",
      "properties": { "path": { "type": "string", "default": "" } },
      "required": ["path"],
      "additionalProperties": false
    },
    "FunctionCall": {
      "type": "object",
      "properties": {
        "call": { "type": "string", "default": "" },
        "args": { "type": "object", "additionalProperties": {} },
        "returnType": {
          "type": "string",
          "enum": ["string","number","boolean","array","object","any","void"],
          "default": "any"
        }
      },
      "required": ["call"],
      "additionalProperties": {}
    },
    "DynamicStringFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "string" } } }
      ]
    },
    "DynamicNumberFunction": {
      "allOf": [
        { "$ref": "#/$defs/FunctionCall" },
        { "properties": { "returnType": { "const": "number" } } }
      ]
    },
    "DynamicString": {
      "oneOf": [
        { "type": "string", "default": "" },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicStringFunction" }
      ]
    },
    "DynamicNumber": {
      "oneOf": [
        { "type": "number", "default": 0 },
        { "$ref": "#/$defs/DataBinding" },
        { "$ref": "#/$defs/DynamicNumberFunction" }
      ]
    },
    "ProgressTypeLiteral": {
      "type": "string",
      "enum": ["linear","ring","eclipse","scaleRing","capsule"],
      "default": "linear"
    }
  },
  "properties": {
    "component": { "const": "Progress" },
    "id": { "type": "string", "default": "" },
    "value": { "$ref": "#/$defs/DynamicNumber" },
    "total": { "$ref": "#/$defs/DynamicNumber" },
    "styles": {
      "type": "object",
      "properties": {
        "color": { "$ref": "#/$defs/DynamicString" },
        "type": {
          "oneOf": [
            { "$ref": "#/$defs/ProgressTypeLiteral" },
            { "$ref": "#/$defs/DataBinding" },
            { "$ref": "#/$defs/DynamicStringFunction" }
          ]
        }
      },
      "additionalProperties": true
    }
  },
  "required": ["component"],
  "additionalProperties": true
}`.trim();
}

function createExtendedNativeCatalogItem(componentName: string): CatalogItem {
  if (componentName === 'Text') {
    return CatalogItem.forComponent(
      componentName,
      (): string => createExtendedTextSchema(),
      EMPTY_COMPONENT_BUILDER
    );
  }
  if (componentName === 'Image') {
    return CatalogItem.forComponent(
      componentName,
      (): string => createExtendedImageSchema(),
      EMPTY_COMPONENT_BUILDER
    );
  }
  if (componentName === 'Divider') {
    return CatalogItem.forComponent(
      componentName,
      (): string => createExtendedDividerSchema(),
      EMPTY_COMPONENT_BUILDER
    );
  }
  if (componentName === 'Progress') {
    return CatalogItem.forComponent(
      componentName,
      (): string => createExtendedProgressSchema(),
      EMPTY_COMPONENT_BUILDER
    );
  }
  return CatalogItem.forComponent(
    componentName,
    (): string => createExtendedNativeSchema(componentName),
    EMPTY_COMPONENT_BUILDER
  );
}

function getExtendedCustomDefinitions(): CustomComponentDefinition[] {
  return [
    createExtendedTabContentDefinition(),
    createExtendedSelectDefinition(),
    createExtendedTabsDefinition(),
    createExtendedWebDefinition()
  ];
}

export function allA2UIExtendedComponents(): CatalogItem[] {
  const components: CatalogItem[] = [];
  for (const componentName of EXTENDED_NATIVE_COMPONENT_NAMES) {
    components.push(createExtendedNativeCatalogItem(componentName));
  }

  for (const definition of getExtendedCustomDefinitions()) {
    CustomComponentFactory.registerCustomComponent(definition);
    components.push(CustomComponentFactory.createCatalogItem(definition.type));
  }
  return components;
}
