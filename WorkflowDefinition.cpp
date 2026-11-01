
#include "WorkflowDefinition.h"
#include "WorkflowComponent.h"

WorkflowDefinition::WorkflowDefinition(const std::string& definitionId, const std::string& version)
    : definitionId(definitionId),version(version),rootElement(nullptr)
{
}

WorkflowDefinition::~WorkflowDefinition()
{
    
    delete rootElement;
}

WorkflowComponent* WorkflowDefinition::getRootElement() const
{
    return rootElement;
}

void WorkflowDefinition::setRootElement(WorkflowComponent* root)
{
    if (rootElement == root)
    {
        return;
    }
    
    delete rootElement;
    rootElement = root;
}

std::string WorkflowDefinition::getDefinitionId() const
{
    return definitionId;
}

std::string WorkflowDefinition::getVersion() const
{
    return version;
}