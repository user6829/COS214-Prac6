#ifndef VALUETHRESHOLDROUTING_H
#define VALUETHRESHOLDROUTING_H
#include <string>
#include "RoutingStrategy.h"

class LeafWorkItem;
class WorkflowContext;
/**
 * @class ValueThresholdRouting
 * @brief Strategy that chooses a role based on a numeric workflow-data threshold.
 */


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
    /**
     * @brief Selects the high-value or low-value role from the instance payload.
     * @param item Work item whose attached workflow context contains the configured key.
     * @return The high-value role when the value meets the threshold; otherwise the low-value role.
     * @throws std::runtime_error If the item has no context or the value is missing/invalid.
     */
    std::string determineAssignee(const LeafWorkItem& WorkflowContext) const override;
    std::string describe() const override;
};

#endif
