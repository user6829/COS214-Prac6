#include "WorkflowInstance.h"
#include "ActiveState.h"
#include "InactiveState.h"
#include "CompletedState.h"
#include "RejectedState.h"
#include "DefectResolutionState.h"

namespace {
std::unique_ptr<WorkItemState> stateFor(const std::string& status)
{
    if (status == "COMPLETED" || status == "APPROVED" || status == "SKIPPED")
        return std::unique_ptr<WorkItemState>(new CompletedState());
    if (status == "REJECTED") return std::unique_ptr<WorkItemState>(new RejectedState());
    if (status == "DEFECT_RESOLUTION")
        return std::unique_ptr<WorkItemState>(new DefectResolutionState());
    if (status == "PENDING_APPROVAL" || status == "WAITLISTED")
        return std::unique_ptr<WorkItemState>(new ActiveState());
    return std::unique_ptr<WorkItemState>(new InactiveState());
}
}
WorkflowContext* WorkflowInstance::getContext() const { return currentContext; }

void WorkflowInstance::publish(const std::string& nextState, const std::string& event)
{
    state = nextState;
    eventName = event;
    currentContext->setData("State", state);
    currentContext->setData("Event", eventName);
    notifyObservser();
}

WorkItemState& WorkflowInstance::getWorkItemState(const std::string& stepId)
{
    if (!workItemStates.count(stepId)) {
        std::string status = "NEW";
        try { status = currentContext->getData("workitem:" + stepId + ":status"); }
        catch (const std::out_of_range&) {}
        workItemStates[stepId] = stateFor(status);
    }
    return *workItemStates.at(stepId);
}

void WorkflowInstance::transitionWorkItem(const std::string& stepId, const std::string& status)
{
    currentContext->setData("workitem:" + stepId + ":status", status);
    workItemStates[stepId] = stateFor(status);
}
