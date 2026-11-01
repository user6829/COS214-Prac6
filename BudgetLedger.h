/** @file BudgetLedger.h
 * @brief Session-owned procurement balances and idempotent approved commitments.
 */
#ifndef BUDGET_LEDGER_H
#define BUDGET_LEDGER_H
#include "EventObserver.h"
#include <map>
#include <string>

/**
 * Owns shared material budgets, not request validation or workflow progression.
 * The procurement approval handler borrows this ledger for live funding checks.
 * Workflow instances notify it on completion; only an approved finance decision
 * can commit. The ledger must outlive its handlers and attached instances.
 */
class BudgetLedger : public EventObserver {
public:
    BudgetLedger();
    double budgetFor(const std::string& material) const;
    double committedFor(const std::string& material) const;
    double remainingFor(const std::string& material) const;
    bool canFund(const std::string& material, double amount) const;
    void update(SubjectWorkFlowInstance& subject) override;
private:
    struct Account { double budget; double committed; };
    struct Commitment { std::string material; double amount; };
    std::map<std::string, Account> accounts;
    std::map<std::string, Commitment> commitments;
};
#endif
