#ifndef CONCRETE_WORKFLOW_BUILDER_H
#define CONCRETE_WORKFLOW_BUILDER_H

#include "WorkflowBuilder.h"
#include <string>
#include <vector>

class WorkflowDefinition;
class CompositeStage;
class RoutingStrategy;

class ConcreteWorkflowBuilder : public WorkflowBuilder
{
private:
    WorkflowDefinition* currentDefinition;
    CompositeStage* currentContainer;
    std::vector<CompositeStage*> openStages;
    RoutingStrategy* pendingRouting;

public:
    ConcreteWorkflowBuilder();
    ~ConcreteWorkflowBuilder() override;

    void reset() override;
    WorkflowBuilder& buildStage(const std::string& stageName) override;
    WorkflowBuilder& buildWorkitem(const std::string& itemName) override;
    WorkflowBuilder& setRoutingStrategy(RoutingStrategy* strategy) override;
    WorkflowDefinition* getResult() override;
};

#endif