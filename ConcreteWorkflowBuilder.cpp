#include "ConcreteWorkflowBuilder.h"
#include "WorkflowDefinition.h"
#include "WorkflowStage.h"
#include "RoutingStrategy.h"

ConcreteWorkflowBuilder::ConcreteWorkflowBuilder() {
    currentDefinition = new WorkflowDefinition();
    currentContainer = nullptr;
}

ConcreteWorkflowBuilder::~ConcreteWorkflowBuilder() {
    // Note: If getResult() transfers ownership, this might cause a double-free.
    // Standard Builder Pattern usually leaves cleanup to the caller AFTER getResult().
    // For this example, we assume the caller takes ownership of the returned pointer.
}

void ConcreteWorkflowBuilder::reset() {
    currentDefinition = new WorkflowDefinition();
    currentContainer = nullptr;
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildStage(const std::string& stageName) {
    WorkflowStage* newStage = new WorkflowStage(stageName);
    currentDefinition->stages.push_back(newStage);
    currentContainer = newStage; // Switch context to this new stage
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::buildWorkitem(const std::string& itemName) {
    if (currentContainer) {
        currentContainer->workitems.push_back(itemName);
    }
    return *this;
}

WorkflowBuilder& ConcreteWorkflowBuilder::setRoutingStrategy(RoutingStrategy* strategy) {
    currentDefinition->routingStrategy = strategy;
    return *this;
}

WorkflowDefinition* ConcreteWorkflowBuilder::getResult() {
    return currentDefinition;
}