#ifndef APPROVAL_HANLDER_H
#define APPROVAL_HANDLER_H
#include <string>


class ApprovalHandler{
    private:
        ApprovalHandler* nextHandler;

    public:
        ApprovalHandler* setNext(ApprovalHandler* handler);
        virtual bool processRequest(double amount, std::string decision) = 0;
    };

#endif