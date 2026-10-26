#include "LeafWorkItem.h"
#include "WorkflowComponent.h"
#include "WorkItemState.h"
#include "RoutingStrategy.h"
#include "WorkflowVisitor.h"
#include "ApprovalHandler.h"

LeafWorkItem::LeafWorkItem(const std::string &id, const std::string &name) : WorkflowComponent(id, name)
{
}
void LeafWorkItem::execute(WorkflowContext<WorkflowComponent> *context)
{
    if (routingStrategy)
    {
        routingStrategy->determineAssignee(*this);
    }
    if (approvalHandler)
    {
        std::string decision;
        double amount;
        approvalHandler->processRequest(amount, decision);
    }
    if (currentState)
    {
        currentState->handle(this);
    }
}

void LeafWorkItem::accept(WorkflowVisitor *visitor)
{
    if (visitor)
    {
        visitor->visitWorkItem(this);
    }
}

WorkitemState *LeafWorkItem::getCurrentState() const
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

ApprovalHandler *LeafWorkItem::getApprovalHandler() const
{
    return approvalHandler;
}

void LeafWorkItem::setApprovalHandler(ApprovalHandler *handler)
{
    approvalHandler = handler;
}
