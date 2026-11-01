#include "CompositeStage.h"
#include "WorkflowComponent.h"
#include "WorkflowVisitor.h"
#include "WorkflowContext.h"
#include <algorithm>

CompositeStage::CompositeStage(const std::string &id, const std::string &name) : WorkflowComponent(id, name) {}

void CompositeStage::add(std::shared_ptr<WorkflowComponent> component)
{
    if (component)
    {
        children.push_back(component);
    }
}

void CompositeStage::remove(std::shared_ptr<WorkflowComponent> component)
{
    auto it = std::find(children.begin(), children.end(), component);
    if (it != children.end())
    {
        children.erase(it);
    }
}

const std::vector<std::shared_ptr<WorkflowComponent>> &CompositeStage::getChildren() const
{
    return children;
}

void CompositeStage::execute(WorkflowContext *context)
{
    for (auto &child : children)
    {
        child->execute(context);
    }
}

void CompositeStage::accept(WorkflowVisitor *visitor)
{
    if (visitor)
    {
        visitor->visitStage(this);
        for (auto &child : children)
        {
            child->accept(visitor);
        }
    }
    else
    {
        return;
    }
}
