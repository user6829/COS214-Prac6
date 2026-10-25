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

ValueThresholdRouting::ValueThresholdRouting(const std::string& valueKey, double threshold,
                                             const std::string& lowValueRole,
                                             const std::string& highValueRole)
    : valueKey_(valueKey), threshold_(threshold),
      lowValueRole_(lowValueRole), highValueRole_(highValueRole) {
    if (valueKey_.empty() || lowValueRole_.empty() || highValueRole_.empty()) {
        throw std::invalid_argument("ValueThresholdRouting: key and roles must not be empty");
    }
}

std::string ValueThresholdRouting::determineAssignee(const LeafWorkItem& item) const {
    const WorkflowContext* context = item.getContext();
    if (context == nullptr) {
        throw std::runtime_error("ValueThresholdRouting: work item has no workflow data attached");
    }
    double value = parseNumber(context->getData(valueKey_), valueKey_);
    return (value >= threshold_) ? highValueRole_ : lowValueRole_;
}

std::string ValueThresholdRouting::describe() const {
    std::ostringstream out;
    out << valueKey_ << " >= " << threshold_ << " -> " << highValueRole_
        << ", otherwise " << lowValueRole_;
    return out.str();
}
