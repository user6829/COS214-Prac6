#include "ConcreteWorkflowBuilder.h"
#include "WorkflowDefinition.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"
#include "RoutingStrategy.h"
#include "ApprovalHandler.h"
#include "WorkItemState.h"
#include <stdexcept>

ConcreteWorkflowBuilder::ConcreteWorkflowBuilder()
    : currentDefinition(new WorkflowDefinition("", "")),
      currentContainer(nullptr),
      pendingRouting(nullptr),
      pendingApproval(nullptr),
      pendingState(nullptr) {}

ConcreteWorkflowBuilder::~ConcreteWorkflowBuilder()
{
    delete currentDefinition;
    delete pendingRouting;
    delete pendingApproval;
    delete pendingState;
}

void ConcreteWorkflowBuilder::reset()
{
    delete currentDefinition;
    delete pendingRouting;
    delete pendingApproval;
    delete pendingState;
    currentDefinition = new WorkflowDefinition("", "");
    currentContainer = nullptr;
    openStages.clear();
    pendingRouting = nullptr;
    pendingApproval = nullptr;
    pendingState = nullptr;
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildStage(const std::string& stageName)
{
    return buildStage(stageName, stageName);
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildStage(const std::string& id,
                                                     const std::string& name)
{
    if (!currentDefinition) reset();
    if (!currentContainer && currentDefinition->getRootElement())
        throw std::logic_error("A workflow must have one root stage");
    CompositeStage* newStage = new CompositeStage(id, name);
    if (currentContainer)
        currentContainer->add(std::shared_ptr<WorkflowComponent>(newStage));
    else
    {
        currentDefinition->setRootElement(newStage);
        currentContainer = newStage;
    }
    currentContainer = newStage;
    openStages.push_back(newStage);
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildWorkitem(const std::string& itemName)
{
    return buildWorkitem(itemName, itemName);
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildWorkitem(const std::string& id,
                                                        const std::string& name)
{
    if (!currentContainer) throw std::logic_error("Build a stage before a work item");

    LeafWorkItem* item = new LeafWorkItem(id, name);
    if (pendingRouting)
    {
        item->setRoutingStrategy(pendingRouting);
        pendingRouting = nullptr;
    }
    if (pendingApproval)
    {
        item->setApprovalHandler(pendingApproval);
        pendingApproval = nullptr;
    }
    if (pendingState)
    {
        item->setState(pendingState);
        pendingState = nullptr;
    }
    currentContainer->add(std::shared_ptr<WorkflowComponent>(item));
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::setApprovalHandler(
    ApprovalHandler<std::string>* handler)
{
    if (pendingApproval == handler) return *this;
    delete pendingApproval;
    pendingApproval = handler;
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::setInitialState(WorkItemState* state)
{
    if (pendingState == state) return *this;
    delete pendingState;
    pendingState = state;
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::setDefinitionMetadata(
    const std::string& id, const std::string& version)
{
    delete currentDefinition;
    currentDefinition = new WorkflowDefinition(id, version);
    currentContainer = nullptr;
    openStages.clear();
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::endStage()
{
    if (!openStages.empty())
        openStages.pop_back();
    currentContainer = openStages.empty() ? nullptr : openStages.back();
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::setRoutingStrategy(RoutingStrategy* strategy)
{
    if (pendingRouting == strategy)
        return *this;
    delete pendingRouting;
    pendingRouting = strategy;
    return *this;
}

WorkflowDefinition* ConcreteWorkflowBuilder::getResult()
{
    WorkflowDefinition* result = currentDefinition;
    currentDefinition = nullptr;
    currentContainer = nullptr;
    openStages.clear();
    return result;
}