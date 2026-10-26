#ifndef LEAFWORKITEM_H
#define LEAFWORKITEM_H


#include "WorkflowComponent.h"
#include "WorkflowContext.h"

class WorkitemState;
class RoutingStrategy;
class ApprovalHandler;

class LeafWorkItem : public WorkflowComponent {
private:
    WorkItemState* currentState{nullptr};
    RoutingStrategy* routingStrategy{nullptr};
    ApprovalHandler* approvalHandler{nullptr};

public:
    LeafWorkItem(const std::string& id, const std::string& name );
    ~LeafWorkItem() override = default;
    void execute(WorkflowContext<WorkflowComponent>* context);
    void accept(WorkflowVisitor* visitor) override;
    WorkitemState* getCurrentState() const ;
    void setState(WorkItemState* state) ;
    RoutingStrategy* getRoutingStrategy() const ;
    void setRoutingStrategy(RoutingStrategy* strategy) ;
    ApprovalHandler* getApprovalHandler() const ;
    void setApprovalHandler(ApprovalHandler* handler);

};
#endif // LEAFWORKITEM_H