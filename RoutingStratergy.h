#ifndef ROUTINGSTRATEGY_H
#define ROUTINGSTRATEGY_H
#include <string>

class RoutingStrategy {
public:
virtual std::string determineAssignee( const context& WorkflowContext) = 0;
virtual ~RoutingStrategy = default;
};
#endif
