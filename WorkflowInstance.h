#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include "SubjectWorkFlowInstance.h"
#include "WorkflowContext.h"
#include "EventObserver.h"
#include <vector>
#include <algorithm>

/**
 * @brief The instance of a running workflow
 */

template <typename T>
class WorkflowInstance : public SubjectWorkFlowInstance
{
private:
    std::string instaceID;
    std::string state;
    std::string eventName;
    std::vector<EventObserver*> observers;
    WorkflowContext<std::string, T>* currentContext;
public:
    /**
    * @brief Add the observer to the list
    * @param obs The observer to be added
    */
    void attach(EventObserver* obs){
        observers.push_back(obs);
    }

    /**
    * @brief Removes the observer from the list
    * @param obs The observer to be removed
    */
    void detach(EventObserver* obs){
        observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
    }

    /**
    * @brief Notifies all the attached observers
    */
    void notifyObservser(){
        for (EventObserver* obs : observers) {
            obs->update(*this);
        }
    }

    /**
    * @brief Sets the new state then calls notify
    */
    void setState(){
        this->state = currentContext->getData(std::string("State"));
        notifyObservser();
    }

    /**
    * @brief Sets the event name then calls notify
    */
    void setEvent()
    {
        this->state = currentContext->getData(std::string("Event"));
        notifyObservser();
    }

    /**
    * @brief Sets the ID of the event then calls notify
    */
    void setID()
    {
        this->state = currentContext->getData(std::string("ID"));
        notifyObservser();
    }

    /**
    * @brief Returns the state
    */
    std::string getState(){
        return state;
    } 

    /**
    * @brief Returns the event name
    */
    std::string getEventName(){
        return eventName;
    }

    /**
    * @brief Returns the ID of the event
    */
    std::string getInstanceID() {
        return instaceID;
    }

    /**
    * @brief Cleans up the workflowcontext
    */
    WorkflowMemento createMemento(){
        delete currentContext;
    }
    void restoreMemento(WorkflowMemento m); 
    ~WorkflowInstance();
};

#endif