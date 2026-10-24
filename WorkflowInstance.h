#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include "SubjectWorkFlowInstance.h"
#include "WorkflowContext.h"
#include "EventObserver.h"
#include <vector>
#include <algorithm>

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
    void attach(EventObserver* obs){
        observers.push_back(obs);
    }

    void detach(EventObserver* obs){
        observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
    }

    void notifyObservser(){
        for (EventObserver* obs : observers) {
            obs->update(*this);
        }
    }

    void setState(){
        this->state = currentContext->getData(std::string("State"));
        notifyObservser();
    }

    void setEvent();
    {
        this->state = currentContext->getData(std::string("Event"));
        notifyObservser();
    }

    void setID()
    {
        this->state = currentContext->getData(std::string("ID"));
        notifyObservser();
    }
    std::string getState(){
        return state;
    } 

    std::string getEventName(){
        return eventName;
    }

    std::string getInstanceID() {
        return instaceID;
    }
    WorkflowMemento createMemento(){
        delete currentContext;
    }
    void restoreMemento(WorkflowMemento m); 
    ~WorkflowInstance();
};

#endif