/**
 *
 @file WorkflowComponent.h
 @brief The Component header file for the Composite design pattern
 */
#ifndef WORKFLOWCOMPONENT_H
#define WORKFLOWCOMPONENT_H

#include <string>

class WorkflowContext;
class WorkflowVisitor;

class WorkflowComponent
{
protected:
    std::string id;
    std::string name;

public:
    WorkflowComponent(const std::string &id, const std::string &name);
    virtual ~WorkflowComponent() = default;
    std::string getId() const;
    void setId(const std::string &id);
    std::string getName() const;
    void setName(const std::string &name);
    virtual void execute(WorkflowContext *context) = 0;
    virtual void accept(WorkflowVisitor *visitor) = 0;
};

#endif // WORKFLOWCOMPONENT_H