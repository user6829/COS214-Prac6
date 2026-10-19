#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include "SubjectWorkFlowInstance.h"

class WorkflowInstance
{
public:
    void attach(EventObserver obs);
    void detach(EventObserver obs);
    void notifyObservser(std::string eventName);
    WorkflowMemento createMemento();
    void restoreMemento(WorkflowMemento m); 
};

#endif