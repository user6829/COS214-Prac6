#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include "SubjectWorkFlowInstance.h"
#include "WorkflowContext.h"
#include <vector>
#include <algorithm>

/**
 * @brief The instance of a running workflow
 */

class WorkflowInstance : public SubjectWorkFlowInstance
{
private:
    std::string instaceID;
    std::string state;
    std::string eventName;
    std::vector<EventObserver*> observers;
    WorkflowContext* currentContext;
public:
    /**
    * @brief Assign the workflow context
    * @param c the context to be assigned
    */
    WorkflowInstance(WorkflowContext* c);
    /**
    * @brief Add the observer to the list
    * @param obs The observer to be added
    */
    void attach(EventObserver* obs);

    /**
    * @brief Removes the observer from the list
    * @param obs The observer to be removed
    */
    void detach(EventObserver* obs);

    /**
    * @brief Notifies all the attached observers
    */
    void notifyObservser();

    /**
    * @brief Sets the new state then calls notify
    */
    void setState();

    /**
    * @brief Sets the event name then calls notify
    */
    void setEvent();

    /**
    * @brief Sets the ID of the event then calls notify
    */
    void setID();

    /**
    * @brief Returns the state
    */
    std::string getState();

    /**
    * @brief Returns the event name
    */
    std::string getEventName();

    /**
    * @brief Returns the ID of the event
    */
    std::string getInstanceID();

    /**
    * @brief Cleans up the workflowcontext
    */
    WorkflowMemento createMemento();


    void restoreMemento(WorkflowMemento m); 
    ~WorkflowInstance();
};

#endif