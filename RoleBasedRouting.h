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
private:
    std::string targetRole;

public:
    /// @throws std::invalid_argument if @p role is empty.
    explicit RoleBasedRouting(const std::string& role);
    ~RoleBasedRouting() override;

    std::string determineAssignee(const LeafWorkItem& item) const override;
    std::string describe() const override;
};

#endif // ROLEBASEDROUTING_H
