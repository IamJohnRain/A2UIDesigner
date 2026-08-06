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

#ifndef A2UI_JSON_ADAPTER_H
#define A2UI_JSON_ADAPTER_H

#include <cstdint>
#include <memory>
#include <string>

struct cJSON;

namespace NativeModule {

enum class JsonValueType { INVALID = 0, NULL_VALUE, BOOL, NUMBER, STRING, ARRAY, OBJECT, RAW, UNKNOWN };

class JsonValue {
public:
    JsonValue() = default;

    static std::shared_ptr<JsonValue> Create()
    {
        return std::make_shared<JsonValue>();
    }

    bool IsValid() const;
    bool IsNull() const;
    bool IsBool() const;
    bool IsTrue() const;
    bool IsFalse() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsObject() const;
    bool IsArray() const;

    JsonValueType GetType() const;
    const char* GetTypeName() const;

    bool Has(const char* key) const;
    bool Has(const std::string& key) const;
    bool HasObjectItem(const char* key) const;

    JsonValue GetItem(const char* key) const;
    JsonValue GetItem(const std::string& key) const;
    JsonValue GetObjectItem(const char* key) const;
    JsonValue GetObject(const char* key) const;
    JsonValue GetChild() const;
    JsonValue GetNext() const;
    JsonValue GetPrev() const;
    JsonValue GetArrayItem(int index) const;
    int GetArraySize() const;

    bool GetBool(const char* key, bool fallback = false) const;
    std::string GetString(const char* key, const std::string& fallback = "") const;
    double GetNumber(const char* key, double fallback = 0.0) const;
    uint32_t GetUint32(const char* key, uint32_t fallback = 0) const;
    int32_t GetInt32(const char* key, int32_t fallback = 0) const;

    bool GetBoolValue(bool fallback = false) const;
    std::string GetStringValue(const std::string& fallback = "") const;
    double GetNumberValue(double fallback = 0.0) const;
    uint32_t GetUint32Value(uint32_t fallback = 0) const;
    int32_t GetInt32Value(int32_t fallback = 0) const;
    std::string ToString(const std::string& fallback = "") const;
    double ToNumber(double fallback = 0.0) const;
    bool ToBool(bool fallback = false) const;
    std::string ToJsonLiteral() const;

    std::string GetKey(const std::string& fallback = "") const;

    bool Put(const char* key, const JsonValue& value);
    bool Set(const char* key, const JsonValue& value);
    bool Remove(const char* key);
    bool PutNull(const char* key);
    bool PutBool(const char* key, bool value);
    bool PutNumber(const char* key, double value);
    bool PutString(const char* key, const std::string& value);
    JsonValue PutObject(const char* key);
    JsonValue PutArray(const char* key);

    bool Replace(const char* key, const JsonValue& value);
    bool ReplaceNull(const char* key);
    bool ReplaceBool(const char* key, bool value);
    bool ReplaceNumber(const char* key, double value);
    bool ReplaceString(const char* key, const std::string& value);
    JsonValue ReplaceObject(const char* key);
    JsonValue ReplaceArray(const char* key);

    bool Append(const JsonValue& value);

private:
    friend class JsonAdapter;

    JsonValue(std::shared_ptr<void> rootHolder, ::cJSON* value);

    std::shared_ptr<void> rootHolder_;
    ::cJSON* value_ = nullptr;
};

class JsonAdapter {
public:
    class ConstructionToken {
    private:
        ConstructionToken() = default;
        friend class JsonAdapter;
    };

    static std::unique_ptr<JsonAdapter> Parse(const std::string& jsonString);
    static std::unique_ptr<JsonAdapter> Clone(const JsonValue& value);
    static std::unique_ptr<JsonAdapter> Adopt(cJSON* root);
    static std::unique_ptr<JsonAdapter> CreateNull();
    static std::unique_ptr<JsonAdapter> CreateBool(bool value);
    static std::unique_ptr<JsonAdapter> CreateNumber(double value);
    static std::unique_ptr<JsonAdapter> CreateString(const std::string& value);
    static std::unique_ptr<JsonAdapter> CreateObject();
    static std::unique_ptr<JsonAdapter> CreateArray();

    JsonValue GetRoot() const;

    JsonAdapter(ConstructionToken, std::shared_ptr<void> rootHolder, ::cJSON* root);

private:
    std::shared_ptr<void> rootHolder_;
    ::cJSON* root_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_JSON_ADAPTER_H
