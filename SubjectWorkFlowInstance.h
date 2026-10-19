#ifndef SUBJECTWORKFLOWINSTANCE_H
#define SUBJECTWORKFLOWINSTANCE_H

#include <string>
#include <vector>

class EventObserver;
class WorkflowContext;

class SubjectWorkFlowInstance
{
private:
    std::string instaceID;
    std::vector<EventObserver> observsers;
    WorkflowContext currentContext;
public:
    virtual void attach(EventObserver obs) = 0;
    virtual void detach(EventObserver obs) = 0;
    virtual void notifyObservser(std::string eventName) = 0;
    virtual WorkflowMemento createMemento() = 0;
    virtual void restoreMemento(WorkflowMemento m) = 0; 
};



#endif