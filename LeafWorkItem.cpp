#include "LeafWorkItem.h"
#include "WorkflowComponent.h"
#include "WorkItemState.h"
#include "RoutingStrategy.h"
#include "WorkflowVisitor.h"
#include "ApprovalHandler.h"
#include <string>

LeafWorkItem::LeafWorkItem(const std::string &id, const std::string &name) : WorkflowComponent(id, name)
{
}
void LeafWorkItem::execute(WorkflowContext* context)
{
    this->context = context; // Store the context for later use

    if (routingStrategy)
        routingStrategy->determineAssignee(*this);

    if (approvalHandler && context)
    {
        std::string amount = "0";
        try { amount = context->getData("amount"); } catch (...) {}
        std::string decision = "PENDING";
        approvalHandler->processRequest(amount, decision);
    }

    if (currentState)
        currentState->handle(this);
}

void LeafWorkItem::accept(WorkflowVisitor *visitor)
{
    if (visitor)
    {
        visitor->visitWorkItem(this);
    }
}

WorkItemState* LeafWorkItem::getCurrentState() const
{
    return currentState;
}
void LeafWorkItem::setState(WorkItemState *state)
{
    currentState = state;
}

RoutingStrategy *LeafWorkItem::getRoutingStrategy() const
{
    return routingStrategy;
}

void LeafWorkItem::setRoutingStrategy(RoutingStrategy *strategy)
{
    routingStrategy = strategy;
}

ApprovalHandler<std::string>* LeafWorkItem::getApprovalHandler() const
{
    return approvalHandler;
}

void LeafWorkItem::setApprovalHandler(ApprovalHandler<std::string>* handler)
{
    approvalHandler = handler;
}

WorkflowContext* LeafWorkItem::getContext() const {
    return context;
}
void LeafWorkItem::setContext(WorkflowContext* context){
    this->context = context;
}
