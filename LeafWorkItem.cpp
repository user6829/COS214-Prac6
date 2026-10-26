#include "LeafWorkItem.h"
#include "WorkflowComponent.h"
#include "WorkItemState.h"
#include "RoutingStrategy.h"
#include "WorkflowVisitor.h"
#include "ApprovalHandler.h"

LeafWorkItem::LeafWorkItem(const std::string& id, const std::string& name ): WorkflowComponent(id,name){}
void LeafWorkItem::execute(WorkflowContext* context) override{
    if(routingStrategy){
        determineAssignee();
    }
    if(approvalHandler){
        approvalHandler->processRequest(context);
    }
    if(currentState){
        currentState->handle(this);
    }
}

void LeafWorkItem::accept(WorkflowVisitor* visitor) override{
    if(visitor){
        visitor->visitWorkItem(this);
    }
}

WorkItemState* LeafWorkItem::getCurrentState() const {
    return currentState;
}
void LeafWorkItem::setState(WorkItemState* state) {
    currentState=state;
}

RoutingStrategy* LeafWorkItem::getRoutingStrategy() const {
    return routeStrategy;
}
void LeafWorkItem::setRoutingStrategy(RoutingStrategy* strategy) {
    routingStartegy=strategy;
}
ApprovalHandler* LeafWorkItem::getApprovalHandler() const {
    return approvalHandler;
}
void LeafWorkItem::setApprovalHandler(ApprovalHandler* handler){
    approvalHandler=handler;
}
std::string LeafWorkItem::determineAssignee() const {
    if (routingStrategy == nullptr) {
        throw std::logic_error("LeafWorkItem: no routing Strategy set");
    }
    return routingStrategy->determineAssignee(*this);
}
const WorkflowContext* LeafWorkItem::getContext() const {
    return workflowContext;
}
void LeafWorkItem::setContext(WorkflowContext* context) {
    workflowContext = context;
}