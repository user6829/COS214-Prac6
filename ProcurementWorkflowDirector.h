/**
 * @file ProcurementWorkflowDirector.h
 * @brief Director for the construction procurement workflow definition.
 */
#ifndef PROCUREMENT_WORKFLOW_DIRECTOR_H
#define PROCUREMENT_WORKFLOW_DIRECTOR_H

#include "Director.h"

class WorkflowDefinition;
class BudgetLedger;

/**
 * @class ProcurementWorkflowDirector
 * @brief Builds the procurement workflow using the original Builder pattern.
 */
class ProcurementWorkflowDirector : public Director
{
private:
    BudgetLedger& ledger; // borrowed by approval handlers
public:
    explicit ProcurementWorkflowDirector(BudgetLedger& balances) : ledger(balances) {}
    WorkflowDefinition* constructWorkflow() override;
};

#endif
