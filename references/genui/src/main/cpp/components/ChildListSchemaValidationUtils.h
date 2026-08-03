#ifndef A2UI_CHILD_LIST_SCHEMA_VALIDATION_UTILS_H
#define A2UI_CHILD_LIST_SCHEMA_VALIDATION_UTILS_H

#include <string>
#include <vector>

#include "utils/JsonAdapter.h"

namespace NativeModule {

enum class ChildListEmptyArrayPolicy { ALLOW = 0, WARN_INVALID_VALUE };

struct SchemaValidationIssue {
    std::string code;
    std::string message;
    std::string propertyPath;
};

std::vector<SchemaValidationIssue> ValidateChildListSchema(
    const JsonValue& descriptor, const std::string& propertyName, ChildListEmptyArrayPolicy emptyArrayPolicy);

} // namespace NativeModule

#endif // A2UI_CHILD_LIST_SCHEMA_VALIDATION_UTILS_H
