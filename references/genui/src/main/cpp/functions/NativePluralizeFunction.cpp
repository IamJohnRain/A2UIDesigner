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

#include "NativePluralizeFunction.h"

#include <cmath>
#include <limits>

#include "utils/LogA2UI.h"

#include "NapiBridge.h"

namespace NativeModule {

PluralLocaleManager& PluralLocaleManager::GetInstance()
{
    static PluralLocaleManager instance;
    return instance;
}

void PluralLocaleManager::RegisterLocaleProvider(napi_env env, napi_value callback)
{
    if (env == nullptr || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "PluralLocaleManager::RegisterLocaleProvider: invalid args");
        return;
    }
    if (providerRef_ != nullptr && env_ != nullptr) {
        napi_delete_reference(env_, providerRef_);
        providerRef_ = nullptr;
    }
    env_ = env;
    napi_status status = napi_create_reference(env, callback, 1, &providerRef_);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "PluralLocaleManager::RegisterLocaleProvider: create ref failed");
        providerRef_ = nullptr;
        return;
    }
    LOG_A2UI(LOG_INFO, "PluralLocaleManager::RegisterLocaleProvider: success");
}

void PluralLocaleManager::SetLocale(const std::string& locale)
{
    cachedLocale_ = locale;
}

std::string PluralLocaleManager::CallProvider()
{
    if (env_ == nullptr || providerRef_ == nullptr) {
        return cachedLocale_;
    }
    auto& napi = NapiBridge::GetInstance().Provider();
    auto now = std::chrono::steady_clock::now();
    if (!cachedLocale_.empty() && lastCallTime_.time_since_epoch().count() > 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallTime_).count();
        if (elapsed < CACHE_TTL_MS) {
            return cachedLocale_;
        }
    }
    napi_value callback = nullptr;
    napi_status status = napi_get_reference_value(env_, providerRef_, &callback);
    if (status != napi_ok || callback == nullptr) {
        return cachedLocale_;
    }
    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, callback, 0, nullptr, &result);
    if (status != napi_ok || result == nullptr) {
        return cachedLocale_;
    }
    size_t strLen = 0;
    if (napi.GetValueStringUtf8(env_, result, nullptr, 0, &strLen) != napi_ok || strLen == 0) {
        return cachedLocale_;
    }
    std::string locale(strLen, '\0');
    if (napi.GetValueStringUtf8(env_, result, &locale[0], strLen + 1, &strLen) != napi_ok) {
        return cachedLocale_;
    }
    cachedLocale_ = locale;
    lastCallTime_ = now;
    return cachedLocale_;
}

std::string PluralLocaleManager::GetLocale()
{
    return CallProvider();
}

static std::string GetLanguagePrefix(const std::string& locale)
{
    auto pos = locale.find('-');
    if (pos != std::string::npos && pos > 0) {
        return locale.substr(0, pos);
    }
    return locale;
}

// ========== CLDR Plural Rule Families ==========
// Reference: https://www.unicode.org/cldr/charts/latest/supplemental/language_plural_rules.html

// Family 1: one/other — most Germanic/Romance/Asian languages
// en, de, fr, es, it, pt, nl, sv, no, da, fi, el, he, hi, id, ja, ko, zh, th, vi, tr, ...
// fr/pt_BR: 0..1 → one (including 0 and decimals 0.x)
// Most others: n==1 → one
static std::string PluralOneOther(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    return "other";
}

// fr, pt-BR: 0 or 1 → one, also decimals 0.x..1.x → one
static std::string PluralFrenchOneOther(double count, long long n, bool isDecimal)
{
    if (n == 0 || n == 1) {
        return "one";
    }
    if (isDecimal) {
        double intPart = 0;
        double frac = std::modf(count, &intPart);
        if (frac > 0 && intPart == 0) {
            return "one";
        }
        if (frac > 0 && intPart == 1) {
            return "one";
        }
    }
    return "other";
}

// Family 2: zero/one/other — latvian, prussian
// lv: n%10==0 || n%100∈{11..19} || v!=0 → zero; n%10==1 && n%100!=11 → one; else other
static std::string PluralZeroOneOther(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "zero";
    }
    if (n % 10 == 0) {
        return "zero";
    }
    long long mod100 = n % 100;
    if (mod100 >= 11 && mod100 <= 19) {
        return "zero";
    }
    if (n % 10 == 1 && mod100 != 11) {
        return "one";
    }
    return "other";
}

// Family 3: one/two/other — Welsh, Irish, Scottish Gaelic
// cy: n==1→one, n==2→two, else other
// ga: n==1→one, n==2→two, else other
static std::string PluralOneTwoOther(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    if (n == 2) {
        return "two";
    }
    return "other";
}

// Family 4: one/few/other — Czech, Slovak
// cs/sk: n==1→one; n∈{2,3,4}→few; else other
static std::string PluralOneFewOther(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    if (n >= 2 && n <= 4) {
        return "few";
    }
    return "other";
}

// Family 5: one/few/many/other — Slavic languages (Russian, Polish, Ukrainian, Bosnian, Croatian, Serbian)
// ru, uk: n%10==1 && n%100!=11 → one; n%10∈{2..4} && n%100∉{12..14} → few;
//         n%10==0 || n%10∈{5..9} || n%100∈{11..14} → many; else other
// pl: n==1→one; n%10∈{2..4} && n%100∉{12..14} → few; (n!=1 && (n%10==0..1 || n%10∈{5..9} || n%100∈{12..14})) → many
// bs/hr/sr: same as ru
static std::string PluralOneFewManyOther(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    long long mod10 = n % 10;
    long long mod100 = n % 100;
    if (mod10 == 1 && mod100 != 11) {
        return "one";
    }
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        return "few";
    }
    if (mod10 == 0 || (mod10 >= 5 && mod10 <= 9) || (mod100 >= 11 && mod100 <= 14)) {
        return "many";
    }
    return "other";
}

// Polish variant: n==1→one; n%10∈{2..4}&&n%100∉{12..14}→few; else→many (other only for decimals)
static std::string PluralPolish(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    long long mod10 = n % 10;
    long long mod100 = n % 100;
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        return "few";
    }
    return "many";
}

// Lithuanian: n%10==1 && n%100!=11 → one; n%10∈{2..9} && n%100∉{12..19} → few; else other
static std::string PluralLithuanian(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    long long mod10 = n % 10;
    long long mod100 = n % 100;
    if (mod10 == 1 && mod100 != 11) {
        return "one";
    }
    if (mod10 >= 2 && mod10 <= 9 && (mod100 < 12 || mod100 > 19)) {
        return "few";
    }
    return "other";
}

// Latvian variant: n==0→zero; n%10==1 && n%100!=11 → one; else other
static std::string PluralLatvian(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "zero";
    }
    if (n % 10 == 0) {
        return "zero";
    }
    long long mod100 = n % 100;
    if (mod100 >= 11 && mod100 <= 19) {
        return "zero";
    }
    if (n % 10 == 1 && mod100 != 11) {
        return "one";
    }
    return "other";
}

// Breton: n==1→one; n==2→two; n==3→few; n==6→many; else other
static std::string PluralBreton(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    if (n == 2) {
        return "two";
    }
    if (n == 3) {
        return "few";
    }
    if (n == 6) {
        return "many";
    }
    return "other";
}

// Macedonian: n%10==1 && n!=11 → one; else other
static std::string PluralMacedonian(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n % 10 == 1 && n != 11) {
        return "one";
    }
    return "other";
}

// Icelandic: n%10==1 && n%100!=11 → one; else other
static std::string PluralIcelandic(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n % 10 == 1 && n % 100 != 11) {
        return "one";
    }
    return "other";
}

// Arabic: n==0→zero; n==1→one; n==2→two; n%100∈{3..10}→few; n%100∈{11..99}→many; else other
static std::string PluralArabic(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 0) {
        return "zero";
    }
    if (n == 1) {
        return "one";
    }
    if (n == 2) {
        return "two";
    }
    long long mod100 = n % 100;
    if (mod100 >= 3 && mod100 <= 10) {
        return "few";
    }
    if (mod100 >= 11 && mod100 <= 99) {
        return "many";
    }
    return "other";
}

// Hebrew: n==1→one; n==2→two; n∈{0,10..∞,10n+0..10n+0}→many; else other
// Simplified: n==1→one; n==2→two; n==0||(n>=10&&n%10==0)→many; else other
static std::string PluralHebrew(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    if (n == 2) {
        return "two";
    }
    if (n == 0) {
        return "many";
    }
    if (n >= 10 && n <= 20) {
        return "many";
    }
    if (n > 20 && n % 10 == 0) {
        return "many";
    }
    return "other";
}

// Welsh extended: n==0→zero; n==1→one; n==2→two; n==3→few; n==6→many; else other
static std::string PluralWelsh(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 0) {
        return "zero";
    }
    if (n == 1) {
        return "one";
    }
    if (n == 2) {
        return "two";
    }
    if (n == 3) {
        return "few";
    }
    if (n == 6) {
        return "many";
    }
    return "other";
}

// Irish: n==1→one; n==2→two; n∈{3..6}→few; n∈{7..10}→many; else other
static std::string PluralIrish(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    if (n == 2) {
        return "two";
    }
    if (n >= 3 && n <= 6) {
        return "few";
    }
    if (n >= 7 && n <= 10) {
        return "many";
    }
    return "other";
}

// Slovenian: n%100==1→one; n%100==2→two; n%100∈{3..4}→few; else other
static std::string PluralSlovenian(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    long long mod100 = n % 100;
    if (mod100 == 1) {
        return "one";
    }
    if (mod100 == 2) {
        return "two";
    }
    if (mod100 == 3 || mod100 == 4) {
        return "few";
    }
    return "other";
}

// Maltese: n==1→one; n==0||(n%100∈{2..10})→few; n%100∈{11..19}→many; else other
static std::string PluralMaltese(long long n, bool isDecimal)
{
    if (isDecimal) {
        return "other";
    }
    if (n == 1) {
        return "one";
    }
    if (n == 0 || (n % 100 >= 2 && n % 100 <= 10)) {
        return "few";
    }
    if (n % 100 >= 11 && n % 100 <= 19) {
        return "many";
    }
    return "other";
}

// Language family lookup
static std::string GetPluralCategory(double count)
{
    const std::string& locale = PluralLocaleManager::GetInstance().GetLocale();
    std::string lang = GetLanguagePrefix(locale);

    double intPart = 0;
    bool isDecimal = std::modf(count, &intPart) != 0.0;
    long long n = static_cast<long long>(count);

    std::string category = "other";

    // Arabic
    if (lang == "ar") {
        category = PluralArabic(n, isDecimal);
    }
    // Belarusian, Russian, Ukrainian, Bosnian, Croatian, Serbian
    else if (lang == "ru" || lang == "uk" || lang == "be" || lang == "bs" || lang == "hr" || lang == "sr") {
        category = PluralOneFewManyOther(n, isDecimal);
    }
    // Polish
    else if (lang == "pl") {
        category = PluralPolish(n, isDecimal);
    }
    // Czech, Slovak
    else if (lang == "cs" || lang == "sk") {
        category = PluralOneFewOther(n, isDecimal);
    }
    // Lithuanian
    else if (lang == "lt") {
        category = PluralLithuanian(n, isDecimal);
    } else if (lang == "lv") {
        category = PluralLatvian(n, isDecimal);
    } else if (lang == "fr" || lang == "pt") {
        category = PluralFrenchOneOther(count, n, isDecimal);
    } else if (lang == "cy") {
        category = PluralWelsh(n, isDecimal);
    } else if (lang == "ga") {
        category = PluralIrish(n, isDecimal);
    } else if (lang == "br") {
        category = PluralBreton(n, isDecimal);
    } else if (lang == "he") {
        category = PluralHebrew(n, isDecimal);
    } else if (lang == "is") {
        category = PluralIcelandic(n, isDecimal);
    } else if (lang == "mk") {
        category = PluralMacedonian(n, isDecimal);
    } else if (lang == "sl") {
        category = PluralSlovenian(n, isDecimal);
    } else if (lang == "mt") {
        category = PluralMaltese(n, isDecimal);
    } else {
        category = PluralOneOther(n, isDecimal);
    }

    return category;
}

static std::string GetOptionalString(const JsonValue& argsObject, const char* key)
{
    JsonValue value = argsObject.GetItem(key);
    if (value.IsString()) {
        return value.GetStringValue("");
    }
    return "";
}

static bool ValidateOptionalString(const JsonValue& argsObject, const char* key)
{
    JsonValue value = argsObject.GetItem(key);
    return !value.IsValid() || value.IsString();
}

std::string NativePluralizeFunction::GetName() const
{
    return "pluralize";
}

FunctionResult NativePluralizeFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(std::string(""));
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsNumber()) {
        return FunctionResult(std::string(""));
    }
    if (!ValidateOptionalString(resolvedArgs, "zero") || !ValidateOptionalString(resolvedArgs, "one") ||
        !ValidateOptionalString(resolvedArgs, "two") || !ValidateOptionalString(resolvedArgs, "few") ||
        !ValidateOptionalString(resolvedArgs, "many") || !ValidateOptionalString(resolvedArgs, "other")) {
        return FunctionResult(std::string(""));
    }

    double count = valueArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
    std::string category = GetPluralCategory(count);

    std::string result = GetOptionalString(resolvedArgs, category.c_str());
    if (!result.empty()) {
        return FunctionResult(std::move(result));
    }

    result = GetOptionalString(resolvedArgs, "other");
    return FunctionResult(std::move(result));
}

} // namespace NativeModule
