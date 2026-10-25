#ifndef ROLEBASEDROUTING_H
#define ROLEBASEDROUTING_H

#include <string>
#include "RoutingStrategy.h"

/**
 * @brief Strategy pattern, role: ConcreteStrategy.
 *
 * Always assigns to one fixed role, whatever the workflow data says.
 */
class RoleBasedRouting : public RoutingStrategy {
public:
    /// @throws std::invalid_argument if @p targetRole_ is empty.
    explicit RoleBasedRouting(const std::string& targetRole_);

    std::string determineAssignee(const LeafWorkItem& item) const override;
    std::string describe() const override;

private:
    std::string targetRole;
};

#endif // ROLEBASEDROUTING_H
