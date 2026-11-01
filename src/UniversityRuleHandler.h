/**
 * @file UniversityRuleHandler.h
 * @brief University admissions rule participant for the approval chain.
 */
#ifndef UNIVERSITY_RULE_HANDLER_H
#define UNIVERSITY_RULE_HANDLER_H

#include "ApprovalHandler.h"

/**
 * @class UniversityRuleHandler
 * @brief Evaluates one admissions rule and forwards the request.
 */
class UniversityRuleHandler : public ApprovalHandler<std::string>
{
public:
    explicit UniversityRuleHandler(const std::string& rule);
    std::string processRequest(std::string amount,
                               const std::string& decision) override;
    std::string processRequest(std::string amount,
                               const std::string& decision,
                               WorkflowContext& context,
                               const std::string& stepId) override;

private:
    std::string rule;
};

#endif
