#ifndef WORKFLOWVISITOR_H
#define WORKFLOWVISITOR_H

// Forward declarations to avoid circular dependencies
class CompositeStage;
class LeafWorkItem;

class WorkflowVisitor {
public:
    virtual ~WorkflowVisitor() = default;
    virtual void visitStage(CompositeStage* stage) = 0;
    virtual void visitWorkItem(LeafWorkItem* item) = 0;
};

#endif // WORKFLOWVISITOR_H