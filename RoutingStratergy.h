#ifndef ROUTINGSTRATEGY_H
#define ROUTINGSTRATEGY_H
#include <string>
class LeafWorkItem;

class RoutingStrategy {
public:
virtual std::string determineAssignee( const LeafWorkItem& WorkflowContext) = 0;
virtual ~RoutingStrategy = default;
};
#endif
