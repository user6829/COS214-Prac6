/**
 * @file CompletedState.h
 * @brief The header file for the concrete state, Complete
 */
#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

class CompletedState: public WorkItemState {
    public:
    std::string getStateName() const override;
    void handle(LeafWorkItem* item) override;
};

#endif