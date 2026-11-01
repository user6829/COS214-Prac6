#include "WorkflowRule.h"
#include "WorkflowContext.h"

#include <cstdlib>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>

namespace
{
std::string trim(const std::string& value)
{
    const std::string ws = " \t\r\n";
    const std::string::size_type first = value.find_first_not_of(ws);
    if (first == std::string::npos) return "";
    const std::string::size_type last = value.find_last_not_of(ws);
    return value.substr(first, last - first + 1);
}

std::string lookup(const WorkflowContext& context, const std::string& key)
{
    try { return context.getData(key); }
    catch (...) { return ""; }
}

bool parseNumber(const std::string& text, double& out)
{
    const std::string t = trim(text);
    if (t.empty()) return false;
    char* end = nullptr;
    out = std::strtod(t.c_str(), &end);
    return end != t.c_str() && *end == '\0';
}

bool listContains(const std::string& list, const std::string& wanted)
{
    std::istringstream input(list);
    std::string item;
    while (std::getline(input, item, ','))
        if (trim(item) == trim(wanted)) return true;
    return false;
}

// Replaces {key} with the payload value (or "?" when absent).
std::string interpolate(const std::string& text, const WorkflowContext& context)
{
    std::string out;
    for (std::string::size_type i = 0; i < text.size(); ++i)
    {
        if (text[i] == '{')
        {
            const std::string::size_type close = text.find('}', i);
            if (close != std::string::npos)
            {
                const std::string key = text.substr(i + 1, close - i - 1);
                std::string value = lookup(context, key);
                out += value.empty() ? "?" : value;
                i = close;
                continue;
            }
        }
        out += text[i];
    }
    return out;
}
} // namespace

bool WorkflowRule::isSupportedOperator(const std::string& op)
{
    return op == ">=" || op == "<=" || op == ">" || op == "<" || op == "==" ||
           op == "!=" || op == "nonEmpty" || op == "contains" || op == "in";
}

WorkflowRule::WorkflowRule(const RuleSpec& ruleSpec) : spec(ruleSpec)
{
    if (spec.stepId.empty() || spec.ruleId.empty() || spec.field.empty())
        throw std::invalid_argument("Rule needs a step id, rule id and field");
    if (!isSupportedOperator(spec.op))
        throw std::invalid_argument("Rule " + spec.ruleId + ": unsupported operator '" + spec.op + "'");
    if (spec.op != "nonEmpty" && spec.operand.empty() && spec.op != "contains")
        throw std::invalid_argument("Rule " + spec.ruleId + ": operator '" + spec.op + "' needs an operand");
}

const std::string& WorkflowRule::getId() const { return spec.ruleId; }
const std::string& WorkflowRule::getStepId() const { return spec.stepId; }

RuleResult WorkflowRule::evaluate(const WorkflowContext& context) const
{
    const std::string lhs = lookup(context, spec.field);
    std::string rhs = spec.operand;
    if (!rhs.empty() && rhs[0] == '@')
        rhs = lookup(context, rhs.substr(1));

    bool passed = false;
    bool numericProblem = false;
    if (spec.op == "nonEmpty")
        passed = !trim(lhs).empty();
    else if (spec.op == "contains")
        passed = trim(rhs).empty() || listContains(lhs, rhs);
    else if (spec.op == "in")
        passed = !trim(lhs).empty() && listContains(rhs, lhs);
    else if (spec.op == "==")
        passed = trim(lhs) == trim(rhs);
    else if (spec.op == "!=")
        passed = trim(lhs) != trim(rhs);
    else
    {
        double a = 0.0, b = 0.0;
        if (!parseNumber(lhs, a) || !parseNumber(rhs, b))
            numericProblem = true;
        else if (spec.op == ">=") passed = a >= b;
        else if (spec.op == "<=") passed = a <= b;
        else if (spec.op == ">")  passed = a > b;
        else if (spec.op == "<")  passed = a < b;
    }

    RuleResult result;
    result.stepId = spec.stepId;
    result.ruleId = spec.ruleId;
    result.name = spec.name.empty() ? spec.ruleId : spec.name;
    result.passed = passed;
    result.compared = spec.compared.empty()
        ? spec.field + " " + spec.op + " " + spec.operand
        : interpolate(spec.compared, context);
    if (!passed)
    {
        if (numericProblem)
            result.reason = "'" + spec.field + "' or its comparison value is missing or not a number.";
        else
            result.reason = spec.failure.empty()
                ? "Rule '" + result.name + "' failed."
                : interpolate(spec.failure, context);
    }
    return result;
}

std::vector<RuleSpec> WorkflowRule::parse(std::istream& input, const std::string& label)
{
    std::vector<RuleSpec> specs;
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;
        const std::string text = trim(line);
        if (text.empty() || text[0] == '#') continue;

        std::vector<std::string> fields;
        std::string::size_type start = 0;
        while (true)
        {
            const std::string::size_type bar = text.find('|', start);
            if (bar == std::string::npos) { fields.push_back(trim(text.substr(start))); break; }
            fields.push_back(trim(text.substr(start, bar - start)));
            start = bar + 1;
        }
        if (fields.size() != 8)
            throw std::runtime_error(label + ":" + std::to_string(lineNumber) +
                                     ": expected 8 '|'-separated fields, found " +
                                     std::to_string(fields.size()));
        RuleSpec spec;
        spec.stepId = fields[0];   spec.ruleId = fields[1];
        spec.name = fields[2];     spec.field = fields[3];
        spec.op = fields[4];       spec.operand = fields[5];
        spec.compared = fields[6]; spec.failure = fields[7];
        try { WorkflowRule validate(spec); }
        catch (const std::exception& e)
        {
            throw std::runtime_error(label + ":" + std::to_string(lineNumber) + ": " + e.what());
        }
        specs.push_back(spec);
    }
    return specs;
}

std::vector<RuleSpec> WorkflowRule::readFile(const std::string& path)
{
    std::ifstream file(path.c_str());
    if (!file)
        throw std::runtime_error("Cannot open rule file '" + path +
                                 "'. Run TIVIDY from the project folder that contains rules/.");
    return parse(file, path);
}
