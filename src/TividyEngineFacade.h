/**
 * @file TividyEngineFacade.h
 * @brief Facade providing the main API for definitions, instances, execution and history.
 */
#ifndef TIVIDY_ENGINE_FACADE_H
#define TIVIDY_ENGINE_FACADE_H

#include <string>
#include <map>
#include <vector>
#include <functional>

class AuditCaretaker;
class WorkflowDefinition;
class WorkflowInstance;
class WorkflowContext;
class WorkflowVisitor;
class LeafWorkItem;

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
    std::vector<WorkflowDefinition*> retiredDefinitions;
    std::map<std::string, InstanceRecord> instances;               // owns instances and contexts
    AuditCaretaker* auditCaretaker;
    int nextInstanceNumber;

    InstanceRecord& requireInstance(const std::string& instanceId);
    void snapshot(WorkflowInstance* instance);
    void updateOutcome(InstanceRecord& record, const std::string& event);

    TividyEngineFacade(const TividyEngineFacade&) = delete;
    TividyEngineFacade& operator=(const TividyEngineFacade&) = delete;

public:
    TividyEngineFacade();
    ~TividyEngineFacade();

    /**
     * @brief Registers a reusable workflow definition.
     * @param defId Unique key used to look up the definition.
     * @param definition Heap-allocated definition; ownership transfers to the facade.
     * @note Registering an existing key retires the prior definition until facade destruction.
     */
    void registerDefinition(const std::string& defId, WorkflowDefinition* definition);

    /**
     * @brief Creates a running instance of a registered definition.
     * @param defId Registered definition key.
     * @param initialData Initial payload copied into the new instance context.
     * @return Pointer to the created instance, owned by the facade.
     * @throws std::invalid_argument If defId is not registered.
     */
    WorkflowInstance* startWorkflow(const std::string& defId,
                                    const std::map<std::string, std::string>& initialData);

    /**
     * @brief Executes a component identified within an instance's definition.
     * @param instanceId Identifier of the running instance.
     * @param stepId Identifier of the component to execute.
     * @throws std::invalid_argument If the instance or step is unknown.
     */
    void executeStep(const std::string& instanceId, const std::string& stepId);

    /**
     * @brief Records a decision in the instance payload and invokes the approval chain.
     * @param instanceId Identifier of the running instance.
     * @param stepId Identifier of the work item.
     * @param decision Decision text, such as APPROVE or REJECT.
     * @param actorRole Role of the person recording the decision; must match the routed assignee.
     * @throws std::invalid_argument If the instance, work item, decision or actor role is invalid.
     * @throws std::runtime_error If the work item has no approval handler.
     */
    void processApproval(const std::string& instanceId, const std::string& stepId,
                         const std::string& decision, const std::string& actorRole);

    /** Publish an audit annotation without allowing a caller to change lifecycle state. */
    void recordEvent(const std::string& instanceId, const std::string& event);
    /** Next enabled, required item; null for a terminal instance. Borrowed pointer. */
    LeafWorkItem* getNextWorkItem(const std::string& instanceId);
    /** Execute/decide the next item. WAITLIST can subsequently be decided again. */
    void advanceWorkflow(const std::string& instanceId, const std::string& decision = "APPROVE");
    /** Automatic execution stops on rejection, completion or waitlist. */
    using ProgressCallback = std::function<void(LeafWorkItem&, bool)>;
    /** Progress callback receives true before and false after an engine operation. */
    void runToCompletion(const std::string& instanceId, ProgressCallback progress = ProgressCallback());
    /** Terminal outcomes, derived from actual required work-item statuses. */
    bool canGenerateDocuments(const std::string& instanceId);
    std::string getOutcome(const std::string& instanceId);
    /**
     * @brief Restores an instance from a saved memento.
     * @param instanceId Identifier of the instance to restore.
     * @param snapshotIndex Zero-based index in the caretaker's history.
     * @throws std::invalid_argument If the instance is unknown.
     * @throws std::out_of_range If no snapshot exists at the requested index.
     */
    void rollbackInstance(const std::string& instanceId, int snapshotIndex);

    /**
     * @brief Applies a visitor to each registered definition tree.
     * @param visitor Visitor to apply; null pointers are ignored.
     */
    void exportMetrics(WorkflowVisitor* visitor);
    /** @brief Looks up a running instance by its instance ID. @return Null if not found. */
    WorkflowInstance* getInstance(const std::string& instanceId);
    std::vector<std::string> getInstanceIds() const;
    /** @brief Looks up a registered definition. @return Null if not found. */
    WorkflowDefinition* getDefinition(const std::string& defId);
    /** @brief Returns a read-only view of all registered definitions. */
    const std::map<std::string, WorkflowDefinition*>& getAllDefinitions() const;
    /** @brief Returns the facade-owned history caretaker. */
    AuditCaretaker* getAuditCaretaker() const;
};

#endif // TIVIDY_ENGINE_FACADE_H