#ifndef WORKFLOWCONTEXT_H
#define WORKFLOWCONTEXT_H

#include <map>
#include <string>



template <typename A>
class WorkflowContext
{
private:
    std::map<std::string,A> dataPayload;
public:
    B getData(const std::string& key){
        return dataPayload.at(key);
    };
    void setData(const std::string& key, const A& value){
        dataPayload[key] = value;
    };
};

#endif