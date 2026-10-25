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
public:
    /// @throws std::invalid_argument if the key or either role is empty.
    ValueThresholdRouting(const std::string& valueKey, double threshold,
                          const std::string& lowValueRole,
                          const std::string& highValueRole);

    /// @throws std::runtime_error if the key is missing or not a number.
    std::string determineAssignee(const LeafWorkItem& item) const override;
    std::string describe() const override;

private:
    std::string valueKey_;
    double threshold_;
    std::string lowValueRole_;
    std::string highValueRole_;
};

#endif // VALUETHRESHOLDROUTING_H
