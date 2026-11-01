#include "BudgetLedger.h"
#include "WorkflowInstance.h"
#include <cmath>
#include <stdexcept>

BudgetLedger::BudgetLedger()
{
    accounts["Concrete"] = {1000000.0, 100000.0};
    accounts["Cement"] = {600000.0, 100000.0};
    accounts["Steel"] = {500000.0, 470000.0};
    accounts["Bricks"] = {700000.0, 100000.0};
    accounts["Aggregates"] = {800000.0, 100000.0};
}
double BudgetLedger::budgetFor(const std::string& material) const
{
    const auto found = accounts.find(material);
    return found == accounts.end() ? 0.0 : found->second.budget;
}
double BudgetLedger::committedFor(const std::string& material) const
{
    const auto found = accounts.find(material);
    return found == accounts.end() ? 0.0 : found->second.committed;
}
double BudgetLedger::remainingFor(const std::string& material) const
{ return budgetFor(material) - committedFor(material); }
bool BudgetLedger::canFund(const std::string& material, double amount) const
{
    return accounts.count(material) && std::isfinite(amount) && amount > 0.0 &&
           amount <= remainingFor(material);
}
void BudgetLedger::update(SubjectWorkFlowInstance& subject)
{
    WorkflowInstance* instance = dynamic_cast<WorkflowInstance*>(&subject);
    if (!instance || subject.getState() != "Completed" ||
        subject.getEventName() != "WORKFLOW_COMPLETED") return;
    WorkflowContext& context = *instance->getContext();
    if (context.getData("definitionId") != "construction-procurement") return;
    if (context.getData("Result") != "APPROVED" ||
        context.getData("workitem:proc-approve:status") != "APPROVED" ||
        context.getData("approval:proc-approve") != "APPROVE")
        throw std::runtime_error("Budget commitment requires approved finance work");
    const std::string material = context.getData("material");
    const std::string text = context.getData("amount");
    std::size_t parsed = 0;
    const double amount = std::stod(text, &parsed);
    if (parsed != text.size() || !std::isfinite(amount) || amount <= 0.0)
        throw std::runtime_error("Invalid budget commitment amount");
    const auto prior = commitments.find(subject.getInstanceID());
    if (prior != commitments.end()) {
        if (prior->second.material != material || prior->second.amount != amount)
            throw std::runtime_error("An existing commitment cannot be changed by replay");
        return;
    }
    if (!canFund(material, amount)) throw std::runtime_error("Insufficient funds at commitment");
    // Insert first: allocation failure cannot spend money without its identity record.
    commitments.emplace(subject.getInstanceID(), Commitment{material, amount});
    accounts.at(material).committed += amount;
    context.setData("budgetCommitted", "true");
}
