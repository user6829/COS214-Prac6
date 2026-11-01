#include "WorkflowContext.h"

std::string WorkflowContext::getData(const std::string& key) const{
        return dataPayload.at(key);
}

void WorkflowContext::setData(const std::string& key, const std::string& value){
        dataPayload[key] = value;
}

const std::map<std::string, std::string> &WorkflowContext::getAll() const
{
    return dataPayload;
}

void WorkflowContext::replaceAll(const std::map<std::string, std::string>& values) {
    dataPayload = values;
}
