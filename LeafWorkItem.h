#ifndef LEAFWORKITEM_H
#define LEAFWORKITEM_H


#include "WorkflowComponent.h"
#include "WorkflowContext.h"

class WorkItemState;
class RoutingStrategy;
template <typename T>class ApprovalHandler;
class WorkflowContext;

class LeafWorkItem : public WorkflowComponent {
private:
    WorkItemState* currentState{nullptr};
    RoutingStrategy* routingStrategy{nullptr};
    ApprovalHandler<std::string>* approvalHandler{nullptr};
    WorkflowContext* context{nullptr};

public:
    LeafWorkItem(const std::string& id, const std::string& name );
    ~LeafWorkItem() override = default;
    void execute(WorkflowContext *context);
    void accept(WorkflowVisitor *visitor) override;
    WorkItemState* getCurrentState() const ;
    void setState(WorkItemState* state) ;
    RoutingStrategy* getRoutingStrategy() const ;
    void setRoutingStrategy(RoutingStrategy* strategy) ;
    ApprovalHandler<std::string>* getApprovalHandler() const ;
    void setApprovalHandler(ApprovalHandler<std::string>* handler);
    WorkflowContext* getContext() const ;
    void setContext(WorkflowContext* context) ;

};
#endif // LEAFWORKITEM_H