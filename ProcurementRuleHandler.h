/**
 * @file ProcurementRuleHandler.h
 * @brief Procurement rule participant for the approval chain.
 */
#ifndef PROCUREMENT_RULE_HANDLER_H
#define PROCUREMENT_RULE_HANDLER_H

#include "ApprovalHandler.h"
class BudgetLedger;

/**
 * @class ProcurementRuleHandler
 * @brief Evaluates one procurement rule and forwards the request.
 */
class ProcurementRuleHandler : public ApprovalHandler<std::string>
{
public:
    /** A handler validates one named domain check and forwards through the existing chain. */
    ProcurementRuleHandler(const std::string& rule, BudgetLedger& ledger);
    struct CheckResult {
        std::string name;
        std::string compared;
        bool passed;
        std::string reason;
    };
    std::string processRequest(std::string amount,
                               const std::string& decision) override;
    std::string processRequest(std::string amount,
                               const std::string& decision,
                               WorkflowContext& context,
                               const std::string& stepId) override;

private:
    std::string rule;
    BudgetLedger& ledger; // borrowed, shared session balances
    CheckResult evaluate(const WorkflowContext& context) const;
};

#endif
