#ifndef VALUETHRESHOLDROUTING_H
#define VALUETHRESHOLDROUTING_H
#include <string>
#include "RoutingStrategy.h"

class LeafWorkItem;

class ValueThresholdRouting : public RoutingStrategy  {
private:
std::string highValueRole;
std::string lowValueRole;
double thresholdAmount;
std::string valueKey;

public:
    ValueThresholdRouting(const std::string& key,double limit,const std::string& lowRole,const std::string& highRole);
    ValueThresholdRouting(double limit,const std::string& lowRole,const std::string& highRole);
    ~ValueThresholdRouting() override;
    std::string determineAssignee(const LeafWorkItem& WorkflowContext) const override;
    std::string describe() const override;
};

#endif
