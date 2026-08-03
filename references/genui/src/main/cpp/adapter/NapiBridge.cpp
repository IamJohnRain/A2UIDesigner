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

#include "NapiBridge.h"
#ifndef TDD_BUILD
#include "NapiProvider.h"
#endif

namespace NativeModule {

NapiBridge::NapiBridge()
{
#ifndef TDD_BUILD
    provider_ = std::make_unique<NapiProvider>();
#endif
}

NapiBridge& NapiBridge::GetInstance()
{
    static NapiBridge instance;
    return instance;
}

void NapiBridge::SetProvider(std::unique_ptr<INapiProvider> provider)
{
    GetInstance().provider_ = std::move(provider);
}

INapiProvider& NapiBridge::Provider()
{
    return *provider_;
}

} // namespace NativeModule
