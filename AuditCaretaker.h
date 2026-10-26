#ifndef AUDIT_CARETAKER_H
#define AUDIT_CARETAKER_H

#include "WorkflowMemento.h"
#include <list>

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
};

#endif