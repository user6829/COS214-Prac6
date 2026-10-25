#ifndef FINANCE_DIRECTOR_APPROVAL_HANDLER_H
#define FINANCE_DIRECTOR_APPROVAL_HANLDER_H
#include "AppovalHandler.h"
#include <string>

class FinanceDirectorApprovalHandler : public ApprovalHandler{
    public:
        bool processRequest(double amount, std::string decision);
};

#endif