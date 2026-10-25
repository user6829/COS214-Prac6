#include "LeafWorkItem.h"
#include "WorkflowComponent.h"
#include "WorkItemState.h"
#include "RoutingStrategy.h"
#include "WorkflowVisitor.h"
#include "ApprovalHandler.h"

LeafWorkItem::LeafWorkItem(const std::string& id, const std::string& name ): WorkflowComponent(id,name){}
void LeafWorkItem::execute(WorkflowContext* context) override{
    if(routingStrategy){
        routingStartegy->route(context);
    }
    if(approvalHandler){
        approvalHandler->handleApproval(context);
    }
    if(currentState){
        currentState->handle(context);
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
    return routeStrategy
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
