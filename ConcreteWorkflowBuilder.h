<<<<<<< HEAD
#ifndef CONCRETEWORKFLOWBUILDER_H
#define CONCRETEWORKFLOWBUILDER_H

#include "WorkflowBuilder.h"

#include <string>
#include <vector>

template <typename PayloadT> class CompositeStage;
template <typename PayloadT> class LeafWorkItem;

/** 
 * Design pattern: Builder (role: ConcreteBuilder)
 * 
 * Builds a WorkflowDefinition step by step whose tree is made of CompositeStage and
 * LeafWorkItem objects.
 * 
 * currentDefinition_ is owned by the builder until getResult() hands 
 * it
 * to the caller, The destructor and reset() must delete must delete a definition 
 * that was never handed over
 * 
 * The tree itself is owned by the definition: each CompositeStage deletes it's own children
 * 
 * openStages_ and lastItem_ are non-owning. They only record where the next element should be attached
 * 
 * 
 */
template <typename PayloadT>
class ConcreteWorkflowBuilder : public WorkflowBuilder<PayloadT> {
  public:
    ConcreteWorkflowBuilder();
    virtual ~ConcreteWorkflowBuilder();

    virtual WorkflowBuilder<PayloadT>& reset(const std::string& definitionId, const std::string& version);
    virtual WorkflowBuilder<PayloadT>& buildStage(const std::string& stageName);

    
    virtual WorkflowBuilder<PayloadT>& endStage();

    virtual WorkflowBuilder<PayloadT>& buildWorkItem(const std::string& itemName);

    virtual WorkflowBuilder<PayloadT>& setRoutingStrategy(RoutingStrategy<PayloadT>* strategy);

    virtual WorkflowBuilder<PayloadT>& setApprovalChain(ApprovalHandler* chain);

  
    virtual WorkflowDefinition<PayloadT>* getResult();

  private:
    WorkflowDefinition<PayloadT>* currentDefinition_; //owned by the builder until getResult() hands it to the caller
    std::vector<CompositeStage<PayloadT>*> openStages_; //non-owning
    LeafWorkItem<PayloadT>* lastItem_; //non-owning
    int nextId_;

    void requireDefinition() const;
    void requireItem() const;
    CompositeStage<PayloadT>& currentStage();
    std::string nextId(const std::string& prefix);
};


#endif // CONCRETEWORKFLOWBUILDER_H
=======
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
>>>>>>> origin/Development
