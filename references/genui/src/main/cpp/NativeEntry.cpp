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

#include "NativeEntry.h"

#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <hitrace/trace.h>
#include <memory>
#include <string>
#include <vector>

#include "catalog/CatalogConstants.h"
#include "components/Component.h"
#include "components/custom/CustomComponent.h"
#include "data/BindingEngine.h"
#include "data/DynamicValueResolver.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/CrossLanguageAttributeBridge.h"
#include "functions/FunctionBridge.h"
#include "functions/NativePluralizeFunction.h"
#include "functions/RuntimeErrorDispatchBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "theme/ThemeBase.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "ArkUIOHApiAdapter.h"
#include "NapiResourceManager.h"
#include "ProtocolVersionPolicy.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SchemaErrorCodes.h"
#include "SurfaceContext.h"
#include "SurfaceErrorCodes.h"
#include "SurfaceManager.h"
#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/ExpressionEngine.h"
#endif
#include "catalog/Catalog.h"
#include "components/actions/BuiltInActions.h"
#include "components/actions/NativeActionRegistry.h"
#include "utils/DisplayDensityUtils.h"
#include "utils/JsonAdapter.h"
#include "utils/NapiUtils.h"
#include "utils/SystemProperties.h"
#include "utils/ThemeColorUtils.h"

#include "NapiBridge.h"

namespace NativeModule {

namespace {
class HiTraceScoped {
public:
    explicit HiTraceScoped(const char* name)
    {
        OH_HiTrace_StartTrace(name);
    }
    ~HiTraceScoped()
    {
        OH_HiTrace_FinishTrace();
    }
    HiTraceScoped(const HiTraceScoped&) = delete;
    HiTraceScoped& operator=(const HiTraceScoped&) = delete;
};

// Helper function to get NapiResourceManager from RenderManager
NapiResourceManager* GetNapiResourceManager()
{
    return RenderManager::GetInstance().GetNapiResourceManager();
}

napi_value CreateSurfaceResult(napi_env env, bool success, int32_t code, const std::string& message)
{
    napi_value result = nullptr;
    NapiBridge::GetInstance().Provider().CreateObject(env, &result);

    napi_value successValue = nullptr;
    NapiBridge::GetInstance().Provider().GetBoolean(env, success, &successValue);
    NapiBridge::GetInstance().Provider().SetNamedProperty(env, result, "success", successValue);

    napi_value codeValue = nullptr;
    NapiBridge::GetInstance().Provider().CreateInt32(env, code, &codeValue);
    NapiBridge::GetInstance().Provider().SetNamedProperty(env, result, "code", codeValue);

    if (!message.empty()) {
        napi_value messageValue = nullptr;
        NapiBridge::GetInstance().Provider().CreateStringUtf8(env, message.c_str(), NAPI_AUTO_LENGTH, &messageValue);
        NapiBridge::GetInstance().Provider().SetNamedProperty(env, result, "message", messageValue);
    }

    return result;
}

napi_value CreateSurfaceIdArray(napi_env env, const std::vector<std::string>& surfaceIds)
{
    napi_value result = nullptr;
    NapiBridge::GetInstance().Provider().CreateArrayWithLength(env, surfaceIds.size(), &result);
    for (uint32_t i = 0; i < surfaceIds.size(); ++i) {
        napi_value surfaceIdValue = nullptr;
        NapiBridge::GetInstance().Provider().CreateStringUtf8(
            env, surfaceIds[i].c_str(), NAPI_AUTO_LENGTH, &surfaceIdValue);
        NapiBridge::GetInstance().Provider().SetElement(env, result, i, surfaceIdValue);
    }
    return result;
}

#ifdef ENABLE_EXPRESSION_ENGINE
const char* EvalValueTypeToString(const EvalResult& evalResult)
{
    switch (evalResult.type) {
        case EvalValueType::STRING:
            return "string";
        case EvalValueType::NUMBER:
            return "number";
        case EvalValueType::BOOLEAN:
            return "boolean";
        case EvalValueType::JSON_VALUE:
            return evalResult.IsArray() ? "array" : "object";
        case EvalValueType::NULL_VALUE:
            return "null";
        case EvalValueType::UNDEFINED:
        default:
            return "undefined";
    }
}

napi_value CreateExpressionResult(napi_env env, const EvalResult& evalResult)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value result = nullptr;
    napi.CreateObject(env, &result);

    bool success = !evalResult.IsUndefined();
    napi_value successValue = nullptr;
    napi.GetBoolean(env, success, &successValue);
    napi.SetNamedProperty(env, result, "success", successValue);

    napi_value typeValue = nullptr;
    napi.CreateStringUtf8(env, EvalValueTypeToString(evalResult), NAPI_AUTO_LENGTH, &typeValue);
    napi.SetNamedProperty(env, result, "type", typeValue);

    napi_value value = nullptr;
    if (evalResult.IsString()) {
        napi.CreateStringUtf8(env, evalResult.stringValue.c_str(), NAPI_AUTO_LENGTH, &value);
    } else if (evalResult.IsNumber()) {
        napi.CreateDouble(env, evalResult.numberValue, &value);
    } else if (evalResult.IsBoolean()) {
        napi.GetBoolean(env, evalResult.boolValue, &value);
    } else if (evalResult.IsJson()) {
        value = JsonValueToNapiValue(env, evalResult.AsJson());
    } else if (evalResult.IsNull()) {
        napi.GetNull(env, &value);
    }
    if (value != nullptr) {
        napi.SetNamedProperty(env, result, "value", value);
    }
    return result;
}
#endif

std::string GetShortComponentType(const std::string& type)
{
    size_t separatorIndex = type.find_last_of('.');
    if (separatorIndex == std::string::npos || separatorIndex + 1 >= type.size()) {
        return type;
    }
    return type.substr(separatorIndex + 1);
}
} // namespace

// ============================================================================
// Helper functions for catalog parsing
// ============================================================================

namespace {

constexpr uint32_t MESSAGE_TYPE_CREATE_SURFACE = 0;
constexpr uint32_t MESSAGE_TYPE_UPDATE_COMPONENTS = 1;
constexpr uint32_t MESSAGE_TYPE_UPDATE_DATA_MODEL = 2;
constexpr uint32_t MESSAGE_TYPE_DELETE_SURFACE = 3;

const char* DescribeMessageType(uint32_t messageType)
{
    switch (messageType) {
        case MESSAGE_TYPE_CREATE_SURFACE:
            return "CREATE_SURFACE";
        case MESSAGE_TYPE_UPDATE_COMPONENTS:
            return "UPDATE_COMPONENTS";
        case MESSAGE_TYPE_UPDATE_DATA_MODEL:
            return "UPDATE_DATA_MODEL";
        case MESSAGE_TYPE_DELETE_SURFACE:
            return "DELETE_SURFACE";
        default:
            return "UNKNOWN";
    }
}

struct ParsedMessage {
    std::string version;
    std::string surfaceId;
    std::string catalogId;
    uint32_t messageType = 0;
    JsonValue messageBody;
};

struct MessageSelection {
    uint32_t count = 0;
    const char* key = nullptr;
    uint32_t type = 0;
};

struct ProcessResult {
    bool success = false;
    std::string errorCode;
    std::string errorMessage;
    bool hasSurfaceResultCode = false;
    int32_t surfaceResultCode = SURFACE_RESULT_OK;
    bool hasMessageMetadata = false;
    std::string surfaceId;
    uint32_t messageType = 0;
};

struct SurfacePolicyOptions {
    bool supportsMultipleSurfaces = false;
    int32_t maxSurfaceCount = -1;
    bool isExtend = false;
};

struct ProcessMessageInput {
    int32_t renderId = 0;
    std::string dsl;
    napi_value catalog = nullptr;
    SurfacePolicyOptions policyOptions;
};

struct MessageDispatchContext {
    int32_t renderId = 0;
    RenderSlot* renderSlot = nullptr;
    const std::shared_ptr<SurfaceManager>& surfaceManager;
    napi_env env = nullptr;
    napi_value catalog = nullptr;
};

struct CustomComponentActionInput {
    int32_t renderId = 0;
    std::string surfaceId;
    std::string componentId;
    std::string eventName;
    std::string contextJson;
};

struct CustomComponentActionTarget {
    std::shared_ptr<Component> component;
    std::shared_ptr<CustomComponent> customComponent;
};

ProcessResult Ok()
{
    ProcessResult result;
    result.success = true;
    return result;
}

ProcessResult Fail(const std::string& errorCode, const std::string& errorMessage)
{
    ProcessResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = errorMessage;
    return result;
}

ProcessResult Fail(const std::string& errorCode, const std::string& errorMessage, int32_t surfaceResultCode)
{
    ProcessResult result = Fail(errorCode, errorMessage);
    result.hasSurfaceResultCode = true;
    result.surfaceResultCode = surfaceResultCode;
    return result;
}

ProcessResult FailSurfaceNotFound(const std::string& surfaceId)
{
    return Fail("SURFACE_NOT_FOUND", "Surface not found: " + surfaceId, SURFACE_ERROR_NO_SURFACE_MATCHED);
}

ProcessResult Ok(const ParsedMessage& parsedMessage)
{
    ProcessResult result;
    result.success = true;
    result.hasMessageMetadata = true;
    result.surfaceId = parsedMessage.surfaceId;
    result.messageType = parsedMessage.messageType;
    return result;
}

napi_value CreateProcessResultValue(napi_env env, const ProcessResult& result)
{
    napi_value output = nullptr;
    NapiBridge::GetInstance().Provider().CreateObject(env, &output);

    napi_value successValue = nullptr;
    NapiBridge::GetInstance().Provider().GetBoolean(env, result.success, &successValue);
    NapiBridge::GetInstance().Provider().SetNamedProperty(env, output, "success", successValue);

    if (!result.success) {
        napi_value errorCodeValue = nullptr;
        NapiBridge::GetInstance().Provider().CreateStringUtf8(
            env, result.errorCode.c_str(), NAPI_AUTO_LENGTH, &errorCodeValue);
        NapiBridge::GetInstance().Provider().SetNamedProperty(env, output, "errorCode", errorCodeValue);

        napi_value errorMessageValue = nullptr;
        NapiBridge::GetInstance().Provider().CreateStringUtf8(
            env, result.errorMessage.c_str(), NAPI_AUTO_LENGTH, &errorMessageValue);
        NapiBridge::GetInstance().Provider().SetNamedProperty(env, output, "errorMessage", errorMessageValue);

        if (result.hasSurfaceResultCode) {
            napi_value surfaceResultCodeValue = nullptr;
            NapiBridge::GetInstance().Provider().CreateInt32(env, result.surfaceResultCode, &surfaceResultCodeValue);
            NapiBridge::GetInstance().Provider().SetNamedProperty(
                env, output, "surfaceResultCode", surfaceResultCodeValue);
        }
    }

    if (result.hasMessageMetadata) {
        napi_value surfaceIdValue = nullptr;
        NapiBridge::GetInstance().Provider().CreateStringUtf8(
            env, result.surfaceId.c_str(), NAPI_AUTO_LENGTH, &surfaceIdValue);
        NapiBridge::GetInstance().Provider().SetNamedProperty(env, output, "surfaceId", surfaceIdValue);

        napi_value messageTypeValue = nullptr;
        NapiBridge::GetInstance().Provider().CreateUint32(env, result.messageType, &messageTypeValue);
        NapiBridge::GetInstance().Provider().SetNamedProperty(env, output, "messageType", messageTypeValue);
    }

    return output;
}

bool IsBlankString(const std::string& value)
{
    if (value.empty()) {
        return true;
    }
    for (char ch : value) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string result = value;
    for (char& ch : result) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return result;
}

bool IsExtendedCatalogId(const std::string& catalogId)
{
    return ToLowerCopy(catalogId) == ToLowerCopy(A2UI_EXTENDED_CATALOG_ID);
}

bool ParseCreateSurfaceComponentTheme(const JsonValue& messageBody, ThemeContext& themeContext)
{
    if (!messageBody.IsObject() || !messageBody.Has("theme")) {
        return false;
    }

    JsonValue themeValue = messageBody.GetItem("theme");
    if (!themeValue.IsObject()) {
        LOG_A2UI(LOG_WARN, "ParseCreateSurfaceComponentTheme: createSurface.theme is not an object");
        return false;
    }

    themeContext.iconUrl = themeValue.GetString("iconUrl", "");
    themeContext.agentDisplayName = themeValue.GetString("agentDisplayName", "");

    std::string primaryColor = themeValue.GetString("primaryColor", "");
    uint32_t primaryColorArgb = 0;
    if (!primaryColor.empty()) {
        if (ThemeColorUtils::TryParseArgb(primaryColor, primaryColorArgb)) {
            themeContext.hasPrimaryColor = true;
            themeContext.primaryColorArgb = primaryColorArgb;
        } else {
            LOG_A2UI(
                LOG_WARN, "ParseCreateSurfaceComponentTheme: invalid primaryColor='%{public}s'", primaryColor.c_str());
        }
    }

    std::string darkPrimaryColor = themeValue.GetString("darkPrimaryColor", "");
    uint32_t darkPrimaryColorArgb = 0;
    if (!darkPrimaryColor.empty()) {
        if (ThemeColorUtils::TryParseArgb(darkPrimaryColor, darkPrimaryColorArgb)) {
            themeContext.hasDarkPrimaryColor = true;
            themeContext.darkPrimaryColorArgb = darkPrimaryColorArgb;
        } else {
            LOG_A2UI(LOG_WARN, "ParseCreateSurfaceComponentTheme: invalid darkPrimaryColor='%{public}s'",
                darkPrimaryColor.c_str());
        }
    }

    return true;
}

void ApplyCreateSurfaceTheme(SurfaceSlot& slot, const ThemeContext& defaultThemeContext, const JsonValue& messageBody)
{
    ThemeContext componentThemeContext;
    if (!ParseCreateSurfaceComponentTheme(messageBody, componentThemeContext)) {
        return;
    }

    slot.InitializeThemeManager(defaultThemeContext);
    std::shared_ptr<ThemeManager> themeManager = slot.GetThemeManager();
    if (themeManager == nullptr) {
        return;
    }

    themeManager->SetComponentTheme(componentThemeContext);
}

void DispatchDslWarning(int32_t renderId, const std::string& code, const std::string& message, const std::string& path,
    const std::string& itemName)
{
    WarningDispatchBridge::GetInstance().Dispatch(renderId, "", "", code, message, path, "message", itemName);
}

ProcessResult FailDslSchema(const std::string& errorCode, const std::string& message, int32_t surfaceResultCode)
{
    LOG_A2UI(LOG_ERROR, "ParseDslMessage: %{public}s", message.c_str());
    return Fail(errorCode, message, surfaceResultCode);
}

ProcessResult FailUnsupportedDslVersion(const std::string& version)
{
    LOG_A2UI(LOG_ERROR, "ParseDslMessage: unsupported A2UI protocol version=%{public}s", version.c_str());
    return Fail("UNSUPPORTED_PROTOCOL_VERSION", "unsupported A2UI protocol version",
        SURFACE_RESULT_UNSUPPORTED_PROTOCOL_VERSION);
}

ProcessResult ValidateDslVersion(JsonValue& root, int32_t renderId, std::string& version)
{
    JsonValue versionValue = root.GetItem("version");
    if (!versionValue.IsValid()) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Message version is required", "version", "dsl");
        return FailDslSchema("VERSION_INVALID", "version is invalid", SURFACE_RESULT_SCHEMA_VERSION_INVALID);
    }
    if (!versionValue.IsString()) {
        DispatchDslWarning(
            renderId, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Message version must be a string", "version", "dsl");
        return FailDslSchema("VERSION_INVALID", "version is invalid", SURFACE_RESULT_SCHEMA_VERSION_INVALID);
    }
    version = versionValue.GetStringValue("");
    if (IsBlankString(version)) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Message version is required", "version", "dsl");
        return FailDslSchema("VERSION_INVALID", "version is invalid", SURFACE_RESULT_SCHEMA_VERSION_INVALID);
    }
    if (!IsSupportedA2UIProtocolVersion(version)) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Message version is unsupported and native processing will reject this message", "version", "dsl");
        return FailUnsupportedDslVersion(version);
    }
    return Ok();
}

void SelectDslMessageBody(JsonValue& root, MessageSelection& selection)
{
    if (root.HasObjectItem("createSurface")) {
        selection = { selection.count + 1, "createSurface", MESSAGE_TYPE_CREATE_SURFACE };
    }
    if (root.HasObjectItem("updateComponents")) {
        selection = { selection.count + 1, "updateComponents", MESSAGE_TYPE_UPDATE_COMPONENTS };
    }
    if (root.HasObjectItem("updateDataModel")) {
        selection = { selection.count + 1, "updateDataModel", MESSAGE_TYPE_UPDATE_DATA_MODEL };
    }
    if (root.HasObjectItem("deleteSurface")) {
        selection = { selection.count + 1, "deleteSurface", MESSAGE_TYPE_DELETE_SURFACE };
    }
}

std::string BuildUnknownDslOperationKeys(JsonValue& root)
{
    std::string unknownKeys;
    JsonValue child = root.GetChild();
    while (child.IsValid()) {
        std::string key = child.GetKey();
        if (key != "version") {
            unknownKeys += unknownKeys.empty() ? key : (", " + key);
        }
        child = child.GetNext();
    }
    return unknownKeys;
}

ProcessResult ValidateDslOperationSelection(JsonValue& root, int32_t renderId, const MessageSelection& selection)
{
    if (selection.count == 0) {
        std::string unknownKeys = BuildUnknownDslOperationKeys(root);
        std::string errorMessage = unknownKeys.empty()
                                       ? "Message body is missing, expected exactly one of createSurface, "
                                         "updateComponents, updateDataModel, deleteSurface"
                                       : "Message operation is unknown: " + unknownKeys;
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_INVALID_VALUE, errorMessage, "root", "dsl");
        return FailDslSchema(
            "MESSAGE_OPERATION_INVALID", errorMessage, SURFACE_RESULT_SCHEMA_MESSAGE_OPERATION_INVALID);
    }
    if (selection.count > 1) {
        std::string errorMessage = "Message contains multiple bodies, only one is allowed";
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_INVALID_VALUE, errorMessage, "root", "dsl");
        return FailDslSchema("MESSAGE_MULTIPLE_BODIES", errorMessage, SURFACE_RESULT_SCHEMA_MESSAGE_MULTIPLE_BODIES);
    }
    return Ok();
}

ProcessResult ValidateDslMessageBody(
    JsonValue& body, int32_t renderId, const MessageSelection& selection, std::string& surfaceId)
{
    if (!body.IsObject()) {
        std::string errorMessage = std::string("Message ") + selection.key + " body must be an object";
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_TYPE_MISMATCH, errorMessage, selection.key, selection.key);
        return FailDslSchema("MESSAGE_BODY_INVALID", std::string(selection.key) + " body is invalid",
            SURFACE_RESULT_SCHEMA_MESSAGE_BODY_INVALID);
    }
    surfaceId = body.GetString("surfaceId", "");
    if (IsBlankString(surfaceId)) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Message surfaceId is required",
            std::string(selection.key) + ".surfaceId", selection.key);
        return FailDslSchema("SURFACE_ID_MISSING", std::string(selection.key) + ".surfaceId is invalid",
            SURFACE_RESULT_SCHEMA_SURFACE_ID_MISSING);
    }
    return Ok();
}

void DispatchUnsupportedComponentFieldExpressionWarnings(
    JsonValue& body, int32_t renderId, const MessageSelection& selection)
{
#ifdef ENABLE_EXPRESSION_ENGINE
    JsonValue components = body.GetObjectItem("components");
    constexpr const char* STRUCTURAL_FIELDS[] = { "id", "component" };
    for (int32_t index = 0; index < components.GetArraySize(); ++index) {
        JsonValue component = components.GetArrayItem(index);
        if (!component.IsObject()) {
            continue;
        }
        for (const char* field : STRUCTURAL_FIELDS) {
            JsonValue value = component.GetItem(field);
            if (!value.IsString() || !ExpressionEngine::IsExpression(value.GetStringValue(""))) {
                continue;
            }
            std::string path = std::string(selection.key) + ".components[" + std::to_string(index) + "]." + field;
            DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Component field " + std::string(field) + " does not support expression values", path, selection.key);
        }
    }
#else
    (void)body;
    (void)renderId;
    (void)selection;
#endif
}

ProcessResult ValidateTypedDslMessageBody(JsonValue& body, int32_t renderId, const MessageSelection& selection)
{
    if (selection.type == MESSAGE_TYPE_CREATE_SURFACE && IsBlankString(body.GetString("catalogId", ""))) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Message catalogId is required",
            std::string(selection.key) + ".catalogId", selection.key);
        return FailDslSchema("CATALOG_ID_MISSING", std::string(selection.key) + ".catalogId is invalid",
            SURFACE_RESULT_SCHEMA_CATALOG_ID_MISSING);
    }
    if (selection.type == MESSAGE_TYPE_UPDATE_COMPONENTS && !body.GetObjectItem("components").IsArray()) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Message components must be an array",
            std::string(selection.key) + ".components", selection.key);
        return FailDslSchema("COMPONENTS_INVALID", std::string(selection.key) + ".components is invalid",
            SURFACE_RESULT_SCHEMA_COMPONENTS_INVALID);
    }
    if (selection.type == MESSAGE_TYPE_UPDATE_COMPONENTS) {
        DispatchUnsupportedComponentFieldExpressionWarnings(body, renderId, selection);
    }
    if (selection.type == MESSAGE_TYPE_UPDATE_DATA_MODEL && !body.HasObjectItem("path") &&
        !body.HasObjectItem("value")) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Message updateDataModel requires path or value, fallback path to \"/\"",
            std::string(selection.key) + ".path", selection.key);
        LOG_A2UI(LOG_WARN, "ParseDslMessage: updateDataModel requires path or value, fallback path to /");
        body.PutString("path", "/");
    }
    return Ok();
}

void DispatchUndefinedDslRootFields(JsonValue& root, int32_t renderId, const char* messageKey)
{
    JsonValue checkChild = root.GetChild();
    while (checkChild.IsValid()) {
        std::string key = checkChild.GetKey();
        if (key != "version" && key != messageKey) {
            DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
                "Root field " + key + " is undefined in schema and has been removed", key, "dsl");
        }
        checkChild = checkChild.GetNext();
    }
}

ProcessResult ParseDslMessage(const std::string& dsl, int32_t renderId, ParsedMessage& parsedMessage)
{
    if (IsBlankString(dsl)) {
        return FailDslSchema("DSL_EMPTY", "dsl is empty", SURFACE_RESULT_SCHEMA_DSL_EMPTY);
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(dsl);
    if (adapter == nullptr) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_SCHEMA_PARSE_FAILED, "DSL JSON parse failed", "root", "dsl");
        return FailDslSchema("JSON_PARSE_FAILED", "dsl json parse failed", SURFACE_RESULT_SCHEMA_JSON_PARSE_FAILED);
    }

    JsonValue root = adapter->GetRoot();
    if (!root.IsObject()) {
        DispatchDslWarning(renderId, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Root JSON must be an object", "root", "dsl");
        return FailDslSchema("ROOT_NOT_OBJECT", "root json must be an object", SURFACE_RESULT_SCHEMA_ROOT_NOT_OBJECT);
    }

    std::string version;
    ProcessResult versionResult = ValidateDslVersion(root, renderId, version);
    if (!versionResult.success) {
        return versionResult;
    }

    MessageSelection selection;
    SelectDslMessageBody(root, selection);
    ProcessResult selectionResult = ValidateDslOperationSelection(root, renderId, selection);
    if (!selectionResult.success) {
        return selectionResult;
    }
    JsonValue body = root.GetObjectItem(selection.key);
    std::string surfaceId;
    ProcessResult bodyResult = ValidateDslMessageBody(body, renderId, selection, surfaceId);
    if (!bodyResult.success) {
        return bodyResult;
    }
    ProcessResult typedBodyResult = ValidateTypedDslMessageBody(body, renderId, selection);
    if (!typedBodyResult.success) {
        return typedBodyResult;
    }
    DispatchUndefinedDslRootFields(root, renderId, selection.key);

    parsedMessage.version = version;
    parsedMessage.surfaceId = surfaceId;
    if (selection.type == MESSAGE_TYPE_CREATE_SURFACE) {
        parsedMessage.catalogId = body.GetString("catalogId", "");
    }
    parsedMessage.messageType = selection.type;
    parsedMessage.messageBody = body;
    return Ok();
}

SurfacePolicyOptions ParseSurfacePolicyOptions(napi_env env, napi_value optionsValue)
{
    SurfacePolicyOptions options;
    if (optionsValue == nullptr) {
        return options;
    }

    napi_valuetype valueType = napi_undefined;
    if (NapiBridge::GetInstance().Provider().Typeof(env, optionsValue, &valueType) != napi_ok ||
        valueType != napi_object) {
        return options;
    }

    if (NapiHasProperty(env, optionsValue, "supportsMultipleSurfaces")) {
        napi_value allowMultipleValue = NapiGetProperty(env, optionsValue, "supportsMultipleSurfaces");
        bool allowMultiple = false;
        NapiBridge::GetInstance().Provider().GetValueBool(env, allowMultipleValue, &allowMultiple);
        options.supportsMultipleSurfaces = allowMultiple;
    }

    options.maxSurfaceCount = NapiGetInt32(env, optionsValue, "maxSurfaceCount", -1);
    if (NapiHasProperty(env, optionsValue, "isExtend")) {
        napi_value isExtendValue = NapiGetProperty(env, optionsValue, "isExtend");
        bool isExtend = false;
        NapiBridge::GetInstance().Provider().GetValueBool(env, isExtendValue, &isExtend);
        options.isExtend = isExtend;
    }
    return options;
}

ProcessResult ValidateSurfaceProtocol(const ParsedMessage& parsedMessage, const SurfacePolicyOptions& options)
{
    if (parsedMessage.messageType != MESSAGE_TYPE_CREATE_SURFACE) {
        return Ok();
    }

    bool isExtendedCatalog = IsExtendedCatalogId(parsedMessage.catalogId);
    if (options.isExtend) {
        if (isExtendedCatalog) {
            return Ok();
        }
        return Fail(
            "PROTOCOL_MISMATCH", "SurfaceController expects extended protocol. createSurface.catalogId must be " +
                                     std::string(A2UI_EXTENDED_CATALOG_ID));
    }

    if (!isExtendedCatalog) {
        return Ok();
    }
    return Fail("PROTOCOL_MISMATCH", "SurfaceController expects basic protocol. createSurface.catalogId must not be " +
                                         std::string(A2UI_EXTENDED_CATALOG_ID));
}

ProcessResult ValidateSurfacePolicy(const ParsedMessage& parsedMessage,
    const std::shared_ptr<SurfaceManager>& surfaceManager, const SurfacePolicyOptions& options)
{
    if (parsedMessage.messageType != MESSAGE_TYPE_CREATE_SURFACE || surfaceManager == nullptr) {
        return Ok();
    }

    if (!surfaceManager->HasSurface(parsedMessage.surfaceId) && surfaceManager->GetSurfaceCount() > 0) {
        if (!options.supportsMultipleSurfaces) {
            std::vector<std::string> surfaceIds = surfaceManager->GetSurfaceIds();
            std::string activeSurfaceId = surfaceIds.empty() ? "" : surfaceIds[0];
            return Fail("MULTI_SURFACE_DISABLED",
                "SurfaceController only supports one surface. Active surface: " + activeSurfaceId,
                SURFACE_RESULT_MULTI_SURFACE_DISABLED);
        }

        if (options.maxSurfaceCount >= 0 &&
            surfaceManager->GetSurfaceCount() >= static_cast<size_t>(options.maxSurfaceCount)) {
            return Fail("MAX_SURFACE_LIMIT_REACHED",
                "SurfaceController supports at most " + std::to_string(options.maxSurfaceCount) + " surfaces.",
                SURFACE_RESULT_MAX_SURFACE_LIMIT_REACHED);
        }
    }

    return Ok();
}

void MaybeDispatchCreateSurfaceCatalogMismatchWarning(
    const ParsedMessage& parsedMessage, napi_env env, napi_value catalogNapi, int32_t renderId)
{
    if (parsedMessage.messageType != MESSAGE_TYPE_CREATE_SURFACE || catalogNapi == nullptr) {
        return;
    }

    const std::string& messageCatalogId = parsedMessage.catalogId;
    if (IsBlankString(messageCatalogId)) {
        return;
    }

    std::string controllerCatalogId = NapiGetString(env, catalogNapi, "id", "");
    if (IsBlankString(controllerCatalogId) || controllerCatalogId == messageCatalogId) {
        return;
    }

    WarningDispatchBridge::GetInstance().Dispatch(renderId, parsedMessage.surfaceId, "",
        SCHEMA_ERROR_CODE_INVALID_VALUE,
        "Message catalogId differs from SurfaceController catalog.id and native "
        "processing will use "
        "createSurface.catalogId",
        "createSurface.catalogId", "message", "createSurface");
}

static std::shared_ptr<CatalogItem> ParseCatalogItem(napi_env env, napi_value itemNapi)
{
    std::string name = NapiGetString(env, itemNapi, "name", "");
    if (name.empty()) {
        return nullptr;
    }

    int32_t categoryValue = NapiGetInt32(env, itemNapi, "category", 1);
    int32_t typeValue = NapiGetInt32(env, itemNapi, "type", 1);

    bool isInnerNative = false;
    if (NapiHasProperty(env, itemNapi, "isInnerNative")) {
        napi_value boolValue = NapiGetProperty(env, itemNapi, "isInnerNative");
        NapiBridge::GetInstance().Provider().GetValueBool(env, boolValue, &isInnerNative);
    }

    bool preserveDynamicDescriptors = false;
    if (NapiHasProperty(env, itemNapi, "preserveDynamicDescriptors")) {
        napi_value boolValue = NapiGetProperty(env, itemNapi, "preserveDynamicDescriptors");
        NapiBridge::GetInstance().Provider().GetValueBool(env, boolValue, &preserveDynamicDescriptors);
    }

    auto item = std::make_shared<CatalogItem>(name, static_cast<CatalogItemType>(typeValue));
    item->SetCategory(static_cast<CatalogCategory>(categoryValue));
    item->SetInnerNative(isInnerNative);
    item->SetPreserveDynamicDescriptors(preserveDynamicDescriptors);

    return item;
}

static void AddCatalogItemsFromNapi(
    napi_env env, napi_value arrayNapi, const std::shared_ptr<Catalog>& catalog, bool asFunction)
{
    if (!NapiIsArray(env, arrayNapi)) {
        return;
    }
    uint32_t length = NapiGetArrayLength(env, arrayNapi);
    for (uint32_t i = 0; i < length; ++i) {
        napi_value itemNapi = NapiGetElement(env, arrayNapi, i);
        auto item = ParseCatalogItem(env, itemNapi);
        if (item == nullptr) {
            continue;
        }
        if (asFunction) {
            catalog->AddFunction(item);
        } else {
            catalog->AddComponent(item);
        }
    }
}

static std::shared_ptr<Catalog> ParseCatalog(napi_env env, napi_value catalogNapi)
{
    if (catalogNapi == nullptr) {
        return nullptr;
    }

    std::string catalogId = NapiGetString(env, catalogNapi, "id", "");
    std::string a2UIProtocolVersion =
        NapiGetString(env, catalogNapi, "a2UIProtocolVersion", DEFAULT_A2UI_PROTOCOL_VERSION);

    auto catalog = std::make_shared<Catalog>(catalogId, a2UIProtocolVersion);
    LOG_A2UI(LOG_ERROR, "ParseCatalog=%{public}s, a2UIProtocolVersion=%{public}s", catalogId.c_str(),
        a2UIProtocolVersion.c_str());
    if (NapiHasProperty(env, catalogNapi, "components")) {
        AddCatalogItemsFromNapi(env, NapiGetProperty(env, catalogNapi, "components"), catalog, false);
    }

    if (NapiHasProperty(env, catalogNapi, "functions")) {
        AddCatalogItemsFromNapi(env, NapiGetProperty(env, catalogNapi, "functions"), catalog, true);
    }

    return catalog;
}

static void ResolveCatalogForSurface(
    SurfaceSlot* slot, const std::string& surfaceCatalogId, napi_env env, napi_value catalogNapi)
{
    if (slot == nullptr) {
        return;
    }

    slot->SetSurfaceCatalogId(surfaceCatalogId);
    auto catalog = ParseCatalog(env, catalogNapi);
    slot->SetCatalog(catalog);
    return;
}

ProcessResult UpdateNativeTreeInternal(
    int32_t renderId, const std::string& surfaceId, const JsonValue& messageBody, napi_env env, napi_value catalogNapi)
{
    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_ERROR, "UpdateNativeTreeInternal: RenderSlot not found for renderId=%{public}d", renderId);
        return Fail("RENDER_SLOT_NOT_FOUND", "RenderSlot not found for renderId=" + std::to_string(renderId));
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        LOG_A2UI(LOG_ERROR, "UpdateNativeTreeInternal: SurfaceManager is nullptr for renderId=%{public}d", renderId);
        return Fail("SURFACE_MANAGER_NOT_READY", "SurfaceManager is nullptr for renderId=" + std::to_string(renderId));
    }

    SurfaceSlot* slot = surfaceManager->FindSurface(surfaceId);
    if (slot == nullptr) {
        LOG_A2UI(LOG_ERROR, "UpdateNativeTreeInternal: surface not found, surfaceId=%{public}s", surfaceId.c_str());
        return FailSurfaceNotFound(surfaceId);
    }

    if (slot->GetCatalog() == nullptr) {
        ResolveCatalogForSurface(slot, "", env, catalogNapi);
    }
    bool success = slot->UpdateComponents(messageBody);
    return success ? Ok() : Fail("UPDATE_COMPONENTS_FAILED", "Update components failed for surfaceId=" + surfaceId);
}

std::string ResolveComponentBindingPath(const std::shared_ptr<Component>& component, const std::string& propertyName)
{
    if (component == nullptr || propertyName.empty()) {
        return "";
    }

    const auto& bindings = component->GetDataBindings();
    for (auto iter = bindings.rbegin(); iter != bindings.rend(); ++iter) {
        if (iter->propertyName_ == propertyName && !iter->dataPath_.empty()) {
            return iter->dataPath_;
        }
    }
    return "";
}

ProcessResult SyncComponentBoundDataModelInternal(int32_t renderId, const std::string& surfaceId,
    const std::string& componentId, const std::string& propertyName, const std::string& valueJson)
{
    if (componentId.empty() || propertyName.empty()) {
        return Fail("INVALID_ARGUMENT", "componentId or propertyName is empty");
    }

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        return Fail("RENDER_SLOT_NOT_FOUND", "RenderSlot not found for renderId=" + std::to_string(renderId));
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return Fail("SURFACE_MANAGER_NOT_READY", "SurfaceManager is nullptr for renderId=" + std::to_string(renderId));
    }

    std::string resolvedSurfaceId = surfaceId.empty() ? "default" : surfaceId;
    SurfaceSlot* slot = surfaceManager->FindSurface(resolvedSurfaceId);
    if (slot == nullptr) {
        return FailSurfaceNotFound(resolvedSurfaceId);
    }

    std::shared_ptr<Component> component = slot->FindComponentById(componentId);
    if (component == nullptr) {
        return Fail("COMPONENT_NOT_FOUND", "Component not found: " + componentId);
    }

    auto valueAdapter = JsonAdapter::Parse(valueJson);
    if (valueAdapter == nullptr || !valueAdapter->GetRoot().IsValid()) {
        return Fail("INVALID_ARGUMENT", "valueJson parse failed");
    }

    std::string bindingPath = ResolveComponentBindingPath(component, propertyName);
    if (bindingPath.empty()) {
        component->OnDataUpdate(propertyName, valueAdapter->GetRoot());
        return Ok();
    }

    std::shared_ptr<BindingEngine> bindingEngine = slot->GetBindingEngine();
    if (bindingEngine == nullptr) {
        return Fail("BINDING_ENGINE_NOT_READY", "BindingEngine is nullptr for surfaceId=" + resolvedSurfaceId);
    }

    bindingEngine->UpdateDataModelByPath(resolvedSurfaceId, bindingPath, valueAdapter->GetRoot());
    return Ok();
}

ProcessResult DispatchCreateSurfaceMessage(const ParsedMessage& parsedMessage, RenderSlot* renderSlot,
    const std::shared_ptr<SurfaceManager>& surfaceManager, napi_env env, napi_value catalog)
{
    const std::string& surfaceId = parsedMessage.surfaceId;
    if (surfaceManager->HasSurface(surfaceId)) {
        LOG_A2UI(LOG_ERROR, "ProcessMessage: Surface already exists for surfaceId=%{public}s", surfaceId.c_str());
        return Fail(
            "SURFACE_ALREADY_EXISTS", "Surface already exists: " + surfaceId, SURFACE_RESULT_SURFACE_ALREADY_EXISTS);
    }

    SurfaceSlot& slot = surfaceManager->CreateSurface(surfaceId, renderSlot->GetContentHandle());
    ApplyCreateSurfaceTheme(slot, surfaceManager->GetThemeContext(), parsedMessage.messageBody);
    ResolveCatalogForSurface(&slot, parsedMessage.catalogId, env, catalog);
    return Ok(parsedMessage);
}

ProcessResult DispatchUpdateComponentsMessage(
    int32_t renderId, const ParsedMessage& parsedMessage, napi_env env, napi_value catalog)
{
    ProcessResult updateResult =
        UpdateNativeTreeInternal(renderId, parsedMessage.surfaceId, parsedMessage.messageBody, env, catalog);
    return updateResult.success ? Ok(parsedMessage) : updateResult;
}

ProcessResult DispatchUpdateDataModelMessage(
    const ParsedMessage& parsedMessage, const std::shared_ptr<SurfaceManager>& surfaceManager)
{
    const std::string& surfaceId = parsedMessage.surfaceId;
    SurfaceSlot* slot = surfaceManager->FindSurface(surfaceId);
    if (slot == nullptr) {
        return FailSurfaceNotFound(surfaceId);
    }

    bool success = slot->UpdateDataModel(parsedMessage.messageBody);
    if (!success) {
        return Fail("UPDATE_DATA_MODEL_FAILED", "Update data model failed for surfaceId=" + surfaceId);
    }
    return Ok(parsedMessage);
}

ProcessResult DispatchDeleteSurfaceMessage(
    const ParsedMessage& parsedMessage, const std::shared_ptr<SurfaceManager>& surfaceManager)
{
    const std::string& surfaceId = parsedMessage.surfaceId;
    SurfaceSlot* slot = surfaceManager->FindSurface(surfaceId);
    if (slot == nullptr) {
        return FailSurfaceNotFound(surfaceId);
    }

    surfaceManager->RemoveSurface(surfaceId);
    return Ok(parsedMessage);
}

ProcessResult DispatchV09Message(const ParsedMessage& parsedMessage, const MessageDispatchContext& context)
{
    const std::string& surfaceId = parsedMessage.surfaceId;
    uint32_t messageType = parsedMessage.messageType;

    switch (messageType) {
        case MESSAGE_TYPE_CREATE_SURFACE: {
            HiTraceScoped trace("SurfaceController:onReceive:CREATE_SURFACE");
            return DispatchCreateSurfaceMessage(
                parsedMessage, context.renderSlot, context.surfaceManager, context.env, context.catalog);
        }
        case MESSAGE_TYPE_UPDATE_COMPONENTS: {
            HiTraceScoped trace("SurfaceController:onReceive:UPDATE_COMPONENTS");
            return DispatchUpdateComponentsMessage(context.renderId, parsedMessage, context.env, context.catalog);
        }
        case MESSAGE_TYPE_UPDATE_DATA_MODEL: {
            HiTraceScoped trace("SurfaceController:onReceive:UPDATE_DATA_MODEL");
            return DispatchUpdateDataModelMessage(parsedMessage, context.surfaceManager);
        }
        case MESSAGE_TYPE_DELETE_SURFACE: {
            HiTraceScoped trace("SurfaceController:onReceive:DELETE_SURFACE");
            return DispatchDeleteSurfaceMessage(parsedMessage, context.surfaceManager);
        }
        default:
            LOG_A2UI(LOG_ERROR, "ProcessMessage: unknown messageType=%{public}u, surfaceId=%{public}s", messageType,
                surfaceId.c_str());
            return Fail("UNKNOWN_MESSAGE_TYPE", "Unknown message type: " + std::to_string(messageType));
    }
}

ProcessResult DispatchMessageByVersion(const ParsedMessage& parsedMessage, const MessageDispatchContext& context)
{
    if (parsedMessage.version == DEFAULT_A2UI_PROTOCOL_VERSION) {
        return DispatchV09Message(parsedMessage, context);
    }

    return Fail("UNSUPPORTED_PROTOCOL_VERSION", "unsupported A2UI protocol version",
        SURFACE_RESULT_UNSUPPORTED_PROTOCOL_VERSION);
}

ProcessResult ParseProcessMessageInput(napi_env env, napi_callback_info info, ProcessMessageInput& input)
{
    size_t argc = 4;
    napi_value args[4] = { nullptr, nullptr, nullptr, nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    LOG_A2UI(LOG_DEBUG, "ProcessMessage: received, argc=%{public}zu", argc);
    if (argc < 3 || args[0] == nullptr || args[1] == nullptr || args[2] == nullptr) {
        LOG_A2UI(LOG_ERROR, "ProcessMessage: invalid arguments, argc=%{public}zu", argc);
        return Fail("INVALID_ARGUMENT", "ProcessMessage: invalid arguments");
    }

    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &input.renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "ProcessMessage: failed to get renderId from napi value");
        return Fail("INVALID_ARGUMENT", "ProcessMessage: failed to get renderId from napi value");
    }

    input.dsl = NapiGetStringValue(env, args[1]);
    input.catalog = args[2];
    input.policyOptions = ParseSurfacePolicyOptions(env, args[3]);
    return Ok();
}

ProcessResult DispatchDslLengthErrorIfNeeded(int32_t renderId, const std::string& dsl)
{
    constexpr size_t MAX_DSL_LENGTH = 10 * 1024;
    if (dsl.size() <= MAX_DSL_LENGTH) {
        return Ok();
    }

    std::string errorMessage = "DSL string length " + std::to_string(dsl.size()) + " exceeds maximum allowed " +
                               std::to_string(MAX_DSL_LENGTH);
    LOG_A2UI(LOG_ERROR, "ProcessMessage: %{public}s", errorMessage.c_str());
    return Fail("NATIVE_PROCESS_FAILED", errorMessage, SURFACE_ERROR_NATIVE_PROCESS_FAILED);
}

napi_value CreateTimedProcessResultValue(napi_env env, const ProcessResult& result,
    std::chrono::steady_clock::time_point startTime, const ParsedMessage& parsedMessage)
{
    if (result.success) {
        auto durationMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        LOG_A2UI(LOG_INFO,
            "ProcessMessage timing: messageType=%{public}s, surfaceId=%{public}s, durationMs=%{public}lld",
            DescribeMessageType(parsedMessage.messageType), parsedMessage.surfaceId.c_str(),
            static_cast<long long>(durationMs));
    }
    return CreateProcessResultValue(env, result);
}

ProcessResult ResolveProcessMessageTarget(int32_t renderId, const ParsedMessage& parsedMessage,
    const SurfacePolicyOptions& policyOptions, RenderSlot*& renderSlot, std::shared_ptr<SurfaceManager>& surfaceManager)
{
    renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_ERROR, "ProcessMessage: RenderSlot not found for renderId=%{public}d", renderId);
        return Fail("RENDER_SLOT_NOT_FOUND", "RenderSlot not found for renderId=" + std::to_string(renderId));
    }

    surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        LOG_A2UI(LOG_ERROR, "ProcessMessage: SurfaceManager is nullptr for renderId=%{public}d", renderId);
        return Fail("SURFACE_MANAGER_NOT_READY", "SurfaceManager is nullptr for renderId=" + std::to_string(renderId));
    }

    ProcessResult protocolResult = ValidateSurfaceProtocol(parsedMessage, policyOptions);
    if (!protocolResult.success) {
        return protocolResult;
    }
    return ValidateSurfacePolicy(parsedMessage, surfaceManager, policyOptions);
}

bool ParseCustomComponentActionInput(napi_env env, napi_callback_info info, CustomComponentActionInput& input)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 5;
    napi_value args[5] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 5) {
        LOG_A2UI(LOG_ERROR, "DispatchCustomComponentAction: expected 5 args, got %{public}zu", argc);
        return false;
    }

    if (napi.GetValueInt32(env, args[0], &input.renderId) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "DispatchCustomComponentAction: failed to get renderId");
        return false;
    }

    input.surfaceId = NapiGetStringValue(env, args[1]);
    input.componentId = NapiGetStringValue(env, args[2]);
    input.eventName = NapiGetStringValue(env, args[3]);
    input.contextJson = NapiGetStringValue(env, args[4]);
    return true;
}

bool FindCustomComponentActionTarget(const CustomComponentActionInput& input, CustomComponentActionTarget& target)
{
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(input.renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "DispatchCustomComponentAction: RenderSlot not found, renderId=%{public}d", input.renderId);
        return false;
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return false;
    }

    SurfaceSlot* surface = surfaceManager->FindSurface(input.surfaceId);
    if (surface == nullptr) {
        return false;
    }

    target.component = surface->FindComponentById(input.componentId);
    target.customComponent = std::dynamic_pointer_cast<CustomComponent>(target.component);
    if (target.customComponent == nullptr) {
        LOG_A2UI(LOG_WARN,
            "DispatchCustomComponentAction: component not found or not a CustomComponent, componentId=%{public}s",
            input.componentId.c_str());
        return false;
    }
    return true;
}

JsonValue ParseCustomComponentActionContext(const std::string& contextJson)
{
    if (contextJson.empty()) {
        return JsonValue();
    }

    std::unique_ptr<JsonAdapter> contextAdapter = JsonAdapter::Parse(contextJson);
    if (contextAdapter == nullptr) {
        return JsonValue();
    }
    return contextAdapter->GetRoot();
}

std::string FindCheckedBindingPath(const std::shared_ptr<Component>& component)
{
    const auto& bindings = component->GetDataBindings();
    for (auto iter = bindings.rbegin(); iter != bindings.rend(); ++iter) {
        if (iter->propertyName_ == "checked" && !iter->dataPath_.empty()) {
            return iter->dataPath_;
        }
    }
    return "";
}

bool TryGetCheckedValue(const JsonValue& extraContext, bool& checkedValue)
{
    if (extraContext.IsBool()) {
        checkedValue = extraContext.GetBoolValue(false);
        return true;
    }
    if (extraContext.IsObject() && extraContext.Has("checked")) {
        checkedValue = extraContext.GetBool("checked", false);
        return true;
    }
    return false;
}

void SyncCustomComponentCheckedState(const CustomComponentActionTarget& target, const JsonValue& extraContext)
{
    bool checkedValue = false;
    bool hasCheckedValue = TryGetCheckedValue(extraContext, checkedValue);
    if (hasCheckedValue && GetShortComponentType(target.customComponent->GetType()) == "Radio") {
        std::unique_ptr<JsonAdapter> checkedAdapter = JsonAdapter::CreateBool(checkedValue);
        if (checkedAdapter != nullptr) {
            target.customComponent->SetRuntimeCustomProperty("checked", checkedAdapter->GetRoot());
        }
    }

    std::string checkedBindingPath = FindCheckedBindingPath(target.component);
    if (!checkedBindingPath.empty() && hasCheckedValue) {
        target.customComponent->SyncCheckedToBoundDataModel(checkedBindingPath, checkedValue);
    }
}

void SyncCustomComponentRuntimeValues(
    const std::shared_ptr<CustomComponent>& customComponent, const JsonValue& extraContext)
{
    if (!extraContext.IsObject()) {
        return;
    }

    JsonValue selectedVal =
        extraContext.Has("index") ? extraContext.GetItem("index") : extraContext.GetItem("selected");
    if (selectedVal.IsValid()) {
        customComponent->SetRuntimeCustomProperty("selected", selectedVal);
    }
    if (extraContext.Has("value")) {
        JsonValue valueVal = extraContext.GetItem("value");
        if (valueVal.IsValid()) {
            customComponent->SetRuntimeCustomProperty("value", valueVal);
        }
    }
}

} // namespace

// ============================================================================
// New NAPI functions for RenderSlot-based architecture
// ============================================================================

napi_value InitRenderSlot(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_status cbInfoStatus = NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (cbInfoStatus != napi_ok || argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_ERROR, "InitRenderSlot: failed to parse args, status=%{public}d, argc=%{public}zu",
            static_cast<int32_t>(cbInfoStatus), argc);
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "InitRenderSlot: failed to get renderId from napi value");
        return nullptr;
    }

    if (renderId < 0) {
        LOG_A2UI(LOG_ERROR, "InitRenderSlot: invalid renderId=%{public}d", renderId);
        return nullptr;
    }

    LOG_A2UI(LOG_INFO, "InitRenderSlot: renderId=%{public}d", renderId);

    auto& renderManager = RenderManager::GetInstance();
    renderManager.CreateRenderSlot(renderId);

    RegisterBuiltInActions(NativeActionRegistry::GetInstance());

    return nullptr;
}

napi_value DestroyRenderSlot(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_ERROR, "DestroyRenderSlot: invalid arguments, argc=%{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "DestroyRenderSlot: failed to get renderId from napi value");
        return nullptr;
    }

    LOG_A2UI(LOG_INFO, "DestroyRenderSlot: renderId=%{public}d", renderId);
    RenderManager::GetInstance().RemoveRenderSlot(renderId);
    return nullptr;
}

napi_value ProcessMessage(napi_env env, napi_callback_info info)
{
    ProcessMessageInput input;
    ProcessResult inputResult = ParseProcessMessageInput(env, info, input);
    if (!inputResult.success) {
        return CreateProcessResultValue(env, inputResult);
    }

    ProcessResult dslLengthResult = DispatchDslLengthErrorIfNeeded(input.renderId, input.dsl);
    if (!dslLengthResult.success) {
        return CreateProcessResultValue(env, dslLengthResult);
    }

    ParsedMessage parsedMessage;
    ProcessResult parseResult = ParseDslMessage(input.dsl, input.renderId, parsedMessage);
    if (!parseResult.success) {
        return CreateProcessResultValue(env, parseResult);
    }

    auto startTime = std::chrono::steady_clock::now();
    LOG_A2UI(LOG_DEBUG, "ProcessMessage: parsed, renderId=%{public}d, surfaceId=%{public}s, messageType=%{public}u",
        input.renderId, parsedMessage.surfaceId.c_str(), parsedMessage.messageType);

    RenderSlot* renderSlot = nullptr;
    std::shared_ptr<SurfaceManager> surfaceManager;
    ProcessResult targetResult =
        ResolveProcessMessageTarget(input.renderId, parsedMessage, input.policyOptions, renderSlot, surfaceManager);
    if (!targetResult.success) {
        return CreateTimedProcessResultValue(env, targetResult, startTime, parsedMessage);
    }

    MaybeDispatchCreateSurfaceCatalogMismatchWarning(parsedMessage, env, input.catalog, input.renderId);
    MessageDispatchContext dispatchContext = {
        .renderId = input.renderId,
        .renderSlot = renderSlot,
        .surfaceManager = surfaceManager,
        .env = env,
        .catalog = input.catalog,
    };
    ProcessResult dispatchResult = DispatchMessageByVersion(parsedMessage, dispatchContext);
    return CreateTimedProcessResultValue(env, dispatchResult, startTime, parsedMessage);
}

napi_value BindSurfaceToRender(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = { nullptr, nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2 || args[0] == nullptr || args[1] == nullptr) {
        LOG_A2UI(LOG_ERROR, "BindSurfaceToRender: invalid arguments, argc=%{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "BindSurfaceToRender: failed to get renderId from napi value");
        return nullptr;
    }

    A2UINodeContentHandle contentHandle = nullptr;
    ArkUIOHApiAdapter::GetNodeContentFromNapiValue(env, args[1], &contentHandle);

    LOG_A2UI(LOG_INFO, "BindSurfaceToRender: renderId=%{public}d", renderId);

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "BindSurfaceToRender: RenderSlot not found for renderId=%{public}d", renderId);
        return nullptr;
    }

    // Set content handle to RenderSlot, which will also set it to the latest surface
    renderSlot->SetContentHandle(contentHandle);

    LOG_A2UI(LOG_INFO, "BindSurfaceToRender: Successfully bound surface for renderId=%{public}d", renderId);

    return nullptr;
}

napi_value UnbindSurfaceFromRender(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_ERROR, "UnbindSurfaceFromRender: invalid arguments, argc=%{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "UnbindSurfaceFromRender: failed to get renderId from napi value");
        return nullptr;
    }

    LOG_A2UI(LOG_INFO, "UnbindSurfaceFromRender: renderId=%{public}d", renderId);

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "UnbindSurfaceFromRender: RenderSlot not found for renderId=%{public}d", renderId);
        return nullptr;
    }

    renderSlot->SetContentHandle(nullptr);
    LOG_A2UI(LOG_INFO, "UnbindSurfaceFromRender: Cleared bound content handle for renderId=%{public}d", renderId);
    return nullptr;
}

napi_value SetRootFillMode(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 2;
    napi_value args[2] = { nullptr, nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2 || args[0] == nullptr || args[1] == nullptr) {
        LOG_A2UI(LOG_ERROR, "SetRootFillMode: invalid arguments, argc=%{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    if (napi.GetValueInt32(env, args[0], &renderId) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetRootFillMode: failed to parse renderId");
        return nullptr;
    }

    bool forceFill = false;
    if (napi.GetValueBool(env, args[1], &forceFill) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetRootFillMode: failed to parse forceFill");
        return nullptr;
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "SetRootFillMode: RenderSlot not found, renderId=%{public}d", renderId);
        return nullptr;
    }

    renderSlot->SetRootFillMode(forceFill);
    return nullptr;
}

napi_value PopSurface(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_ERROR, "PopSurface: invalid arguments, argc=%{public}zu", argc);
        return CreateSurfaceResult(env, false, SURFACE_RESULT_EMPTY_STACK, "renderId is required");
    }

    int32_t renderId = 0;
    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "PopSurface: failed to get renderId from napi value");
        return CreateSurfaceResult(env, false, SURFACE_RESULT_EMPTY_STACK, "invalid renderId");
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "PopSurface: RenderSlot not found for renderId=%{public}d", renderId);
        return CreateSurfaceResult(env, false, SURFACE_RESULT_EMPTY_STACK, "render slot not found");
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        LOG_A2UI(LOG_ERROR, "PopSurface: SurfaceManager is nullptr for renderId=%{public}d", renderId);
        return CreateSurfaceResult(env, false, SURFACE_RESULT_EMPTY_STACK, "surface manager not found");
    }

    size_t surfaceCount = surfaceManager->GetSurfaceCount();
    if (surfaceCount == 0) {
        return CreateSurfaceResult(env, false, SURFACE_RESULT_EMPTY_STACK, "surface stack is empty");
    }
    if (surfaceCount == 1) {
        return CreateSurfaceResult(
            env, false, SURFACE_RESULT_ONLY_ONE_SURFACE, "surface stack only contains one surface");
    }

    bool success = surfaceManager->Back();
    if (!success) {
        return CreateSurfaceResult(env, false, SURFACE_RESULT_EMPTY_STACK, "failed to pop surface");
    }
    return CreateSurfaceResult(env, true, SURFACE_RESULT_OK, "");
}

napi_value GetSurfaceIds(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_ERROR, "GetSurfaceIds: invalid arguments, argc=%{public}zu", argc);
        return CreateSurfaceIdArray(env, {});
    }

    int32_t renderId = 0;
    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "GetSurfaceIds: failed to get renderId from napi value");
        return CreateSurfaceIdArray(env, {});
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "GetSurfaceIds: RenderSlot not found for renderId=%{public}d", renderId);
        return CreateSurfaceIdArray(env, {});
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        LOG_A2UI(LOG_ERROR, "GetSurfaceIds: SurfaceManager is nullptr for renderId=%{public}d", renderId);
        return CreateSurfaceIdArray(env, {});
    }

    return CreateSurfaceIdArray(env, surfaceManager->GetSurfaceIds());
}

napi_value GetLatestSurfaceId(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_ERROR, "GetLatestSurfaceId: invalid arguments, argc=%{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = NapiBridge::GetInstance().Provider().GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "GetLatestSurfaceId: failed to get renderId from napi value");
        return nullptr;
    }

    LOG_A2UI(LOG_INFO, "GetLatestSurfaceId: renderId=%{public}d", renderId);

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "GetLatestSurfaceId: RenderSlot not found for renderId=%{public}d", renderId);
        return nullptr;
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        LOG_A2UI(LOG_ERROR, "GetLatestSurfaceId: SurfaceManager is nullptr for renderId=%{public}d", renderId);
        return nullptr;
    }

    std::string surfaceId = surfaceManager->GetLatestSurfaceId();
    LOG_A2UI(LOG_INFO, "GetLatestSurfaceId: Latest surface ID='%{public}s' for renderId=%{public}d", surfaceId.c_str(),
        renderId);

    // Convert string to napi_value
    napi_value result;
    status = NapiBridge::GetInstance().Provider().CreateStringUtf8(env, surfaceId.c_str(), surfaceId.length(), &result);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "GetLatestSurfaceId: failed to create napi string");
        return nullptr;
    }

    return result;
}

#ifdef ENABLE_EXPRESSION_ENGINE
namespace {
void ApplyThemeContextFromRenderId(EvaluationContext& context, int32_t renderId)
{
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        return;
    }
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return;
    }
    context.SetThemeContext(&surfaceManager->GetThemeContext());
}

void ApplySurfaceDataModel(EvaluationContext& context, const std::string& surfaceId)
{
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(context.GetRenderId());
    if (renderSlot == nullptr) {
        return;
    }
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return;
    }
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface(surfaceId);
    if (surfaceSlot == nullptr || surfaceSlot->GetBindingEngine() == nullptr) {
        return;
    }
    auto dataModel = surfaceSlot->GetBindingEngine()->GetOrCreateDataModel(surfaceId);
    context.SetDataModel(dataModel.get());
}
} // namespace

napi_value EvaluateExpression(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 3;
    napi_value args[3] = { nullptr, nullptr, nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_WARN, "EvaluateExpression: invalid arguments, argc=%{public}zu", argc);
        return CreateExpressionResult(env, EvalResult::Undefined());
    }

    napi_valuetype valueType = napi_undefined;
    if (napi.Typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        LOG_A2UI(LOG_WARN, "EvaluateExpression: expression argument is not a string");
        return CreateExpressionResult(env, EvalResult::Undefined());
    }

    size_t jsStringLength = 0;
    napi.GetValueStringUtf8(env, args[0], nullptr, 0, &jsStringLength);
    if (jsStringLength > 2048) {
        LOG_A2UI(LOG_WARN, "EvaluateExpression: expression too long, length=%{public}zu", jsStringLength);
        return CreateExpressionResult(env, EvalResult::Undefined());
    }

    std::string expression = NapiGetStringValue(env, args[0]);
    EvaluationContext context;

    if (argc >= 2 && args[1] != nullptr) {
        napi_valuetype argType = napi_undefined;
        napi.Typeof(env, args[1], &argType);
        if (argType == napi_number) {
            int32_t renderId = 0;
            napi.GetValueInt32(env, args[1], &renderId);
            context.SetRenderId(renderId);
            ApplyThemeContextFromRenderId(context, renderId);
        }
    }

    if (argc >= 3 && args[2] != nullptr) {
        napi_valuetype argType = napi_undefined;
        napi.Typeof(env, args[2], &argType);
        if (argType == napi_string) {
            std::string surfaceId = NapiGetStringValue(env, args[2]);
            context.SetSurfaceId(surfaceId);
            ApplySurfaceDataModel(context, surfaceId);
        }
    }

    JsonValue evaluatedValue = ExpressionEngine::GetInstance().EvaluateAsJsonValue(expression, context, true);
    EvalResult evalResult = EvalResult::FromJson(evaluatedValue);
    return CreateExpressionResult(env, evalResult);
}
#endif

napi_value RegisterCreateCustomComponent(napi_env env, napi_callback_info info)
{
    NapiResourceManager* manager = GetNapiResourceManager();
    if (manager == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterCreateCustomComponent: NapiResourceManager not available");
        return nullptr;
    }
    return manager->RegisterCreateCustomComponent(env, info);
}

napi_value RegisterInvokeLocalFunction(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || !NapiIsFunctionValue(env, args[0])) {
        LOG_A2UI(LOG_ERROR, "RegisterInvokeLocalFunction: invalid args, argc=%{public}zu", argc);
        return nullptr;
    }

    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env, args[0]);
    return nullptr;
}

napi_value RegisterDispatchAction(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || !NapiIsFunctionValue(env, args[0])) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchAction: invalid args, argc=%{public}zu", argc);
        return nullptr;
    }

    ActionDispatchBridge::GetInstance().RegisterDispatchAction(env, args[0]);
    return nullptr;
}

napi_value RegisterDispatchRuntimeError(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || !NapiIsFunctionValue(env, args[0])) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchRuntimeError: invalid args, argc=%{public}zu", argc);
        return nullptr;
    }

    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env, args[0]);
    return nullptr;
}

napi_value RegisterDispatchSchemaWarning(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || !NapiIsFunctionValue(env, args[0])) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchSchemaWarning: invalid args, argc=%{public}zu", argc);
        return nullptr;
    }

    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(env, args[0]);
    return nullptr;
}

napi_value RegisterUpdateCustomComponent(napi_env env, napi_callback_info info)
{
    NapiResourceManager* manager = GetNapiResourceManager();
    if (manager == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterUpdateCustomComponent: NapiResourceManager not available");
        return nullptr;
    }
    return manager->RegisterUpdateCustomComponent(env, info);
}

napi_value SetFontSizeScale(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 2) {
        LOG_A2UI(LOG_ERROR, "SetFontSizeScale: expected 2 args (renderId, scale), got %{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = napi.GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetFontSizeScale: failed to get renderId");
        return nullptr;
    }

    double scale = 1.0;
    status = napi.GetValueDouble(env, args[1], &scale);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetFontSizeScale: failed to get scale");
        return nullptr;
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "SetFontSizeScale: RenderSlot not found, renderId=%{public}d", renderId);
        return nullptr;
    }

    float scaleF = static_cast<float>(scale);
    renderSlot->SetFontSizeScale(scaleF);

    LOG_A2UI(LOG_INFO, "SetFontSizeScale: renderId=%{public}d, scale=%{public}f", renderId, scale);
    return nullptr;
}

napi_value SetApiVersion(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 2) {
        LOG_A2UI(LOG_ERROR, "SetApiVersion: expected 2 args (renderId, apiVersion), got %{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = napi.GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetApiVersion: failed to get renderId");
        return nullptr;
    }

    int32_t apiVersion = 0;
    status = napi.GetValueInt32(env, args[1], &apiVersion);
    if (status != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetApiVersion: failed to get apiVersion");
        return nullptr;
    }
    SystemProperties::GetInstance().SetApiVersion(apiVersion);

    LOG_A2UI(LOG_INFO, "SetApiVersion: renderId=%{public}d, apiVersion=%{public}d", renderId, apiVersion);
    return nullptr;
}

napi_value DispatchCustomComponentAction(napi_env env, napi_callback_info info)
{
    CustomComponentActionInput input;
    if (!ParseCustomComponentActionInput(env, info, input)) {
        return nullptr;
    }

    CustomComponentActionTarget target;
    if (!FindCustomComponentActionTarget(input, target)) {
        return nullptr;
    }

    JsonValue extraContext = ParseCustomComponentActionContext(input.contextJson);
    SyncCustomComponentCheckedState(target, extraContext);
    SyncCustomComponentRuntimeValues(target.customComponent, extraContext);
    target.customComponent->DispatchEvent(input.eventName, extraContext);
    return nullptr;
}

napi_value ValidateCustomComponentChecks(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 4;
    napi_value args[4] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 4) {
        return nullptr;
    }

    int32_t renderId = 0;
    napi_status status = napi.GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        return nullptr;
    }

    std::string surfaceId = NapiGetStringValue(env, args[1]);
    std::string componentId = NapiGetStringValue(env, args[2]);
    std::string valueJson = NapiGetStringValue(env, args[3]);

    napi_value result = nullptr;
    napi.CreateObject(env, &result);
    napi_value validValue = nullptr;
    napi.GetBoolean(env, true, &validValue);
    napi.SetNamedProperty(env, result, "valid", validValue);
    napi_value messageValue = nullptr;
    napi.CreateStringUtf8(env, "", NAPI_AUTO_LENGTH, &messageValue);
    napi.SetNamedProperty(env, result, "message", messageValue);

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        return result;
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return result;
    }

    SurfaceSlot* surface = surfaceManager->FindSurface(surfaceId);
    if (surface == nullptr) {
        return result;
    }

    std::shared_ptr<Component> component = surface->FindComponentById(componentId);
    std::shared_ptr<CustomComponent> customComponent = std::dynamic_pointer_cast<CustomComponent>(component);
    if (customComponent == nullptr) {
        return result;
    }

    std::string message;
    bool valid = customComponent->ValidateChecks(valueJson, &message);
    napi.GetBoolean(env, valid, &validValue);
    napi.SetNamedProperty(env, result, "valid", validValue);
    napi.CreateStringUtf8(env, message.c_str(), NAPI_AUTO_LENGTH, &messageValue);
    napi.SetNamedProperty(env, result, "message", messageValue);
    return result;
}

napi_value ResolveCustomComponentDynamicValue(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 4;
    napi_value args[4] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 4 || args[0] == nullptr || args[1] == nullptr || args[2] == nullptr || args[3] == nullptr) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ResolveCustomComponentDynamicValue: invalid arguments"));
    }

    double handleValue = 0.0;
    if (napi.GetValueDouble(env, args[0], &handleValue) != napi_ok || !std::isfinite(handleValue) ||
        handleValue <= 0.0) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ResolveCustomComponentDynamicValue: invalid customComponentHandle"));
    }

    uintptr_t handle = static_cast<uintptr_t>(handleValue);
    CustomComponent* customComponent = CustomComponent::FindByHandle(handle);
    if (customComponent == nullptr) {
        return CreateProcessResultValue(
            env, Fail("CUSTOM_COMPONENT_NOT_FOUND", "ResolveCustomComponentDynamicValue: component handle not found"));
    }

    std::string propertyName = NapiGetStringValue(env, args[1]);
    std::string descriptorJson = NapiGetStringValue(env, args[2]);
    if (propertyName.empty() || descriptorJson.empty()) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ResolveCustomComponentDynamicValue: propertyName or descriptorJson empty"));
    }
    if (!NapiIsFunctionValue(env, args[3])) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ResolveCustomComponentDynamicValue: callback is not a function"));
    }

    std::unique_ptr<JsonAdapter> descriptorAdapter = JsonAdapter::Parse(descriptorJson);
    if (descriptorAdapter == nullptr) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ResolveCustomComponentDynamicValue: descriptorJson parse failed"));
    }

    std::string errorMessage;
    if (!customComponent->RegisterDynamicValueCallback(
            propertyName, descriptorAdapter->GetRoot(), env, args[3], &errorMessage)) {
        return CreateProcessResultValue(env,
            Fail("DYNAMIC_VALUE_RESOLVE_FAILED",
                errorMessage.empty() ? "ResolveCustomComponentDynamicValue: register callback failed" : errorMessage));
    }

    return CreateProcessResultValue(env, Ok());
}

napi_value ClearCustomComponentDynamicValue(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 2 || args[0] == nullptr || args[1] == nullptr) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ClearCustomComponentDynamicValue: invalid arguments"));
    }

    double handleValue = 0.0;
    if (napi.GetValueDouble(env, args[0], &handleValue) != napi_ok || !std::isfinite(handleValue) ||
        handleValue <= 0.0) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ClearCustomComponentDynamicValue: invalid customComponentHandle"));
    }

    CustomComponent* customComponent = CustomComponent::FindByHandle(static_cast<uintptr_t>(handleValue));
    if (customComponent == nullptr) {
        return CreateProcessResultValue(
            env, Fail("CUSTOM_COMPONENT_NOT_FOUND", "ClearCustomComponentDynamicValue: component handle not found"));
    }

    std::string propertyName = NapiGetStringValue(env, args[1]);
    if (propertyName.empty()) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "ClearCustomComponentDynamicValue: propertyName is empty"));
    }

    customComponent->ClearDynamicValueCallback(propertyName);
    return CreateProcessResultValue(env, Ok());
}

napi_value EvaluateDynamicValue(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 5;
    napi_value args[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 5 || args[0] == nullptr || args[1] == nullptr || args[2] == nullptr || args[3] == nullptr ||
        args[4] == nullptr) {
        return CreateProcessResultValue(env, Fail("INVALID_ARGUMENT", "EvaluateDynamicValue: invalid arguments"));
    }

    int32_t renderId = 0;
    if (napi.GetValueInt32(env, args[0], &renderId) != napi_ok) {
        return CreateProcessResultValue(env, Fail("INVALID_ARGUMENT", "EvaluateDynamicValue: invalid renderId"));
    }

    std::string surfaceId = NapiGetStringValue(env, args[1]);
    std::string componentId = NapiGetStringValue(env, args[2]);
    std::string descriptorJson = NapiGetStringValue(env, args[3]);
    if (surfaceId.empty() || descriptorJson.empty()) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "EvaluateDynamicValue: surfaceId or descriptorJson empty"));
    }

    bool allowExpression = false;
    if (napi.GetValueBool(env, args[4], &allowExpression) != napi_ok) {
        return CreateProcessResultValue(env, Fail("INVALID_ARGUMENT", "EvaluateDynamicValue: invalid allowExpression"));
    }

    std::unique_ptr<JsonAdapter> descriptorAdapter = JsonAdapter::Parse(descriptorJson);
    if (descriptorAdapter == nullptr) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "EvaluateDynamicValue: descriptorJson parse failed"));
    }

    DynamicResolveContext context;
    context.renderId = renderId;
    context.surfaceId = surfaceId;
    context.componentId = componentId;
    context.allowExpression = allowExpression;
    ResolvedValue resolved = DynamicValueResolver::Resolve(descriptorAdapter->GetRoot(), context);
    if (!resolved.success || !resolved.value.IsValid()) {
        return CreateProcessResultValue(
            env, Fail("DYNAMIC_VALUE_RESOLVE_FAILED",
                     resolved.errorMessage.empty() ? "EvaluateDynamicValue: resolve failed" : resolved.errorMessage));
    }

    napi_value output = CreateProcessResultValue(env, Ok());
    napi_value value = JsonValueToNapiValue(env, resolved.value);
    if (value == nullptr || napi.SetNamedProperty(env, output, "value", value) != napi_ok) {
        return CreateProcessResultValue(
            env, Fail("DYNAMIC_VALUE_RESOLVE_FAILED", "EvaluateDynamicValue: value conversion failed"));
    }
    return output;
}

napi_value SyncComponentBoundDataModel(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 5;
    napi_value args[5] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 5 || args[0] == nullptr || args[1] == nullptr || args[2] == nullptr || args[3] == nullptr ||
        args[4] == nullptr) {
        return CreateProcessResultValue(
            env, Fail("INVALID_ARGUMENT", "SyncComponentBoundDataModel: invalid arguments"));
    }

    int32_t renderId = 0;
    napi_status status = napi.GetValueInt32(env, args[0], &renderId);
    if (status != napi_ok) {
        return CreateProcessResultValue(env, Fail("INVALID_ARGUMENT", "SyncComponentBoundDataModel: invalid renderId"));
    }

    std::string surfaceId = NapiGetStringValue(env, args[1]);
    std::string componentId = NapiGetStringValue(env, args[2]);
    std::string propertyName = NapiGetStringValue(env, args[3]);
    std::string valueJson = NapiGetStringValue(env, args[4]);

    ProcessResult result =
        SyncComponentBoundDataModelInternal(renderId, surfaceId, componentId, propertyName, valueJson);
    return CreateProcessResultValue(env, result);
}

napi_value RegisterLocale(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterLocale: invalid args");
        return nullptr;
    }

    napi_valuetype type = napi_undefined;
    napi.Typeof(env, args[0], &type);
    if (type == napi_function) {
        LOG_A2UI(LOG_INFO, "RegisterLocale: registering locale provider callback");
        PluralLocaleManager::GetInstance().RegisterLocaleProvider(env, args[0]);
    } else {
        std::string locale = NapiGetStringValue(env, args[0]);
        PluralLocaleManager::GetInstance().SetLocale(locale);
    }
    return nullptr;
}

napi_value UpdateThemeMode(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 2;
    napi_value args[2] = { nullptr, nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2 || args[0] == nullptr || args[1] == nullptr) {
        LOG_A2UI(LOG_ERROR, "UpdateThemeMode: invalid arguments, argc=%{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    if (napi.GetValueInt32(env, args[0], &renderId) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "UpdateThemeMode: failed to parse renderId");
        return nullptr;
    }

    int32_t mode = 0;
    if (napi.GetValueInt32(env, args[1], &mode) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "UpdateThemeMode: failed to parse mode");
        return nullptr;
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "UpdateThemeMode: RenderSlot not found, renderId=%{public}d", renderId);
        return nullptr;
    }

    auto surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        LOG_A2UI(LOG_WARN, "UpdateThemeMode: SurfaceManager is null, renderId=%{public}d", renderId);
        return nullptr;
    }

    ThemeMode themeMode = (mode == 1) ? ThemeMode::DARK : ThemeMode::LIGHT;
    surfaceManager->UpdateThemeMode(themeMode);
    LOG_A2UI(LOG_INFO, "UpdateThemeMode: Updated theme mode to %{public}d for renderId=%{public}d",
        static_cast<int32_t>(themeMode), renderId);
    return nullptr;
}

Breakpoint ResolveBreakpointValue(int32_t breakpoint)
{
    switch (breakpoint) {
        case 0:
            return Breakpoint::XS;
        case 1:
            return Breakpoint::SM;
        case 2:
            return Breakpoint::MD;
        case 3:
            return Breakpoint::LG;
        case 4:
            return Breakpoint::XL;
        default:
            LOG_A2UI(LOG_WARN, "UpdateBreakpoint: Invalid breakpoint value=%{public}d, using SM", breakpoint);
            return Breakpoint::SM;
    }
}

napi_value UpdateBreakpoint(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    size_t argc = 2;
    napi_value args[2] = { nullptr, nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2 || args[0] == nullptr || args[1] == nullptr) {
        LOG_A2UI(LOG_ERROR, "UpdateBreakpoint: invalid arguments, argc=%{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    if (napi.GetValueInt32(env, args[0], &renderId) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "UpdateBreakpoint: failed to parse renderId");
        return nullptr;
    }

    int32_t breakpoint = 0;
    if (napi.GetValueInt32(env, args[1], &breakpoint) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "UpdateBreakpoint: failed to parse breakpoint");
        return nullptr;
    }

    auto& renderManager = RenderManager::GetInstance();
    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "UpdateBreakpoint: RenderSlot not found, renderId=%{public}d", renderId);
        return nullptr;
    }

    auto surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        LOG_A2UI(LOG_WARN, "UpdateBreakpoint: SurfaceManager is null, renderId=%{public}d", renderId);
        return nullptr;
    }

    surfaceManager->UpdateBreakpoint(ResolveBreakpointValue(breakpoint));
    LOG_A2UI(
        LOG_INFO, "UpdateBreakpoint: Updated breakpoint to %{public}d for renderId=%{public}d", breakpoint, renderId);
    return nullptr;
}

napi_value RegisterCrossLanguageAttributeCallback(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || !NapiIsFunctionValue(env, args[0])) {
        LOG_A2UI(LOG_ERROR, "RegisterCrossLanguageAttributeCallback: invalid args, argc=%{public}zu", argc);
        return nullptr;
    }

    CrossLanguageAttributeBridge::GetInstance().RegisterCrossLanguageCallback(env, args[0]);
    LOG_A2UI(LOG_INFO, "RegisterCrossLanguageAttributeCallback: success");
    return nullptr;
}

napi_value SetDisplayDensity(napi_env env, napi_callback_info info)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    constexpr size_t MIN_ARGC = 2;
    constexpr size_t MAX_ARGC = 3;
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = { nullptr };
    napi.GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < MIN_ARGC || args[0] == nullptr || args[1] == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "SetDisplayDensity: expected 2 or 3 args (renderId, densityPixels, fpToVpScale), got %{public}zu", argc);
        return nullptr;
    }

    int32_t renderId = 0;
    if (napi.GetValueInt32(env, args[0], &renderId) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetDisplayDensity: failed to parse renderId");
        return nullptr;
    }

    double densityPixels = 0.0;
    if (napi.GetValueDouble(env, args[1], &densityPixels) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetDisplayDensity: failed to parse densityPixels");
        return nullptr;
    }

    double fpToVpScale = 0.0;
    if (argc >= MAX_ARGC && args[2] != nullptr && napi.GetValueDouble(env, args[2], &fpToVpScale) != napi_ok) {
        LOG_A2UI(LOG_ERROR, "SetDisplayDensity: failed to parse fpToVpScale");
        return nullptr;
    }

    DisplayDensityUtils::GetInstance().SetDisplayDensity(
        renderId, static_cast<float>(densityPixels), static_cast<float>(fpToVpScale));

    LOG_A2UI(LOG_INFO, "SetDisplayDensity: renderId=%{public}d, density=%{public}f, fpToVpScale=%{public}f", renderId,
        densityPixels, fpToVpScale);
    return nullptr;
}

} // namespace NativeModule
