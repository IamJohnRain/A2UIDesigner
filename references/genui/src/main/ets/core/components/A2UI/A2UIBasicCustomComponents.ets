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

import { createAudioPlayerDefinition } from './CustomAudioPlayer';
import { createChoicePickerDefinition } from './CustomChoicePicker';
import { CustomComponentDefinition, CustomComponentFactory } from './CustomComponentFactory';
import { createDateTimeInputDefinition } from './CustomDateTimeInput';
import { createDividerDefinition } from './CustomDivider';
import { createIconDefinition } from './CustomIcon';
import { createTabsDefinition } from './CustomTabs';
import { createVideoDefinition } from './CustomVideo';

let builtInDefinitions: CustomComponentDefinition[] | undefined = undefined;

function getOrCreateBuiltInDefinitions(): CustomComponentDefinition[] {
  if (builtInDefinitions === undefined) {
    builtInDefinitions = [
      createAudioPlayerDefinition(),
      createChoicePickerDefinition(),
      createDateTimeInputDefinition(),
      createDividerDefinition(),
      createIconDefinition(),
      createTabsDefinition(),
      createVideoDefinition()
    ];
  }
  return builtInDefinitions;
}

export function getBuiltInCustomComponentDefinitions(): CustomComponentDefinition[] {
  return getOrCreateBuiltInDefinitions().slice();
}

export function registerBuiltInCustomComponents(): void {
  const definitions: CustomComponentDefinition[] = getOrCreateBuiltInDefinitions();
  for (const definition of definitions) {
    CustomComponentFactory.registerCustomComponent(definition);
  }
}
