/**
 * @file SubjectWorkFlowInstance.h
 * @brief Subject interface for workflow instances that publish events to observers.
 */
#ifndef SUBJECTWORKFLOWINSTANCE_H
#define SUBJECTWORKFLOWINSTANCE_H

#include <string>
#include "WorkflowMemento.h"

class EventObserver;
class WorkflowMemento;
/**
 * @class SubjectWorkFlowInstance
 * @brief Subject interface for workflow instances that publish events to observers.
 */
class SubjectWorkFlowInstance
{
public:
    virtual void attach(EventObserver* obs) = 0;
    virtual void detach(EventObserver* obs) = 0;
    virtual void notifyObservser() = 0;
    virtual void setState() = 0;
    virtual void setEvent() = 0;
    virtual void setID() = 0;
    virtual std::string getState() = 0;
    virtual std::string getEventName() = 0;
    virtual std::string getInstanceID() = 0;
    virtual WorkflowMemento* createMemento() = 0;
    virtual void restoreMemento(WorkflowMemento* m) = 0; 
    virtual ~SubjectWorkFlowInstance(){};
};



#endif