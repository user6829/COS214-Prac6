/**
 * @file AuditCaretaker.h
 * @brief Owns and indexes workflow mementos used for audit history and rollback.
 */
#ifndef AUDIT_CARETAKER_H
#define AUDIT_CARETAKER_H

#include "WorkflowMemento.h"
#include <list>
#include <string>

/**
 * @brief Manages and maintains the history of WorkflowMemento objects.
 */
class AuditCaretaker {
private:
    /**
     * @brief List storing pointers to historical workflow mementos.
     */
    std::list<WorkflowMemento*> history;

public:
    AuditCaretaker() = default;
    AuditCaretaker(const AuditCaretaker&) = delete;
    AuditCaretaker& operator=(const AuditCaretaker&) = delete;
    int size() const;
    /**
     * @brief Destructor for cleaning up stored mementos.
     */
    ~AuditCaretaker();

    /**
     * @brief Adds a workflow snapshot memento to the historical record.
     * @param memento Pointer to the WorkflowMemento to store.
     */
    void addSnapshot(WorkflowMemento* memento);

    /**
     * @brief Retrieves a specific workflow snapshot from history by index.
     * @param index The zero-based index of the snapshot to retrieve.
     * @return Pointer to the WorkflowMemento at the specified index, or nullptr if out of bounds.
     */
    WorkflowMemento* getSnapshot(int index);
    /** Returns the zero-based snapshot belonging to one instance only. */
    WorkflowMemento* getSnapshot(const std::string& instanceId, int index);
    /** Number of snapshots belonging to one instance. */
    int size(const std::string& instanceId) const;
};

#endif