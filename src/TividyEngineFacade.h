#ifndef TIVIDY_ENGINE_FACADE_H
#define TIVIDY_ENGINE_FACADE_H
#include <string>
#include <map>

class WorkflowDefinitionRegistry;
class InstanceManager;
class AuditCaretaker;
class WorkflowInstance;
class WorkflowVisitor;

class TividyEngineFacade{
    private:
        WorkflowDefinitionRegistry* definitionRegistry;
        InstanceManager* instanceManager;
        AuditCaretaker* auditCaretaker;
    
        public:
            WorkflowInstance* startWorkflow(const std::string& defId, const std::map<std::string, std::string>& initialData);
            void executeStep(const std::string& instanceId, const std::string& stepId);
            void processApproval(const std::string& instanceId, const std::string& stepId, const std::string& decision);
            void rollbackInstance(const std::string& instanceId, int snapshotIndex);
            void* exportMetrics(WorkflowVisitor* visitor);
};


#endif