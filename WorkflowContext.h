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
     * @brief Gets specific data
     * @param key The keyword used to get the data 
     */
    std::string getData(const std::string& key);

    /**
     * @brief Put data into the payload
     * @param key keyword used to identify the data
     * @param value the value associated with that keywork can be any type 
     */
    void setData(const std::string& key, const std::string& value)
};

#endif