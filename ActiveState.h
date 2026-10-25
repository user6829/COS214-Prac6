/**
 * @file ActiveState.h
 * @brief The header file for the concrete state, Active
 */
#ifndef ACTIVESTATE_H
#define ACTIVESTATE_H

#include "WorkItemState.h"

class LeafWorkItem;

class ActiveState: public WorkItemState {
    public:
    std::string getStateName() const override ;
    void handle(LeafWorkItem* item) override;
};

#endif