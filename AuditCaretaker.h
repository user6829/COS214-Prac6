#ifndef AUDIT_CARETAKER_H
#define AUDIT_CARETAKER_H

#include "WorkflowMemento.h"
#include <list>

class AuditCaretaker {
private:
    std::list<WorkflowMemento*> history;

public:
    void addSnapshot(WorkflowMemento* memento);
    WorkflowMemento* getSnapshot(int index);
};

#endif