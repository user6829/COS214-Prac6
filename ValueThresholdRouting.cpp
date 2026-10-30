#include "ValueThresholdRouting.h"

#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "LeafWorkItem.h"
#include "WorkflowContext.h"

namespace {
/// Converts text from the workflow data to a number, or throws a clear error.
double parseNumber(const std::string& text, const std::string& key) {
    if (text.empty()) {
        throw std::runtime_error("ValueThresholdRouting: no value for data key '" + key + "'");
    }
    char* end = nullptr;
    double value = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0') {
        throw std::runtime_error("ValueThresholdRouting: data key '" + key +
                                 "' is not a number: '" + text + "'");
    }
    return value;
}
} // namespace

ValueThresholdRouting::ValueThresholdRouting(const std::string& key,double limit,const std::string& lowRole,const std::string& highRole)
    : valueKey(key),thresholdAmount(limit),lowValueRole(lowRole),highValueRole(highRole)
{
    if (valueKey.empty() || lowValueRole.empty() || highValueRole.empty()) {
        throw std::invalid_argument("ValueThresholdRouting: key and roles must not be empty");
    }
}

// 3-arg convenience: delegates with key = "amount"
ValueThresholdRouting::ValueThresholdRouting(double limit,const std::string& lowRole, const std::string& highRole)
    : ValueThresholdRouting("amount", limit, lowRole, highRole)
{
}

ValueThresholdRouting::~ValueThresholdRouting() {
    // Nothing to release: the members are a double and std::strings.
}

std::string ValueThresholdRouting::determineAssignee(const LeafWorkItem& item) const {
    WorkflowContext* context = item.getContext();
    if (context == nullptr) {
        throw std::runtime_error("ValueThresholdRouting: work item has no workflow data attached");
    }
    double value = parseNumber(context->getData(valueKey), valueKey);
    return (value >= thresholdAmount) ? highValueRole : lowValueRole;
}

std::string ValueThresholdRouting::describe() const {
    std::ostringstream out;
    out << valueKey << " >= " << thresholdAmount << " -> " << highValueRole
        << ", otherwise " << lowValueRole;
    return out.str();
}
