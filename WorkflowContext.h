#ifndef WORKFLOWCONTEXT_H
#define WORKFLOWCONTEXT_H

#include <map>
#include <string>

template <typename A, typename B>
class WorkflowContext
{
private:
    std::map<A,B> dataPayload;
public:
    A getData(const A& key);
    void setData(const A& key, const V& value);
};

#endif