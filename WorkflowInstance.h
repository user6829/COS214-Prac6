#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

class SubjectWorkFlowInstance;

template <typename K, typename V>
class WorkflowContext;

class EventObserver;

#include "SubjectWorkFlowInstance.h"
#include "WorkflowContext.h"
#include "EventObserver.h"
#include "WorkflowMemento.h"
#include <vector>
#include <algorithm>
#include <string>
#include <map>

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
<<<<<<< HEAD
    std::string getEventName(){
        return eventName;
    }
    std::string getEventName();
>>>>>>> 654d34be75b3018f7ff3286b666ebb16f71b240f

    std::string getInstanceID() {
        return instaceID;
    }

    /**
    * @brief Creates and returns a snapshot of the current state
    */
    WorkflowMemento* createMemento() {
        std::map<std::string, std::string>* stateData = new std::map<std::string, std::string>();
        
        (*stateData)["instaceID"] = this->instaceID;
        (*stateData)["state"] = this->state;
        (*stateData)["eventName"] = this->eventName;

        return new WorkflowMemento(stateData, "TIMESTAMP");
    }

    /**
    * @brief Restores the workflow instance state from a memento
    */
    void restoreMemento(WorkflowMemento* m) {
        if (m == nullptr) {
            return;
        }

        std::map<std::string, std::string>* stateData = m->getState();
        if (stateData != nullptr) {
            if (stateData->find("instaceID") != stateData->end()) {
                this->instaceID = (*stateData)["instaceID"];
            }
            if (stateData->find("state") != stateData->end()) {
                this->state = (*stateData)["state"];
            }
            if (stateData->find("eventName") != stateData->end()) {
                this->eventName = (*stateData)["eventName"];
            }
        }
    }

    /**
    * @brief Destructor that cleans up the currentContext pointer
    */
    ~WorkflowInstance() {
        if (currentContext != nullptr) {
            delete currentContext;
            currentContext = nullptr;
        }
    }
};

#endif