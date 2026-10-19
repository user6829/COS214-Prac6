#ifndef ESCALATEDSTATE_H
#define ESCALATEDSTATE_H

#include "WorkItemState.h"

class EscalatedState: public WorkItemState {
    std::string getStateName() const override {
        return "Escalated";
    }
    void handleStart(LeafWorkItem* item) override;
    void handleComplete(LeafWorkItem* item) override;
    void handleEscalate(LeafWorkItem* item) override;
};

#endif