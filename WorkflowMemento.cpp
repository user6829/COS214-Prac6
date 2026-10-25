#include "WorkflowMemento.h"

/**
 * @brief Constructs the WorkflowMemento.
 */
WorkflowMemento::WorkflowMemento(std::map<std::string, std::string>* state, std::string ts)
    : instanceState(state), timestamp(ts) {}

/**
 * @brief Destroys the WorkflowMemento and deallocates memory.
 */
WorkflowMemento::~WorkflowMemento() {
    delete instanceState;
}

/**
 * @brief Returns the state map pointer.
 */
std::map<std::string, std::string>* WorkflowMemento::getState() {
    return instanceState;
}

/**
 * @brief Returns the creation timestamp.
 */
std::string WorkflowMemento::getTimestamp() const {
    return timestamp;
}