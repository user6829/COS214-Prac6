#ifndef APPROVAL_HANLDER_H
#define APPROVAL_HANDLER_H
#include <string>

template<typename T>
class ApprovalHandler{
    private:
        ApprovalHandler<T>* nextHandler;

    public:
        ApprovalHandler<T>* setNext(ApprovalHandler* handler);
        virtual T processRequest(T amount,T decision) = 0;
    };

#endif