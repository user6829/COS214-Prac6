/**
 * @file InactiveState.h
 * @brief The header file for the concrete state, Inactive
 */
#ifndef INACTIVESTATE_H
#define INACTIVESTATE_H

#include "WorkItemState.h"

class InactiveState: public WorkItemState {
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};

#endif