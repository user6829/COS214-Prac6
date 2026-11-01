/**
 * @file InactiveState.h
 * @brief Concrete State representing a work item that is not currently active.
 */
#ifndef INACTIVESTATE_H
#define INACTIVESTATE_H

#include "WorkItemState.h"

/**
 * @class InactiveState
 * @brief Concrete State representing a work item that is not currently active.
 */
class InactiveState: public WorkItemState {
public:
    bool canExecute() const override { return true; }
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};

#endif