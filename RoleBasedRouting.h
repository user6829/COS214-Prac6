#ifndef ROLEBASEDROUTING_H
#define ROLEBASEDROUTING_H
#include <string>
#include "RoutingStrategy.h"



class RoleBasedRouting : public RoutingStrategy  {
private:
std::string targetRole;

public:
std::string determineAssignee( const context& WorkflowContext) override;
~RoleBasedRouting() override = default;
};

#endif