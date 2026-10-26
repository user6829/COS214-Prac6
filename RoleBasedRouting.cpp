#include "RoleBasedRouting.h"

#include <stdexcept>

RoleBasedRouting::RoleBasedRouting(const std::string& role)
    : targetRole(role) {
    if (targetRole.empty()) {
        throw std::invalid_argument("RoleBasedRouting: role must not be empty");
    }
}

RoleBasedRouting::~RoleBasedRouting() {
    // Nothing to release: the only member is a std::string.
}

std::string RoleBasedRouting::determineAssignee(const LeafWorkItem&) const {
    return targetRole;
}

std::string RoleBasedRouting::describe() const {
    return "always " + targetRole;
}
