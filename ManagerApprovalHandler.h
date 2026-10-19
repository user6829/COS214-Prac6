#ifndef MANAGER_APPROVAL_HANDLER_H
#define MANAGER_APPROVAL_HANDLER_H
#include "AppovalHandler.h"
#include <string>

class ManagerApprovalHandler : public ApprovalHandler{
    public:
        bool processRequest(double amount, std::string decision);
};

#endif