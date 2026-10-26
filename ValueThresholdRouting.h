#ifndef VALUETHRESHOLDROUTING_H
#define VALUETHRESHOLDROUTING_H

#include <string>
#include "RoutingStrategy.h"

/**
 * @brief Strategy pattern, role: ConcreteStrategy.
 *
 * Routes by comparing one numeric value in the workflow data with a threshold:
 * value >= threshold goes to the high-value role, anything lower to the
 * low-value role.
 *
 * Which data key is read is a constructor argument (for example "amount" or
 * "score"), so the class itself holds no knowledge of any industry.
 */
class ValueThresholdRouting : public RoutingStrategy {
private:
    std::string valueKey;
    double threshold;
    std::string lowValueRole;
    std::string highValueRole;

public:
    /// @throws std::invalid_argument if the key or either role is empty.
    ValueThresholdRouting(const std::string& key, double limit,
                          const std::string& lowRole,
                          const std::string& highRole);
    ~ValueThresholdRouting() override;

    /// @throws std::runtime_error if the item has no data attached, or the key
    ///         is missing or not a number.
    std::string determineAssignee(const LeafWorkItem& item) const override;
    std::string describe() const override;
};

#endif // VALUETHRESHOLDROUTING_H
