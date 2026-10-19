#ifndef WORKITEMSTATE_H
#define WORKITEMSTATE_H

#include <string>

class LeafWorkItem;

class WorkItemState {
    public:
    virtual ~WorkItemState() = default;
    virtual std::string getStateName() const = 0;
    virtual void handleStart(LeafWorkItem* item) = 0;
    virtual void handleComplete(LeafWorkItem* item) = 0;
    virtual void handleEscalate(LeafWorkItem* item) = 0;
};



#endif