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

#ifndef A2UI_SHARED_LIBRARY_SYMBOL_LOADER_H
#define A2UI_SHARED_LIBRARY_SYMBOL_LOADER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace NativeModule {

enum class SharedLibraryId : uint8_t {
    ARKUI = 0,
    DISPLAY = 1,
};

struct SharedLibrarySpec {
    SharedLibraryId libraryId;
    const char* soName;
    int32_t minRomApiVersion;
};

struct SharedLibrarySymbolSpec {
    SharedLibraryId libraryId;
    const char* symbolName;
    int32_t minRomApiVersion;
};

struct SharedLibraryState {
    void* handle = nullptr;
    bool openAttempted = false;
    std::unordered_map<std::string, void*> resolvedSymbols;
    std::unordered_set<std::string> probedSymbols;
};

struct SharedLibraryEntry {
    SharedLibrarySpec spec;
    SharedLibraryState state;
};

class SharedLibrarySymbolLoaderImpl {
public:
    SharedLibrarySymbolLoaderImpl();
    ~SharedLibrarySymbolLoaderImpl();

    void* Resolve(const SharedLibrarySymbolSpec& spec);

private:
    static constexpr int32_t ROM_API_VERSION_UNKNOWN = -1;
    static constexpr int32_t MIN_SUPPORTED_ROM_API_VERSION = 0;

    static std::vector<SharedLibraryEntry> BuildLibraryEntries();
    SharedLibraryEntry* FindLibraryEntry(SharedLibraryId libraryId);
    void* OpenLibrary(SharedLibraryEntry& libraryEntry);
    bool IsSupported(int32_t minRomApiVersion) const;

    int32_t currentRomApiVersion_ = ROM_API_VERSION_UNKNOWN;
    std::vector<SharedLibraryEntry> libraries_;
    std::mutex mutex_;
};

class SharedLibrarySymbolLoader final {
public:
    static SharedLibrarySymbolLoader& GetInstance();

    template<typename Function>
    Function Resolve(const SharedLibrarySymbolSpec& spec)
    {
        return reinterpret_cast<Function>(ResolveRaw(spec));
    }

private:
    SharedLibrarySymbolLoader();
    ~SharedLibrarySymbolLoader() = default;
    SharedLibrarySymbolLoader(const SharedLibrarySymbolLoader&) = delete;
    SharedLibrarySymbolLoader& operator=(const SharedLibrarySymbolLoader&) = delete;

    void* ResolveRaw(const SharedLibrarySymbolSpec& spec);

    std::unique_ptr<SharedLibrarySymbolLoaderImpl> impl_;
};

} // namespace NativeModule

#endif // A2UI_SHARED_LIBRARY_SYMBOL_LOADER_H
