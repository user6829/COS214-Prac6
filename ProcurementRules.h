/**
 * @file ProcurementRules.h
 * @brief Industry-specific checks and budget policy for construction requests.
 */
#ifndef PROCUREMENT_RULES_H
#define PROCUREMENT_RULES_H

#include <map>
#include <string>
#include <vector>

/**
 * @brief Values collected from one construction procurement request.
 */
struct ProcurementData
{
    std::string firstName;
    std::string surname;
    std::string role;
    std::string project;
    std::string material;
    std::string quantity;
    std::string supplier;
    double amount;
    std::string deliveryDate;
    std::string notes;
};

/**
 * @brief Construction reference data: authority limits, approved suppliers and material budgets.
 *
 * This class no longer decides anything. It only supplies the lookup values that are
 * copied into a request's workflow payload; the pass/fail rules live in
 * rules/construction-procurement.rules and are evaluated by the engine.
 */
class ProcurementRules
{
public:
    ProcurementRules();

    /** @brief Lookup values for the payload: authorityLimit, approvedSuppliers, materialRemaining, escalationThreshold. */
    std::map<std::string, std::string> referenceData(const ProcurementData &data) const;
    /** @brief Records an approved request against its material budget. */
    void commit(const ProcurementData &data);
    double remainingFor(const std::string &material) const;
    double budgetFor(const std::string &material) const;
    double committedFor(const std::string &material) const;
    double authorityLimit(const std::string &role) const;
    double escalationThreshold() const;

private:
    struct MaterialBudget
    {
        std::string material;
        double budget;
        double committed;
    };

    std::vector<MaterialBudget> budgets;
};

#endif
