/**
 * @file WorkflowDefinition.h
 * @brief Reusable workflow template containing a root component tree and version label.
 */
#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include "WorkflowComponent.h"

/**
 * @class WorkflowDefinition
 * @brief Reusable workflow template containing a root component tree and version label.
 */
class WorkflowDefinition {
    private:
        std::string definitionId;
        std::string version;
        WorkflowComponent* rootElement;
        std::string successResult{"COMPLETED"};
        bool completionReversible{true};
    public:
        WorkflowDefinition(const std::string& definitionId, const std::string& version );
        ~WorkflowDefinition() ;
        std::string getDefinitionId() const;
        std::string getVersion() const;
        void setRootElement(WorkflowComponent* root);
        WorkflowComponent* getRootElement() const;
        /** Domain label published only after all required work succeeds. */
        void setSuccessResult(const std::string& result) { successResult = result; }
        std::string getSuccessResult() const { return successResult; }
        /** Definitions with irreversible completion effects reject completed rollback. */
        void setCompletionReversible(bool allowed) { completionReversible = allowed; }
        bool isCompletionReversible() const { return completionReversible; }
        WorkflowDefinition(const WorkflowDefinition&) = delete;
        WorkflowDefinition& operator=(const WorkflowDefinition&) = delete;
};      

#endif // WORKFLOWDEFINITION_H

