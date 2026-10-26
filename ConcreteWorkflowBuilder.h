#ifndef CONCRETE_WORKFLOW_BUILDER_H
#define CONCRETE_WORKFLOW_BUILDER_H

#include "WorkflowBuilder.h"
#include <string>

// Forward declarations for private member types
class WorkflowDefinition;
class CompositeStage;
class RoutingStrategy;

class ConcreteWorkflowBuilder : public WorkflowBuilder {
private:
    WorkflowDefinition* currentDefinition;
    CompositeStage* currentContainer;

public:
    ConcreteWorkflowBuilder();
    virtual ~ConcreteWorkflowBuilder();

    void reset();
    WorkflowBuilder& buildStage(const std::string& stageName);
    WorkflowBuilder& buildWorkitem(const std::string& itemName);
    WorkflowBuilder& setRoutingStrategy(RoutingStrategy* strategy);
    WorkflowDefinition* getResult();
};

#endif // CONCRETE_WORKFLOW_BUILDER_