#include "WorkflowComponent.h"

WorkflowComponent::WorkflowComponent(const std::string& id , const std::string& name):
    id(id),name(name){}
std::string getId() const {
    return id;
}
void WorkflowComponent::setId(const std::string& id){
    this->id=id;
}
std::string WorkflowComponent::getName() const{
    return name;
}
void WorkflowComponent::setName(const std::string& name){
    this->name=name;
}
