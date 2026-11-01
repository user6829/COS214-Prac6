/**
 * @file CompletedState.h
 * @brief Concrete State representing a work item that has completed.
 */
#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

/**
 * @class CompletedState
 * @brief Concrete State representing a work item that has completed.
 */
class CompletedState: public WorkItemState {
    public:
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};

#endif