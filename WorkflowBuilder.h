/**
 * @file WorkflowBuilder.h
 * @brief Builder interface for constructing a workflow definition step by step.
 */
#ifndef WORKFLOW_BUILDER_H
#define WORKFLOW_BUILDER_H

#include <string>

// Forward declarations
class RoutingStrategy;
class WorkflowDefinition;
template <typename T> class ApprovalHandler;
class WorkItemState;

/**
 * @class WorkflowBuilder
 * @brief Builder interface for constructing a workflow definition step by step.
 */
class WorkflowBuilder {
public:
    virtual ~WorkflowBuilder() {}

    virtual void reset() = 0;
    virtual WorkflowBuilder& buildStage(const std::string& stageName) = 0;
    virtual WorkflowBuilder& buildStage(const std::string& id,
                                        const std::string& name)
    {
        (void)id;
        return buildStage(name);
    }
    virtual WorkflowBuilder& endStage() = 0;
    virtual WorkflowBuilder& buildWorkitem(const std::string& itemName) = 0;
    virtual WorkflowBuilder& buildWorkitem(const std::string& id,
                                           const std::string& name)
    {
        (void)id;
        return buildWorkitem(name);
    }
    virtual WorkflowBuilder& setRoutingStrategy(RoutingStrategy* strategy) = 0;
    virtual WorkflowBuilder& setApprovalHandler(
        ApprovalHandler<std::string>* handler)
    {
        (void)handler;
        return *this;
    }
    virtual WorkflowBuilder& setInitialState(WorkItemState* state)
    {
        (void)state;
        return *this;
    }
    virtual WorkflowBuilder& setDefinitionMetadata(const std::string& id,
                                                    const std::string& version)
    {
        (void)id;
        (void)version;
        return *this;
    }
    virtual WorkflowDefinition* getResult() = 0;
};

#endif // WORKFLOW_BUILDER_H