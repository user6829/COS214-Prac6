/**
 * @file WorkflowRule.h
 * @brief Data-driven business rules that the engine evaluates against an instance payload.
 *
 * A rule is pure data: a work-item id, a payload field, an operator and an
 * operand. Rules are read from text files (see readFile) and attached to the
 * work items of a registered definition by TividyEngineFacade::loadRules, so a
 * workflow's pass/fail policy can change without touching C++ code.
 *
 * Rule file format, one rule per line, fields separated by '|', '#' starts a comment:
 *   stepId | ruleId | name | field | operator | operand | comparedText | failureText
 *
 * Operators:  >=  <=  >  <  ==  !=  nonEmpty  contains  in
 *  - Numeric operators require both sides to be numbers.
 *  - contains: the field is a comma-separated list that must contain the operand
 *              (an empty operand means "not required" and passes).
 *  - in:       the field value must appear in the comma-separated operand list.
 * Operand: a literal, or '@key' to read the operand from the instance payload.
 * comparedText / failureText may contain {key} placeholders filled from the payload.
 */
#ifndef WORKFLOWRULE_H
#define WORKFLOWRULE_H

#include <iosfwd>
#include <string>
#include <vector>

class WorkflowContext;

/** @brief Raw, unvalidated description of one rule (one line of a rule file). */
struct RuleSpec
{
    std::string stepId;
    std::string ruleId;
    std::string name;
    std::string field;
    std::string op;
    std::string operand;
    std::string compared;
    std::string failure;
};

/** @brief Outcome of evaluating one rule against one instance. */
struct RuleResult
{
    std::string stepId;
    std::string ruleId;
    std::string name;
    std::string compared;
    std::string reason; ///< Empty when the rule passed.
    bool passed;
};

/** @brief One executable rule built from a RuleSpec. */
class WorkflowRule
{
private:
    RuleSpec spec;

public:
    /** @throws std::invalid_argument If the operator is unsupported or mandatory fields are empty. */
    explicit WorkflowRule(const RuleSpec& ruleSpec);

    const std::string& getId() const;
    const std::string& getStepId() const;

    /** @brief Evaluates the rule against the payload; never throws for missing data (a missing field fails the rule). */
    RuleResult evaluate(const WorkflowContext& context) const;

    /** @brief True if @p op is one of the operators listed in the file header. */
    static bool isSupportedOperator(const std::string& op);

    /**
     * @brief Parses rules from a stream.
     * @param label Name used in error messages (usually the file name).
     * @throws std::runtime_error On a malformed line, naming the file and line number.
     */
    static std::vector<RuleSpec> parse(std::istream& input, const std::string& label);

    /** @brief Reads and parses a rule file. @throws std::runtime_error If it cannot be opened or is malformed. */
    static std::vector<RuleSpec> readFile(const std::string& path);
};

#endif // WORKFLOWRULE_H
