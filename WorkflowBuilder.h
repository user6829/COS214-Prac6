
#ifndef WORKFLOW_BUILDER_H
#define WORKFLOW_BUILDER_H

#include <string>

// Forward declarations
class RoutingStrategy;
class WorkflowDefinition;

class WorkflowBuilder {
public:
    virtual ~WorkflowBuilder() {}

    virtual void reset() = 0;
    virtual WorkflowBuilder& buildStage(const std::string& stageName) = 0;
    virtual WorkflowBuilder& buildWorkitem(const std::string& itemName) = 0;
    virtual WorkflowBuilder& setRoutingStrategy(RoutingStrategy* strategy) = 0;
    virtual WorkflowDefinition* getResult() = 0;
};

#endif // WORKFLOW_BUILDER_H



