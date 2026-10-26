#include "WorkflowInstance.h"
#include "EventObserver.h"

WorkflowInstance::WorkflowInstance(WorkflowContext *c)
{
    this->currentContext = c;
}

void WorkflowInstance::attach(EventObserver *obs)
{
    observers.push_back(obs);
}

void WorkflowInstance::detach(EventObserver* obs){
    observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
}

void WorkflowInstance::notifyObservser(){
    for (EventObserver* obs : observers) {
        obs->update(*this);
    }
}

void WorkflowInstance::setState(){
    this->state = currentContext->getData(std::string("State"));
    notifyObservser();
}

void WorkflowInstance::setEvent()
{
    this->eventName = currentContext->getData(std::string("Event"));
    notifyObservser();
}

void WorkflowInstance::setID()
{
    this->instanceID = currentContext->getData(std::string("ID"));
    notifyObservser();
}

std::string WorkflowInstance::getState(){
    return state;
} 

std::string WorkflowInstance::getEventName(){
    return eventName;
}

std::string WorkflowInstance::getInstanceID() {
    return instanceID;
}

WorkflowInstance::~WorkflowInstance()
{
    delete currentContext;
}
