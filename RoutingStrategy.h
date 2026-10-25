#ifndef ROUTINGSTRATEGY_H
#define ROUTINGSTRATEGY_H

#include <string>

class LeafWorkItem;

/**
 * @brief Strategy pattern, role: Strategy (abstract).
 *
 * Decides WHO should receive a work item. Organisations make this decision
 * differently (a fixed role, a value in the workflow data, current workload),
 * so each rule is its own class behind this one interface.
 *
 * Participants in this design:
 *  - Strategy         : RoutingStrategy
 *  - ConcreteStrategy : RoleBasedRouting, ValueThresholdRouting, WorkloadBasedRouting
 *  - Context          : LeafWorkItem (holds a RoutingStrategy* and delegates to it)
 *  - Client           : ConcreteWorkflowBuilder / the scenario factory, which
 *                       chooses a strategy and hands it to the work item
 *
 * The strategy receives the work item and reads any data it needs through the
 * item, so a strategy keeps no state of its own.
 *
 * The result is an assignee identifier: a role name or a participant id. The
 * engine resolves it to a real participant.
 *
 * Ownership: a work item does NOT own its strategy. Whoever creates the
 * strategies (the scenario factory) keeps them alive and deletes them.
 * Strategies never change after construction, so one object may be shared by
 * many work items and many workflow instances without leaking state between them.
 */
class RoutingStrategy {
public:
    virtual ~RoutingStrategy() {};

    /**
     * @brief Chooses the assignee for @p item using the instance's data.
     * @param item the work item being routed. Rules that need workflow data read it
     *             through item.getContext().
     * @throws std::runtime_error if data the rule needs is missing or invalid.
     */
    virtual std::string determineAssignee(const LeafWorkItem& item) const = 0;

    /// Human-readable description of the rule, for history and the user interface.
    virtual std::string describe() const = 0;
};

#endif // ROUTINGSTRATEGY_H
