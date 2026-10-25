#ifndef HANDLER_B_H
#define HANDLER_B_H
#include "AppovalHandler.h"
#include <string>

template <typename T>
class HandlerB : public ApprovalHandler{
    public:
        T processRequest(T amount,T decision);
};
#endif