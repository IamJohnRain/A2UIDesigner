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

import { harTasks } from '@ohos/hvigor-ohos-plugin';
import * as fs from 'fs'
import * as path from 'path'

function checkPublicExportsFromInterfaceOnly(): void {
  const indexPath = path.resolve(__dirname, 'Index.ets');
  const indexContent = fs.readFileSync(indexPath, 'utf-8');
  const exportPattern = /export\s+(?:type\s+)?(?:\{[\s\S]*?\}|\*)\s+from\s+['"]([^'"]+)['"]/g;
  const invalidExports: string[] = [];
  let match: RegExpExecArray | null = exportPattern.exec(indexContent);
  while (match !== null) {
    const exportPath = match[1];
    if (!exportPath.includes('/interface/')) {
      invalidExports.push(exportPath);
    }
    match = exportPattern.exec(indexContent);
  }

  if (invalidExports.length > 0) {
    throw new Error(`Public API exports must come from src/main/ets/interface only. Invalid exports: ${invalidExports.join(', ')}`);
  }
}

checkPublicExportsFromInterfaceOnly();

export default {
  system: harTasks, /* Built-in plugin of Hvigor. It cannot be modified. */
  plugins: []       /* Custom plugin to extend the functionality of Hvigor. */
}