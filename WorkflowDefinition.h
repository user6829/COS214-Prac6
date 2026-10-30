#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include "WorkflowComponent.h"

class WorkflowDefinition {
    private:
        std::string definitionId;
        std::string version;
        WorkflowComponent* rootElement;
    public:
        WorkflowDefinition(const std::string& definitionId, const std::string& version );
        ~WorkflowDefinition() ;
        std::string getDefinitionId() const;
        std::string getVersion() const;
        void setRootElement(WorkflowComponent* root);
        WorkflowComponent* getRootElement() const;
        WorkflowDefinition(const WorkflowDefinition&) = delete;
        WorkflowDefinition& operator=(const WorkflowDefinition&) = delete;
};      

#endif // WORKFLOWDEFINITION_H

