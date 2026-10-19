#ifndef VALUETHRESHOLDROUTING_H
#define VALUETHRESHOLDROUTING_H
#include <string>
#include "RoutingStrategy.h"
class LeafWorkItem;

class ValueThresholdRouting : public RoutingStrategy  {
private:
std::string highValueRole;
std::string lowValueRole;
int thresholdAmount;

public:
std::string determineAssignee( const LeafWorkItem& WorkflowContext) override;
~ValueThresholdRouting() override = default;
};

#endif
