/**
 * @file ConcreteWorkflowBuilder.h
 * @brief Concrete Builder that assembles workflow definitions from stages and work items.
 */
#ifndef CONCRETE_WORKFLOW_BUILDER_H
#define CONCRETE_WORKFLOW_BUILDER_H

#include "WorkflowBuilder.h"
#include <string>
#include <vector>

class WorkflowDefinition;
class CompositeStage;
class RoutingStrategy;
template <typename T> class ApprovalHandler;
class WorkItemState;

/**
 * @class ConcreteWorkflowBuilder
 * @brief Concrete Builder that assembles workflow definitions from stages and work items.
 */
class ConcreteWorkflowBuilder : public WorkflowBuilder
{
private:
    WorkflowDefinition* currentDefinition;
    CompositeStage* currentContainer;
    std::vector<CompositeStage*> openStages;
    RoutingStrategy* pendingRouting;
    ApprovalHandler<std::string>* pendingApproval;
    WorkItemState* pendingState;

public:
    ConcreteWorkflowBuilder(const ConcreteWorkflowBuilder&) = delete;
    ConcreteWorkflowBuilder& operator=(const ConcreteWorkflowBuilder&) = delete;
    ConcreteWorkflowBuilder();
    ~ConcreteWorkflowBuilder() override;

    void reset() override;
    WorkflowBuilder& buildStage(const std::string& stageName) override;
    WorkflowBuilder& buildStage(const std::string& id,
                                 const std::string& name) override;
    WorkflowBuilder& endStage() override;
    WorkflowBuilder& buildWorkitem(const std::string& itemName) override;
    WorkflowBuilder& buildWorkitem(const std::string& id,
                                   const std::string& name) override;
    WorkflowBuilder& setRoutingStrategy(RoutingStrategy* strategy) override;
    WorkflowBuilder& setApprovalHandler(
        ApprovalHandler<std::string>* handler) override;
    WorkflowBuilder& setInitialState(WorkItemState* state) override;
    WorkflowBuilder& setDefinitionMetadata(const std::string& id,
                                            const std::string& version) override;
    WorkflowDefinition* getResult() override;
};

#endif