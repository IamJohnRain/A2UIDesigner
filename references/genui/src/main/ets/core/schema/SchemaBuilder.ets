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

export class ComponentSchemaBuilder {
  private readonly componentName: string;
  private readonly requiredProperties: string[] = [];
  private readonly properties: string[] = [];

  private constructor(componentName: string) {
    this.componentName = componentName.trim();
  }

  public static forComponent(componentName: string): ComponentSchemaBuilder {
    return new ComponentSchemaBuilder(componentName);
  }

  public addProperty(name: string, schemaJson: string): ComponentSchemaBuilder {
    const propertyName: string = name.trim();
    if (propertyName.length === 0) {
      return this;
    }
    const schema: string = schemaJson.trim();
    if (schema.length === 0) {
      return this;
    }
    this.properties.push(`"${ComponentSchemaBuilder.escape(propertyName)}": ${schema}`);
    return this;
  }

  public addRequired(name: string): ComponentSchemaBuilder {
    const propertyName: string = name.trim();
    if (propertyName.length === 0) {
      return this;
    }
    if (this.requiredProperties.indexOf(propertyName) < 0) {
      this.requiredProperties.push(propertyName);
    }
    return this;
  }

  public build(): string {
    const required: string[] = ['component'];
    for (let i = 0; i < this.requiredProperties.length; i++) {
      if (required.indexOf(this.requiredProperties[i]) < 0) {
        required.push(this.requiredProperties[i]);
      }
    }
    const propertyLines: string[] = [
      `"component": { "const": "${ComponentSchemaBuilder.escape(this.componentName)}" }`
    ];
    for (let i = 0; i < this.properties.length; i++) {
      propertyLines.push(this.properties[i]);
    }
    const requiredLines: string[] = required.map((item: string) => `"${ComponentSchemaBuilder.escape(item)}"`);
    return [
      '{',
      '  "allOf": [',
      '    { "$ref": "#/$defs/ComponentCommon" },',
      '    {',
      '      "type": "object",',
      '      "properties": {',
      `        ${propertyLines.join(',\n        ')}`,
      '      },',
      `      "required": [${requiredLines.join(', ')}]`,
      '    }',
      '  ]',
      '}'
    ].join('\n');
  }

  private static escape(value: string): string {
    return value.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
  }
}

export class FunctionSchemaBuilder {
  private readonly functionName: string;
  private readonly argsProperties: string[] = [];
  private readonly argsRequired: string[] = [];
  private returnType: string = 'boolean';

  private constructor(functionName: string) {
    this.functionName = functionName.trim();
  }

  public static forFunction(functionName: string): FunctionSchemaBuilder {
    return new FunctionSchemaBuilder(functionName);
  }

  public addArg(name: string, schemaJson: string): FunctionSchemaBuilder {
    const argName: string = name.trim();
    if (argName.length === 0) {
      return this;
    }
    const schema: string = schemaJson.trim();
    if (schema.length === 0) {
      return this;
    }
    this.argsProperties.push(`"${FunctionSchemaBuilder.escape(argName)}": ${schema}`);
    return this;
  }

  public addRequiredArg(name: string): FunctionSchemaBuilder {
    const argName: string = name.trim();
    if (argName.length === 0) {
      return this;
    }
    if (this.argsRequired.indexOf(argName) < 0) {
      this.argsRequired.push(argName);
    }
    return this;
  }

  public setReturnType(returnType: string): FunctionSchemaBuilder {
    const normalized: string = returnType.trim();
    if (normalized.length > 0) {
      this.returnType = normalized;
    }
    return this;
  }

  public build(): string {
    const requiredArgs: string = this.argsRequired.map((item: string) => `"${FunctionSchemaBuilder.escape(item)}"`).join(', ');
    return [
      '{',
      '  "type": "object",',
      '  "properties": {',
      `    "call": { "const": "${FunctionSchemaBuilder.escape(this.functionName)}" },`,
      '    "args": {',
      '      "type": "object",',
      '      "properties": {',
      `        ${this.argsProperties.join(',\n        ')}`,
      '      },',
      `      "required": [${requiredArgs}]`,
      '    },',
      `    "returnType": { "const": "${FunctionSchemaBuilder.escape(this.returnType)}" }`,
      '  },',
      '  "required": ["call", "args"]',
      '}'
    ].join('\n');
  }

  private static escape(value: string): string {
    return value.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
  }
}
