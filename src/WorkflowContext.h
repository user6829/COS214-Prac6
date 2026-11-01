/**
 * @file WorkflowContext.h
 * @brief Per-instance payload containing workflow input and execution data.
 */
#ifndef WORKFLOWCONTEXT_H
#define WORKFLOWCONTEXT_H

#include <map>
#include <string>

/**
 * @brief The data associated with the entire workflow
 */

class WorkflowContext
{
private:
    std::map<std::string,std::string> dataPayload;
public:
    /**
     * @brief Returns a value from the workflow payload.
     * @param key Key identifying the requested value.
     * @return The stored value.
     * @throws std::out_of_range If the key is absent. 
     */
    std::string getData(const std::string& key) const;

    /**
     * @brief Put data into the payload
     * @param key keyword used to identify the data
     * @param value the value associated with that keywork can be any type 
     */
    void setData(const std::string& key, const std::string& value);

    /**
     * @brief Returns a read-only reference to the complete payload.
     * @return Reference to the internal map; no copy is made.
     */
    const std::map<std::string,std::string>& getAll() const;
    /** Replaces all payload values, used when restoring an instance snapshot. */
    void replaceAll(const std::map<std::string,std::string>& values);
};

#endif