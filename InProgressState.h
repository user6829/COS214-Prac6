#ifndef INPROGRESSSTATE_H
#define INPROGRESSSTATE_H

#include "WorkItemState.h"

class InProgressState: public WorkItemState {
    public:
    std::string getStateName() const override { 
        return "InPorgress";
    }
    void handleStart(LeafWorkItem* item) override;
    void handleComplete(LeafWorkItem* item) override;
    void handleEscalate(LeafWorkItem* item) override;
};

#endif