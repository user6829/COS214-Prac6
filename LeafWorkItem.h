/**
 * @file LeafWorkItem.h
 * @brief Leaf node of the workflow Composite; owns its state, routing and approval policies.
 */
#ifndef LEAFWORKITEM_H
#define LEAFWORKITEM_H


#include "WorkflowComponent.h"

class WorkItemState;
class RoutingStrategy;
template <typename T>class ApprovalHandler;

/**
 * @class LeafWorkItem
 * @brief Leaf node of the workflow Composite; owns its state, routing and approval policies.
 */
class LeafWorkItem : public WorkflowComponent {
private:
    bool deferred{false};
    std::string conditionKey;
    double conditionThreshold{0.0};
    bool conditionHigh{false};
    WorkItemState* currentState{nullptr};
    RoutingStrategy* routingStrategy{nullptr};
    ApprovalHandler<std::string>* approvalHandler{nullptr};

public:
    LeafWorkItem(const std::string& id, const std::string& name );
    ~LeafWorkItem() override;
    /** Exclude post-process work (e.g. delivery) from this process outcome. */
    void setDeferred(bool value) { deferred = value; }
    bool isDeferred() const { return deferred; }
    /** Configure one mutually exclusive threshold branch; evaluated by the engine. */
    void setCondition(const std::string& key, double threshold, bool high);
    bool isEnabled(const WorkflowContext& context) const;
    LeafWorkItem(const LeafWorkItem&) = delete;
    LeafWorkItem& operator=(const LeafWorkItem&) = delete;
    void execute(WorkflowContext *context) override;
    void accept(WorkflowVisitor *visitor) override;
    WorkItemState* getCurrentState() const ;
    void setState(WorkItemState* state) ;
    RoutingStrategy* getRoutingStrategy() const ;
    void setRoutingStrategy(RoutingStrategy* strategy) ;
    ApprovalHandler<std::string>* getApprovalHandler() const ;
    void setApprovalHandler(ApprovalHandler<std::string>* handler);

};
#endif // LEAFWORKITEM_H