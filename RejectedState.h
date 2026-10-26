/**
 * @file RejectedState.h
 * @brief The header file for the concrete state, Rejected
 */
#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include "WorkItemState.h"

class RejectedState: public WorkItemState {
    public:
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};

#endif