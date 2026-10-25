#ifndef HANDLER_A_H
#define HANLDER_A_H
#include "AppovalHandler.h"
#include <string>

template <typename T>
class HandlerA : public ApprovalHandler{
    public:
        T processRequest(T amount,T decision);
};

#endif