#ifndef LEAFWORKITEM_H
#define LEAFWORKITEM_H


#include "WorkflowComponent.h"
#include "WorkflowContext.h"

class WorkitemState;
class RoutingStrategy;
class ApprovalHandler;

class LeafWorkItem : public WorkflowComponent {
private:
    WorkitemState* currentState{nullptr};
    RoutingStrategy* routingStrategy{nullptr};
    ApprovalHandler* approvalHandler{nullptr};

public:
    LeafWorkItem(const std::string& id, const std::string& name );
    ~LeafWorkItem() override = default;
    void execute(WorkflowContext* context) override;
    void accept(WorkflowVisitor* visitor) override;
    WorkitemState* getCurrentState() const ;
    void setState(WorkitemState* state) ;
    RoutingStrategy* getRoutingStrategy() const ;
    void setRoutingStrategy(RoutingStrategy* strategy) ;
    ApprovalHandler* getApprovalHandler() const ;
    void setApprovalHandler(ApprovalHandler* handler);

};
#endif // LEAFWORKITEM_H