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

#include "utils/SystemProperties.h"

namespace NativeModule {

SystemProperties& SystemProperties::GetInstance()
{
    static SystemProperties instance;
    return instance;
}

void SystemProperties::SetApiVersion(int32_t apiVersion)
{
    if (apiVersion_ != 0) {
        return;
    }
    apiVersion_ = apiVersion;
}

int32_t SystemProperties::GetApiVersion() const
{
    return apiVersion_;
}

} // namespace NativeModule
