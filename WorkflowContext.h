#ifndef WORKFLOWCONTEXT_H
#define WORKFLOWCONTEXT_H

#include <map>
#include <string>

class WorkflowContext
{
private:
    std::map<std::string, std::string> dataPayload;
public:
    std::string getData(std::string key);
    void setData(std::string key, std::string value);
};

#endif