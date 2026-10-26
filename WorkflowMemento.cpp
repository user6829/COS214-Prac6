#include "WorkflowMemento.h"

WorkflowMemento::WorkflowMemento(std::map<std::string, std::string>* state, std::string ts)
    : instanceState(state), timestamp(ts) {}

WorkflowMemento::~WorkflowMemento() {
    delete instanceState;
}

std::map<std::string, std::string>* WorkflowMemento::getState() {
    return instanceState;
}

std::string WorkflowMemento::getTimestamp() const {
    return timestamp;
}