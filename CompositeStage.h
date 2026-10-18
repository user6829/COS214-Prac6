#ifndef COMPOSITESTAGE_H
#define COMPOSITESTAGE_H

#include "WorkflowComponent.h"
#include <vector>
#include <memory>
#include <algorithm>

class CompositeStage : public WorkflowComponent {
private:
    std::vector<std::shared_ptr<WorkflowComponent>> children;

public:
    CompositeStage(const std::string& id, const std::string& name);
    ~CompositeStage() override = default;
    void add(std::shared_ptr<WorkflowComponent> component);
    void remove(std::shared_ptr<WorkflowComponent> component);
    const std::vector<std::shared_ptr<WorkflowComponent>>& getChildren() const;
    void execute(WorkflowContext* context) override;
    void accept(WorkflowVisitor* visitor) override;
};

#endif // COMPOSITESTAGE_H