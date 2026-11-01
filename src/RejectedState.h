/**
 * @file RejectedState.h
 * @brief Concrete State representing a rejected work item.
 */
#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include "WorkItemState.h"

/**
 * @class RejectedState
 * @brief Concrete State representing a rejected work item.
 */
class RejectedState: public WorkItemState {
    public:
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};

#endif