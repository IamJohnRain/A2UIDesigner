#ifndef A2UI_SURFACE_SLOT_SCHEMA_VALIDATION_H
#define A2UI_SURFACE_SLOT_SCHEMA_VALIDATION_H

#include <string>

#include "utils/JsonAdapter.h"

namespace NativeModule {

enum class SurfaceProtocolMode;

bool ValidateDescriptorIdForPreparation(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId);

void DispatchComponentSchemaWarning(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const std::string& componentType, const std::string& code, const std::string& message,
    const std::string& propertyPath);

void ValidateRequiredCreationFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId);

void ValidateRequiredStructuralFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId,
    SurfaceProtocolMode surfaceProtocolMode);

void ValidateStructuralFieldShapes(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId,
    SurfaceProtocolMode surfaceProtocolMode);

void ValidateEventHandlerFields(const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId);

} // namespace NativeModule

#endif // A2UI_SURFACE_SLOT_SCHEMA_VALIDATION_H
