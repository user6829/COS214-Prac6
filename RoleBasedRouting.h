#ifndef ROLEBASEDROUTING_H
#define ROLEBASEDROUTING_H
#include <string>
#include "RoutingStrategy.h"

class LeafWorkItem;
/**
 * @class RoleBasedRouting
 * @brief Strategy that assigns a work item to a configured role.
 */

class RoleBasedRouting : public RoutingStrategy  {
private:
std::string targetRole;

public:
    explicit RoleBasedRouting(const std::string& role);
    ~RoleBasedRouting() override ;
    std::string determineAssignee(const LeafWorkItem& WorkflowContext) const override;
    std::string describe() const override;
    
};

#endif
