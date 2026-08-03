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

#include "JsonAdapter.h"

#include <cJSON.h>
#include <cctype>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

std::string TrimString(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string NormalizeToken(const std::string& value)
{
    std::string normalized = TrimString(value);
    for (char& ch : normalized) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return normalized;
}

bool IsFiniteNumber(double value)
{
    return std::isfinite(value);
}

cJSON* GetObjectItemInternal(cJSON* object, const char* key)
{
    if (object == nullptr || key == nullptr || !cJSON_IsObject(object)) {
        return nullptr;
    }
    return cJSON_GetObjectItemCaseSensitive(object, key);
}

bool PutObjectItemInternal(cJSON* object, const char* key, cJSON* item)
{
    if (!cJSON_IsObject(object) || key == nullptr || item == nullptr) {
        if (item != nullptr) {
            cJSON_Delete(item);
        }
        return false;
    }
    if (cJSON_HasObjectItem(object, key)) {
        cJSON_Delete(item);
        return false;
    }
    if (!cJSON_AddItemToObject(object, key, item)) {
        cJSON_Delete(item);
        return false;
    }
    return true;
}

bool ReplaceObjectItemInternal(cJSON* object, const char* key, cJSON* item)
{
    if (!cJSON_IsObject(object) || key == nullptr || item == nullptr) {
        if (item != nullptr) {
            cJSON_Delete(item);
        }
        return false;
    }
    if (!cJSON_HasObjectItem(object, key)) {
        cJSON_Delete(item);
        return false;
    }
    if (!cJSON_ReplaceItemInObjectCaseSensitive(object, key, item)) {
        cJSON_Delete(item);
        return false;
    }
    return true;
}

bool SetObjectItemInternal(cJSON* object, const char* key, cJSON* item)
{
    if (!cJSON_IsObject(object) || key == nullptr || item == nullptr) {
        if (item != nullptr) {
            cJSON_Delete(item);
        }
        return false;
    }
    if (cJSON_HasObjectItem(object, key)) {
        if (!cJSON_ReplaceItemInObjectCaseSensitive(object, key, item)) {
            cJSON_Delete(item);
            return false;
        }
        return true;
    }
    if (!cJSON_AddItemToObject(object, key, item)) {
        cJSON_Delete(item);
        return false;
    }
    return true;
}

bool RemoveObjectItemInternal(cJSON* object, const char* key)
{
    if (!cJSON_IsObject(object) || key == nullptr || !cJSON_HasObjectItem(object, key)) {
        return false;
    }
    cJSON* detached = cJSON_DetachItemFromObjectCaseSensitive(object, key);
    if (detached == nullptr) {
        return false;
    }
    cJSON_Delete(detached);
    return true;
}

bool AppendArrayItemInternal(cJSON* array, cJSON* item)
{
    if (!cJSON_IsArray(array) || item == nullptr) {
        if (item != nullptr) {
            cJSON_Delete(item);
        }
        return false;
    }
    if (!cJSON_AddItemToArray(array, item)) {
        cJSON_Delete(item);
        return false;
    }
    return true;
}

} // namespace

JsonValue::JsonValue(std::shared_ptr<void> rootHolder, cJSON* value) : rootHolder_(std::move(rootHolder)), value_(value)
{}

bool JsonValue::IsValid() const
{
    return value_ != nullptr;
}

bool JsonValue::IsNull() const
{
    return cJSON_IsNull(value_);
}

bool JsonValue::IsBool() const
{
    return cJSON_IsBool(value_);
}

bool JsonValue::IsTrue() const
{
    return cJSON_IsTrue(value_);
}

bool JsonValue::IsFalse() const
{
    return cJSON_IsFalse(value_);
}

bool JsonValue::IsNumber() const
{
    return cJSON_IsNumber(value_);
}

bool JsonValue::IsString() const
{
    return cJSON_IsString(value_);
}

bool JsonValue::IsObject() const
{
    return cJSON_IsObject(value_);
}

bool JsonValue::IsArray() const
{
    return cJSON_IsArray(value_);
}

JsonValueType JsonValue::GetType() const
{
    if (!IsValid()) {
        return JsonValueType::INVALID;
    }
    if (IsNull()) {
        return JsonValueType::NULL_VALUE;
    }
    if (IsBool()) {
        return JsonValueType::BOOL;
    }
    if (IsNumber()) {
        return JsonValueType::NUMBER;
    }
    if (IsString()) {
        return JsonValueType::STRING;
    }
    if (IsArray()) {
        return JsonValueType::ARRAY;
    }
    if (IsObject()) {
        return JsonValueType::OBJECT;
    }
    if (cJSON_IsRaw(value_)) {
        return JsonValueType::RAW;
    }
    return JsonValueType::UNKNOWN;
}

const char* JsonValue::GetTypeName() const
{
    switch (GetType()) {
        case JsonValueType::INVALID:
            return "invalid";
        case JsonValueType::NULL_VALUE:
            return "null";
        case JsonValueType::BOOL:
            return "bool";
        case JsonValueType::NUMBER:
            return "number";
        case JsonValueType::STRING:
            return "string";
        case JsonValueType::ARRAY:
            return "array";
        case JsonValueType::OBJECT:
            return "object";
        case JsonValueType::RAW:
            return "raw";
        case JsonValueType::UNKNOWN:
        default:
            return "unknown";
    }
}

bool JsonValue::Has(const char* key) const
{
    return HasObjectItem(key);
}

bool JsonValue::Has(const std::string& key) const
{
    return Has(key.c_str());
}

JsonValue JsonValue::GetItem(const char* key) const
{
    return GetObjectItem(key);
}

JsonValue JsonValue::GetItem(const std::string& key) const
{
    return GetItem(key.c_str());
}

bool JsonValue::HasObjectItem(const char* key) const
{
    return GetObjectItemInternal(value_, key) != nullptr;
}

JsonValue JsonValue::GetObjectItem(const char* key) const
{
    return JsonValue(rootHolder_, GetObjectItemInternal(value_, key));
}

JsonValue JsonValue::GetObject(const char* key) const
{
    return GetObjectItem(key);
}

JsonValue JsonValue::GetChild() const
{
    if (!IsValid()) {
        return {};
    }
    return JsonValue(rootHolder_, value_->child);
}

JsonValue JsonValue::GetNext() const
{
    if (!IsValid()) {
        return {};
    }
    return JsonValue(rootHolder_, value_->next);
}

JsonValue JsonValue::GetPrev() const
{
    if (!IsValid()) {
        return {};
    }
    return JsonValue(rootHolder_, value_->prev);
}

JsonValue JsonValue::GetArrayItem(int index) const
{
    if (!cJSON_IsArray(value_)) {
        return {};
    }
    return JsonValue(rootHolder_, cJSON_GetArrayItem(value_, index));
}

int JsonValue::GetArraySize() const
{
    if (!cJSON_IsArray(value_)) {
        return 0;
    }
    return cJSON_GetArraySize(value_);
}

bool JsonValue::GetBool(const char* key, bool fallback) const
{
    return GetItem(key).GetBoolValue(fallback);
}

std::string JsonValue::GetString(const char* key, const std::string& fallback) const
{
    return GetItem(key).GetStringValue(fallback);
}

double JsonValue::GetNumber(const char* key, double fallback) const
{
    return GetItem(key).GetNumberValue(fallback);
}

uint32_t JsonValue::GetUint32(const char* key, uint32_t fallback) const
{
    double value = GetItem(key).GetNumberValue(static_cast<double>(fallback));
    if (value < 0.0 || value > static_cast<double>(std::numeric_limits<uint32_t>::max())) {
        return fallback;
    }
    return static_cast<uint32_t>(value);
}

int32_t JsonValue::GetInt32(const char* key, int32_t fallback) const
{
    double value = GetItem(key).GetNumberValue(static_cast<double>(fallback));
    if (value < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
        value > static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return fallback;
    }
    return static_cast<int32_t>(value);
}

bool JsonValue::GetBoolValue(bool fallback) const
{
    if (!IsBool()) {
        return fallback;
    }
    return IsTrue();
}

std::string JsonValue::GetStringValue(const std::string& fallback) const
{
    if (!IsString()) {
        return fallback;
    }

    const char* stringValue = cJSON_GetStringValue(value_);
    if (stringValue == nullptr) {
        return fallback;
    }
    return std::string(stringValue);
}

double JsonValue::GetNumberValue(double fallback) const
{
    if (!IsNumber()) {
        return fallback;
    }
    double value = cJSON_GetNumberValue(value_);
    return IsFiniteNumber(value) ? value : fallback;
}

uint32_t JsonValue::GetUint32Value(uint32_t fallback) const
{
    double value = GetNumberValue(static_cast<double>(fallback));
    if (value < 0.0 || value > static_cast<double>(std::numeric_limits<uint32_t>::max())) {
        return fallback;
    }
    return static_cast<uint32_t>(value);
}

int32_t JsonValue::GetInt32Value(int32_t fallback) const
{
    double value = GetNumberValue(static_cast<double>(fallback));
    if (value < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
        value > static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return fallback;
    }
    return static_cast<int32_t>(value);
}

std::string JsonValue::GetKey(const std::string& fallback) const
{
    if (!IsValid() || value_->string == nullptr) {
        return fallback;
    }
    return std::string(value_->string);
}

bool JsonValue::Put(const char* key, const JsonValue& value)
{
    cJSON* duplicatedValue = value.IsValid() ? cJSON_Duplicate(value.value_, 1) : nullptr;
    return PutObjectItemInternal(value_, key, duplicatedValue);
}

bool JsonValue::Set(const char* key, const JsonValue& value)
{
    cJSON* duplicatedValue = value.IsValid() ? cJSON_Duplicate(value.value_, 1) : nullptr;
    return SetObjectItemInternal(value_, key, duplicatedValue);
}

bool JsonValue::Remove(const char* key)
{
    return RemoveObjectItemInternal(value_, key);
}

bool JsonValue::PutNull(const char* key)
{
    return PutObjectItemInternal(value_, key, cJSON_CreateNull());
}

bool JsonValue::PutBool(const char* key, bool value)
{
    return PutObjectItemInternal(value_, key, cJSON_CreateBool(value));
}

bool JsonValue::PutNumber(const char* key, double value)
{
    return PutObjectItemInternal(value_, key, cJSON_CreateNumber(value));
}

bool JsonValue::PutString(const char* key, const std::string& value)
{
    return PutObjectItemInternal(value_, key, cJSON_CreateString(value.c_str()));
}

JsonValue JsonValue::PutObject(const char* key)
{
    if (!PutObjectItemInternal(value_, key, cJSON_CreateObject())) {
        return {};
    }
    return GetObjectItem(key);
}

JsonValue JsonValue::PutArray(const char* key)
{
    if (!PutObjectItemInternal(value_, key, cJSON_CreateArray())) {
        return {};
    }
    return GetObjectItem(key);
}

bool JsonValue::Replace(const char* key, const JsonValue& value)
{
    cJSON* duplicatedValue = value.IsValid() ? cJSON_Duplicate(value.value_, 1) : nullptr;
    return ReplaceObjectItemInternal(value_, key, duplicatedValue);
}

bool JsonValue::ReplaceNull(const char* key)
{
    return ReplaceObjectItemInternal(value_, key, cJSON_CreateNull());
}

bool JsonValue::ReplaceBool(const char* key, bool value)
{
    return ReplaceObjectItemInternal(value_, key, cJSON_CreateBool(value));
}

bool JsonValue::ReplaceNumber(const char* key, double value)
{
    return ReplaceObjectItemInternal(value_, key, cJSON_CreateNumber(value));
}

bool JsonValue::ReplaceString(const char* key, const std::string& value)
{
    return ReplaceObjectItemInternal(value_, key, cJSON_CreateString(value.c_str()));
}

JsonValue JsonValue::ReplaceObject(const char* key)
{
    if (!ReplaceObjectItemInternal(value_, key, cJSON_CreateObject())) {
        return {};
    }
    return GetObjectItem(key);
}

JsonValue JsonValue::ReplaceArray(const char* key)
{
    if (!ReplaceObjectItemInternal(value_, key, cJSON_CreateArray())) {
        return {};
    }
    return GetObjectItem(key);
}

bool JsonValue::Append(const JsonValue& value)
{
    cJSON* duplicatedValue = value.IsValid() ? cJSON_Duplicate(value.value_, 1) : nullptr;
    return AppendArrayItemInternal(value_, duplicatedValue);
}

std::unique_ptr<JsonAdapter> JsonAdapter::Parse(const std::string& jsonString)
{
    LOG_A2UI(LOG_INFO, "Parse start, jsonLength=%{public}zu", jsonString.size());

    cJSON* root = cJSON_Parse(jsonString.c_str());
    if (root == nullptr) {
        const char* errorPtr = cJSON_GetErrorPtr();
        long errorOffset = -1;
        if (errorPtr != nullptr) {
            errorOffset = static_cast<long>(errorPtr - jsonString.c_str());
        }
        LOG_A2UI(
            LOG_ERROR, "Parse failed, jsonLength=%{public}zu, errorOffset=%{public}ld", jsonString.size(), errorOffset);
        return nullptr;
    }

    LOG_A2UI(LOG_INFO, "Parse success, jsonLength=%{public}zu", jsonString.size());

    std::shared_ptr<void> rootHolder(root, [](void* node) { cJSON_Delete(static_cast<cJSON*>(node)); });
    return std::unique_ptr<JsonAdapter>(new JsonAdapter(std::move(rootHolder), root));
}

std::string JsonValue::ToString(const std::string& fallback) const
{
    if (!IsValid()) {
        return fallback;
    }
    if (IsNull()) {
        return "null";
    }
    if (IsString()) {
        return GetStringValue(fallback);
    }
    if (IsNumber()) {
        std::ostringstream builder;
        builder << GetNumberValue(0.0);
        return builder.str();
    }
    if (IsBool()) {
        return GetBoolValue(false) ? "true" : "false";
    }
    if (IsArray() || IsObject()) {
        return ToJsonLiteral();
    }
    return fallback;
}

double JsonValue::ToNumber(double fallback) const
{
    if (!IsValid() || IsNull()) {
        return fallback;
    }
    if (IsNumber()) {
        return GetNumberValue(fallback);
    }
    if (IsBool()) {
        return GetBoolValue(false) ? 1.0 : 0.0;
    }
    if (IsString()) {
        std::string trimmed = TrimString(GetStringValue(""));
        if (trimmed.empty()) {
            return fallback;
        }
        try {
            size_t parsed = 0;
            double value = std::stod(trimmed, &parsed);
            if (parsed == trimmed.size() && IsFiniteNumber(value)) {
                return value;
            }
        } catch (const std::invalid_argument&) {
            return fallback;
        } catch (const std::out_of_range&) {
            return fallback;
        }
    }
    return fallback;
}

bool JsonValue::ToBool(bool fallback) const
{
    if (!IsValid() || IsNull()) {
        return fallback;
    }
    if (IsBool()) {
        return GetBoolValue(fallback);
    }
    if (IsNumber()) {
        return GetNumberValue(0.0) != 0.0;
    }
    if (IsString()) {
        std::string normalized = NormalizeToken(GetStringValue(""));
        if (normalized.empty() || normalized == "0" || normalized == "false") {
            return false;
        }
        return true;
    }
    if (IsArray()) {
        return GetArraySize() > 0;
    }
    if (IsObject()) {
        return GetChild().IsValid();
    }
    return fallback;
}

std::string JsonValue::ToJsonLiteral() const
{
    if (!IsValid()) {
        return "null";
    }

    std::unique_ptr<char, decltype(&cJSON_free)> printedValue(cJSON_PrintUnformatted(value_), &cJSON_free);
    if (!printedValue) {
        return "null";
    }
    return std::string(printedValue.get());
}

std::unique_ptr<JsonAdapter> JsonAdapter::Clone(const JsonValue& value)
{
    if (!value.IsValid()) {
        return nullptr;
    }

    cJSON* root = cJSON_Duplicate(value.value_, 1);
    if (root == nullptr) {
        LOG_A2UI(LOG_ERROR, "JsonAdapter::Clone: duplicate failed");
        return nullptr;
    }

    std::shared_ptr<void> rootHolder(root, [](void* node) { cJSON_Delete(static_cast<cJSON*>(node)); });
    return std::unique_ptr<JsonAdapter>(new JsonAdapter(std::move(rootHolder), root));
}

std::unique_ptr<JsonAdapter> JsonAdapter::Adopt(cJSON* root)
{
    if (root == nullptr) {
        return nullptr;
    }
    std::shared_ptr<void> rootHolder(root, [](void* node) { cJSON_Delete(static_cast<cJSON*>(node)); });
    return std::unique_ptr<JsonAdapter>(new JsonAdapter(std::move(rootHolder), root));
}

std::unique_ptr<JsonAdapter> JsonAdapter::CreateNull()
{
    return Adopt(cJSON_CreateNull());
}

std::unique_ptr<JsonAdapter> JsonAdapter::CreateBool(bool value)
{
    return Adopt(cJSON_CreateBool(value));
}

std::unique_ptr<JsonAdapter> JsonAdapter::CreateNumber(double value)
{
    return Adopt(cJSON_CreateNumber(value));
}

std::unique_ptr<JsonAdapter> JsonAdapter::CreateString(const std::string& value)
{
    return Adopt(cJSON_CreateString(value.c_str()));
}

std::unique_ptr<JsonAdapter> JsonAdapter::CreateObject()
{
    return Adopt(cJSON_CreateObject());
}

std::unique_ptr<JsonAdapter> JsonAdapter::CreateArray()
{
    return Adopt(cJSON_CreateArray());
}

JsonAdapter::JsonAdapter(std::shared_ptr<void> rootHolder, cJSON* root)
    : rootHolder_(std::move(rootHolder)), root_(root)
{}

JsonValue JsonAdapter::GetRoot() const
{
    return JsonValue(rootHolder_, root_);
}

} // namespace NativeModule
