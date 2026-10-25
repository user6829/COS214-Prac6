#ifndef WORKFLOW_MEMENTO_H
#define WORKFLOW_MEMENTO_H

#include <string>
#include <map>

/**
 * @class WorkflowMemento
 * @brief Stores a snapshot of a workflow instance's internal state and time of creation.
 */
class WorkflowMemento {
private:
    std::map<std::string, std::string>* instanceState; /**< Map storing state key-value pairs. */
    std::string timestamp;                            /**< Timestamp when the snapshot was captured. */

public:
    /**
     * @brief Constructs a WorkflowMemento object.
     * @param state Pointer to a map containing state data.
     * @param ts String representing the timestamp of creation.
     */
    WorkflowMemento(std::map<std::string, std::string>* state, std::string ts);

    /**
     * @brief Destructor that cleans up dynamically allocated state data.
     */
    ~WorkflowMemento();

    /**
     * @brief Retrieves the captured state map.
     * @return Pointer to the map containing instance state data.
     */
    std::map<std::string, std::string>* getState();

    /**
     * @brief Gets the timestamp when this memento was created.
     * @return The creation timestamp as a string.
     */
    std::string getTimestamp() const;
};

#endif