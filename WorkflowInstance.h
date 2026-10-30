#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

class SubjectWorkFlowInstance;

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

class WorkflowInstance : public SubjectWorkFlowInstance
{
private:
    std::string instanceID;
    std::string state;
    std::string eventName;
    std::vector<EventObserver *> observers;
    WorkflowContext *currentContext;

public:
    /**
     * @brief Add the observer to the list
     * @param obs The observer to be added
     */
    void attach(EventObserver *obs)
    {
        observers.push_back(obs);
    }

    /**
     * @brief Removes the observer from the list
     * @param obs The observer to be removed
     */
    void detach(EventObserver *obs)
    {
        observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
    }

    /**
     * @brief Notifies all the attached observers
     */
    void notifyObservser()
    {
        for (EventObserver *obs : observers)
        {
            obs->update(*this);
        }
    }

    /**
     * @brief Sets the new state then calls notify
     */
    void setState()
    {
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
        this->instanceID = currentContext->getData(std::string("ID"));
        notifyObservser();
    }

    /**
     * @brief Returns the state
     */
    std::string getState()
    {
        return state;
    }

    /**
     * @brief Returns the event name
     */
    std::string getEventName()
    {
        return eventName;
    }

    /**
     * @brief Returns the instance ID
     */
    std::string getInstanceID()
    {
        return instanceID;
    }

    /**
     * @brief Creates and returns a snapshot of the current state
     */
       WorkflowMemento *createMemento()
    {
        std::map<std::string, std::string> *stateData = new std::map<std::string, std::string>();
        
        (*stateData)["instanceID"] = this->instanceID;
        (*stateData)["state"] = this->state;
        (*stateData)["eventName"] = this->eventName;
        
        // Save the entire data payload using raw map iteration
        if (currentContext) {
            const std::map<std::string, std::string>& payload = currentContext->getAll();
            for (std::map<std::string, std::string>::const_iterator it = payload.begin(); it != payload.end(); ++it) {
                (*stateData)[it->first] = it->second;
            }
        }

        return new WorkflowMemento(stateData, "TIMESTAMP");
    }

    /**
     * @brief Restores the workflow instance state from a memento
     */
        void restoreMemento(WorkflowMemento *m)
    {
        if (m == nullptr) return;
        std::map<std::string, std::string> *stateData = m->getState();
        if (stateData != nullptr)
        {
            if (stateData->find("instanceID") != stateData->end())
                this->instanceID = (*stateData)["instanceID"];
            if (stateData->find("state") != stateData->end())
                this->state = (*stateData)["state"];
            if (stateData->find("eventName") != stateData->end())
                this->eventName = (*stateData)["eventName"];
                
                       if (currentContext) {
                for (std::map<std::string, std::string>::iterator it = stateData->begin(); it != stateData->end(); ++it) {
                    // Exclude internal memento metadata AND context routing metadata
                    if (it->first != "instanceID" && it->first != "state" && it->first != "eventName" 
                        && it->first != "State" && it->first != "Event" && it->first != "ID") 
                    {
                        currentContext->setData(it->first, it->second);
                    }
                }
            }
        }
    }
    /**
     * @brief Destructor that cleans up the currentContext pointer
     */
    ~WorkflowInstance()
    {
        if (currentContext != nullptr)
        {
            delete currentContext;
            currentContext = nullptr;
        }
    }

    WorkflowInstance(WorkflowContext *c)
        : instanceID(""), state(""), eventName(""), currentContext(c) {}

    WorkflowContext* getContext() const;
};

#endif