#include "WorkflowContext.h"

std::string WorkflowContext::getData(const std::string& key){
        return dataPayload.at(key);
}

void WorkflowContext::setData(const std::string& key, const std::string& value){
        dataPayload[key] = value;
}

const std::map<std::string, std::string> &WorkflowContext::getAll() const
{
    return dataPayload;
}
