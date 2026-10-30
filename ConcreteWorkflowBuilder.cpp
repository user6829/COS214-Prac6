#include "ConcreteWorkflowBuilder.h"
#include "WorkflowDefinition.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"
#include "RoutingStrategy.h"

ConcreteWorkflowBuilder::ConcreteWorkflowBuilder()
    : currentDefinition(new WorkflowDefinition("", "")),
      currentContainer(nullptr),
      pendingRouting(nullptr) {}

ConcreteWorkflowBuilder::~ConcreteWorkflowBuilder() {}

void ConcreteWorkflowBuilder::reset()
{
    currentDefinition = new WorkflowDefinition("", "");
    currentContainer = nullptr;
    openStages.clear();
    pendingRouting = nullptr;
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildStage(const std::string& stageName)
{
    CompositeStage* newStage =
        new CompositeStage(stageName, stageName);
    if (currentContainer)
        currentContainer->add(std::shared_ptr<WorkflowComponent>(newStage));
    else
    {
        currentDefinition->setRootElement(newStage);
        currentContainer = newStage;
    }
    openStages.push_back(newStage);
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildWorkitem(const std::string& itemName)
{
    if (!currentContainer) return *this;

    LeafWorkItem* item =
        new LeafWorkItem(itemName, itemName);
    if (pendingRouting)
        item->setRoutingStrategy(pendingRouting);
    currentContainer->add(std::shared_ptr<WorkflowComponent>(item));
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::setRoutingStrategy(RoutingStrategy* strategy)
{
    pendingRouting = strategy;
    return *this;
}

WorkflowDefinition* ConcreteWorkflowBuilder::getResult()
{
    return currentDefinition;
}