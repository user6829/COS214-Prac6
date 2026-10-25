#ifndef WORKFLOWCONTEXT_H
#define WORKFLOWCONTEXT_H

#include <map>
#include <string>

/**
 * @brief The data associated with the entire workflow
 */

template <typename A>
class WorkflowContext
{
private:
    std::map<std::string,A> dataPayload;
public:
    /**
     * @brief Gets specific data
     * @param key The keyword used to get the data 
     */
    B getData(const std::string& key){
        return dataPayload.at(key);
    };

    /**
     * @brief Put data into the payload
     * @param key keyword used to identify the data
     * @param value the value associated with that keywork can be any type 
     */
    void setData(const std::string& key, const A& value){
        dataPayload[key] = value;
    };
};

#endif