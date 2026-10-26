#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include "WorkflowComponent.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"

class WorkComponent; // Forward declaration

class WorkflowDefinition {
    private:
        std::string definitionId;
        std::string version;
        WorkflowComponent* rootElement;
    public:
        WorkflowDefinition(const std::string& definitionId, const std::string& version );
        WorkflowComponent* getRootElement();
};

#endif // WORKFLOWDEFINITION_H

