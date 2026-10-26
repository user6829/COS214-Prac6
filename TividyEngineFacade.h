#ifndef TIVIDY_ENGINE_FACADE_H
#define TIVIDY_ENGINE_FACADE_H

#include <string>
#include <map>

class AuditCaretaker;
class WorkflowDefinition;
class WorkflowInstance;
class WorkflowContext;
class WorkflowVisitor;

/**
 * @brief Single entry point to the TIVIDY workflow engine (Facade pattern).
 *        Hides definition storage, running instances and the audit caretaker.
 */
class TividyEngineFacade
{
private:
    /** @brief Everything the facade tracks about one running workflow. */
    struct InstanceRecord
    {
        WorkflowInstance* instance;
        WorkflowContext* context;
        WorkflowDefinition* definition; // not owned: definitionRegistry owns it
    };

    std::map<std::string, WorkflowDefinition*> definitionRegistry; // owns the definitions
    std::map<std::string, InstanceRecord> instances;               // owns instances and contexts
    AuditCaretaker* auditCaretaker;
    int nextInstanceNumber;

    InstanceRecord& requireInstance(const std::string& instanceId);
    void snapshot(WorkflowInstance* instance);

    TividyEngineFacade(const TividyEngineFacade&) = delete;
    TividyEngineFacade& operator=(const TividyEngineFacade&) = delete;

public:
    TividyEngineFacade();
    ~TividyEngineFacade();

    /** @brief Registers a definition under defId. The facade takes ownership. */
    void registerDefinition(const std::string& defId, WorkflowDefinition* definition);

    /** @brief Creates a running instance of a registered definition. */
    WorkflowInstance* startWorkflow(const std::string& defId,
                                    const std::map<std::string, std::string>& initialData);

    /** @brief Executes one step (found by id) of an instance. */
    void executeStep(const std::string& instanceId, const std::string& stepId);

    /** @brief Records a decision and passes it to the step's approval handler. */
    void processApproval(const std::string& instanceId, const std::string& stepId,
                         const std::string& decision);

    /** @brief Restores an instance from a snapshot in the audit caretaker. */
    void rollbackInstance(const std::string& instanceId, int snapshotIndex);

    /** @brief Runs the visitor over every registered definition's tree. */
    void exportMetrics(WorkflowVisitor* visitor);
};

#endif // TIVIDY_ENGINE_FACADE_H