#ifndef WORKFLOWBUILDER_H
#define WORKFLOWBUILDER_H

#include <string>

template <typename PayloadT> class WorkflowDefinition;
template <typename PayloadT> class RoutingStrategy;
class ApprovalHandler;

/**
 * Design pattern: Builder
 * 
 * Assembles a WorkflowDefinition step by step so a scenario can describe its
 * process from main() without the engine knowing anything about it
 * 
 * PayloadT: the data type a scenario attaches to its workflows
 * 
 * getResul() transfers ownership of the finished definitionto the caller, who must delete it
 * RoutingStrategy and ApprovalHandler pointers are not owned by the the builder or work item. The scenario (main/registry) creates them
 * and keeps them alive for as long as the definition is used
 * 
 */
template <typename PayloadT> 
class WorkflowBuilder { 
  public:
    virtual ~WorkflowBuilder();

    //Starts a new, empty definition
    virtual WorkflowBuilder& reset(const std::string& definitionId, const std::string& version) = 0;

    //Opens a stage nested within the current stage. The new stage becomes the current stage
    virtual WorkflowBuilder& buildStage(const std::string& stageName) = 0;

    //Close the most recently opened stage.
    virtual WorkflowBuilder& endStage() = 0;

    //Adds a work item to the current stage
    virtual WorkflowBuilder& buildWorkItem(const std::string& itemName) = 0;

    virtual WorkflowBuilder& setRoutingStrategy(RoutingStrategy<PayloadT>* strategy) = 0;

    virtual WorkflowBuilder& setApprovalChain(ApprovalHandler* chain) = 0;

    //Returns the finished definition. Caller owns it and must delete it
    virtual WorkflowDefinition<PayloadT>* getResult() = 0;

  protected:
    WorkflowBuilder();


  private: 
    WorkflowBuilder(const WorkflowBuilder&);// Not copyable 
    WorkflowBuilder& operator=(const WorkflowBuilder&);// Not assignable interface
    
};
#endif // WORKFLOWBUILDER_H


