#include "WorkflowInstance.h"

WorkflowContext* WorkflowInstance::getContext() const
{
    return currentContext;
}