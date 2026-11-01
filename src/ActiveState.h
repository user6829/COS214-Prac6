/**
 * @file ActiveState.h
 * @brief Concrete State implementation for a work item in progress or awaiting a decision.
 */
#ifndef ACTIVESTATE_H
#define ACTIVESTATE_H

#include "WorkItemState.h"

class LeafWorkItem;

/**
 * @class ActiveState
 * @brief Concrete State implementation for a work item in progress or awaiting a decision.
 */
class ActiveState: public WorkItemState {
    public:
    bool canApprove() const override { return true; }
    std::string getStateName() const override ;
    void handle(LeafWorkItem* item) override;
};

#endif