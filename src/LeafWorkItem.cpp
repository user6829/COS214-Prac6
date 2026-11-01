#include "LeafWorkItem.h"
#include "WorkflowContext.h"
#include "WorkflowComponent.h"
#include "WorkItemState.h"
#include "RoutingStrategy.h"
#include "WorkflowVisitor.h"
#include "ApprovalHandler.h"
#include <string>
#include <stdexcept>
#include <cmath>

LeafWorkItem::LeafWorkItem(const std::string &id, const std::string &name) : WorkflowComponent(id, name)
{
}

LeafWorkItem::~LeafWorkItem()
{
    delete currentState;
    delete routingStrategy;
    delete approvalHandler;
}

void LeafWorkItem::execute(WorkflowContext* context)
{
    if (!context) return;

    if (routingStrategy)
        context->setData("assignee:" + id, routingStrategy->determineAssignee(*this, *context));

    if (approvalHandler && context)
    {
        std::string amount = "0";
        try { amount = context->getData("amount"); } catch (...) {}
        context->setData("rule:" + id + ":count", "0");
        std::string decision = "PENDING";
        const std::string precheck =
            approvalHandler->processRequest(amount, decision, *context, id);
        context->setData("approval:" + id + ":precheck", precheck);
    }

    // Runtime state belongs to WorkflowInstance, never the shared definition.
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
    if (currentState == state)
        return;
    delete currentState;
    currentState = state;
}

RoutingStrategy *LeafWorkItem::getRoutingStrategy() const
{
    return routingStrategy;
}

void LeafWorkItem::setRoutingStrategy(RoutingStrategy *strategy)
{
    if (routingStrategy == strategy)
        return;
    delete routingStrategy;
    routingStrategy = strategy;
}

ApprovalHandler<std::string>* LeafWorkItem::getApprovalHandler() const
{
    return approvalHandler;
}

void LeafWorkItem::setApprovalHandler(ApprovalHandler<std::string>* handler)
{
    if (approvalHandler == handler)
        return;
    delete approvalHandler;
    approvalHandler = handler;
}

void LeafWorkItem::setCondition(const std::string& key, double threshold, bool high)
{
    conditionKey = key;
    conditionThreshold = threshold;
    conditionHigh = high;
}

bool LeafWorkItem::isEnabled(const WorkflowContext& context) const
{
    if (conditionKey.empty()) return true;
    const std::string text = context.getData(conditionKey);
    std::size_t parsed = 0;
    const double value = std::stod(text, &parsed);
    if (parsed != text.size() || !std::isfinite(value))
        throw std::invalid_argument("Invalid branch value: " + conditionKey);
    return (value >= conditionThreshold) == conditionHigh;
}
