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

import { CatalogItem } from '../../base/CatalogItem';
import { registerBuiltInCustomComponents } from './A2UIBasicCustomComponents';
import { A2UIButton } from './A2UIButton';
import { A2UICard } from './A2UICard';
import { A2UICheckbox } from './A2UICheckbox';
import { A2UIColumn } from './A2UIColumn';
import { A2UIModal } from './A2UIModal';
import { A2UIRow } from './A2UIRow';
import { A2UIImage } from './A2UIImage';
import { A2UIList } from './A2UIList';
import { A2UISlider } from './A2UISlider';
import { A2UIText } from './A2UIText';
import { A2UITextField } from './A2UITextField';
import { asCatalogItem as iconCatalogItem } from './CustomIcon';
import { asCatalogItem as videoCatalogItem } from './CustomVideo';
import { asCatalogItem as audioPlayerCatalogItem } from './CustomAudioPlayer';
import { asCatalogItem as tabsCatalogItem } from './CustomTabs';
import { asCatalogItem as dividerCatalogItem } from './CustomDivider';
import { asCatalogItem as choicePickerCatalogItem } from './CustomChoicePicker';
import { asCatalogItem as dateTimeInputCatalogItem } from './CustomDateTimeInput';

export function allA2UIBasicComponents(): CatalogItem[] {
  const components: CatalogItem[] = [
    A2UIButton.asCatalogItem(),
    A2UIText.asCatalogItem(),
    A2UITextField.asCatalogItem(),
    A2UIColumn.asCatalogItem(),
    A2UIModal.asCatalogItem(),
    A2UIRow.asCatalogItem(),
    A2UICard.asCatalogItem(),
    A2UICheckbox.asCatalogItem(),
    A2UIImage.asCatalogItem(),
    A2UIList.asCatalogItem(),
    A2UISlider.asCatalogItem(),
    iconCatalogItem(),
    videoCatalogItem(),
    audioPlayerCatalogItem(),
    tabsCatalogItem(),
    dividerCatalogItem(),
    choicePickerCatalogItem(),
    dateTimeInputCatalogItem(),
  ];
  registerBuiltInCustomComponents();
  return components;
}
