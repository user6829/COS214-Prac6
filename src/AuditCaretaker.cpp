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
WorkflowMemento* AuditCaretaker::getSnapshot(const std::string& instanceId, int index) {
    if (index < 0) return nullptr;
    int current = 0;
    for (WorkflowMemento* m : history) {
        if (!m || !m->getState()) continue;
        std::map<std::string, std::string>* state = m->getState();
        std::map<std::string, std::string>::const_iterator it = state->find("instanceID");
        if (it != state->end() && it->second == instanceId) {
            if (current == index) return m;
            ++current;
        }
    }
    return nullptr;
}

int AuditCaretaker::size(const std::string& instanceId) const {
    int count = 0;
    for (WorkflowMemento* m : history) {
        if (!m || !m->getState()) continue;
        std::map<std::string, std::string>* state = m->getState();
        std::map<std::string, std::string>::const_iterator it = state->find("instanceID");
        if (it != state->end() && it->second == instanceId) ++count;
    }
    return count;
}
