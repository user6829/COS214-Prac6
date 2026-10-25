#include "RoleBasedRouting.h"

#include <stdexcept>

RoleBasedRouting::RoleBasedRouting(const std::string& targetRole_)
    : targetRole(targetRole_) {
    if (targetRole_.empty()) {
        throw std::invalid_argument("RoleBasedRouting: role must not be empty");
    }
}

std::string RoleBasedRouting::determineAssignee(const LeafWorkItem&) const {
    return targetRole;
}

std::string RoleBasedRouting::describe() const {
    return "always " + targetRole;
}
