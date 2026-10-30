#include "AuditCaretaker.h"

AuditCaretaker::~AuditCaretaker() {
    for (WorkflowMemento* m : history) {
        delete m;
    }
    history.clear();
}

void AuditCaretaker::addSnapshot(WorkflowMemento* memento) {
    if (memento != nullptr) {
        history.push_back(memento);
    }
}

WorkflowMemento* AuditCaretaker::getSnapshot(int index) {
    if (index < 0) {
        return nullptr;
    }

    int i = 0;
    for (WorkflowMemento* m : history) {
        if (i == index) {
            return m;
        }
        i++;
    }

    return nullptr;
}

int AuditCaretaker::size() const {
    return static_cast<int>(history.size());
}