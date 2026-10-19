#ifndef PENDINGSTATE_H
#define PENDINGSTATE_H

#include "WorkItemState.h"

class PendingState: public WorkItemState {
    public:
    std::string getStateName() const override { 
        return "Pending";
    }
    void handleStart(LeafWorkItem* item) override;
    void handleComplete(LeafWorkItem* item) override;
    void handleEscalate(LeafWorkItem* item) override;
};

#endif