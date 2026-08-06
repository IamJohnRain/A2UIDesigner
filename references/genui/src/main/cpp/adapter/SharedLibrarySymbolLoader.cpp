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

#include "SharedLibrarySymbolLoader.h"

#include <algorithm>
#include <dlfcn.h>

namespace NativeModule {

SharedLibrarySymbolLoaderImpl::SharedLibrarySymbolLoaderImpl() : libraries_(BuildLibraryEntries()) {}

SharedLibrarySymbolLoaderImpl::~SharedLibrarySymbolLoaderImpl()
{
    for (SharedLibraryEntry& libraryEntry : libraries_) {
        SharedLibraryState& libraryState = libraryEntry.state;
        if (libraryState.handle != nullptr) {
            dlclose(libraryState.handle);
            libraryState.handle = nullptr;
        }
        libraryState.openAttempted = false;
        libraryState.resolvedSymbols.clear();
        libraryState.probedSymbols.clear();
    }
}

void* SharedLibrarySymbolLoaderImpl::Resolve(const SharedLibrarySymbolSpec& spec)
{
    if (spec.symbolName == nullptr || !IsSupported(spec.minRomApiVersion)) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    SharedLibraryEntry* libraryEntry = FindLibraryEntry(spec.libraryId);
    if (libraryEntry == nullptr) {
        return nullptr;
    }
    SharedLibraryState& libraryState = libraryEntry->state;
    const std::string symbolName(spec.symbolName);
    if (libraryState.probedSymbols.find(symbolName) != libraryState.probedSymbols.end()) {
        auto cached = libraryState.resolvedSymbols.find(symbolName);
        return cached == libraryState.resolvedSymbols.end() ? nullptr : cached->second;
    }

    void* handle = OpenLibrary(*libraryEntry);
    libraryState.probedSymbols.insert(symbolName);
    if (handle == nullptr) {
        return nullptr;
    }

    void* symbol = dlsym(handle, spec.symbolName);
    libraryState.resolvedSymbols.emplace(symbolName, symbol);
    return symbol;
}

std::vector<SharedLibraryEntry> SharedLibrarySymbolLoaderImpl::BuildLibraryEntries()
{
    return {
        { { SharedLibraryId::ARKUI, "libace_ndk.z.so", MIN_SUPPORTED_ROM_API_VERSION }, {} },
        { { SharedLibraryId::DISPLAY, "libnative_display_manager.so", MIN_SUPPORTED_ROM_API_VERSION }, {} },
    };
}

SharedLibraryEntry* SharedLibrarySymbolLoaderImpl::FindLibraryEntry(SharedLibraryId libraryId)
{
    auto it = std::find_if(libraries_.begin(), libraries_.end(),
        [libraryId](const SharedLibraryEntry& libraryEntry) { return libraryEntry.spec.libraryId == libraryId; });
    return it == libraries_.end() ? nullptr : &(*it);
}

void* SharedLibrarySymbolLoaderImpl::OpenLibrary(SharedLibraryEntry& libraryEntry) const
{
    if (!IsSupported(libraryEntry.spec.minRomApiVersion)) {
        return nullptr;
    }

    SharedLibraryState& libraryState = libraryEntry.state;
    if (!libraryState.openAttempted) {
        libraryState.handle = dlopen(libraryEntry.spec.soName, RTLD_LAZY);
        libraryState.openAttempted = true;
    }
    return libraryState.handle;
}

bool SharedLibrarySymbolLoaderImpl::IsSupported(int32_t minRomApiVersion) const
{
    return minRomApiVersion <= MIN_SUPPORTED_ROM_API_VERSION || currentRomApiVersion_ == ROM_API_VERSION_UNKNOWN ||
           currentRomApiVersion_ >= minRomApiVersion;
}

SharedLibrarySymbolLoader::SharedLibrarySymbolLoader() : impl_(std::make_unique<SharedLibrarySymbolLoaderImpl>()) {}

SharedLibrarySymbolLoader& SharedLibrarySymbolLoader::GetInstance()
{
    // Module unload triggers this static object's destructor, which releases dlopen handles in
    // SharedLibrarySymbolLoaderImpl::~SharedLibrarySymbolLoaderImpl().
    static SharedLibrarySymbolLoader loader;
    return loader;
}

void* SharedLibrarySymbolLoader::ResolveRaw(const SharedLibrarySymbolSpec& spec)
{
    return impl_ == nullptr ? nullptr : impl_->Resolve(spec);
}

} // namespace NativeModule
