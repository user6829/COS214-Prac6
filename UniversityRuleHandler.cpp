#include "UniversityRuleHandler.h"

#include <sstream>
#include <set>
#include <stdexcept>

namespace
{
std::string valueOrEmpty(WorkflowContext& context, const std::string& key)
{
    try { return context.getData(key); }
    catch (...) { return ""; }
}

bool hasSubject(const std::string& values, const std::string& required)
{
    std::istringstream input(values);
    std::string value;
    while (std::getline(input, value, ','))
    {
        while (!value.empty() && value[0] == ' ')
            value.erase(value.begin());
        if (value == required)
            return true;
    }
    return required.empty();
}
}

UniversityRuleHandler::UniversityRuleHandler(const std::string& name)
    : rule(name)
{
}

std::string UniversityRuleHandler::processRequest(
    std::string amount, const std::string& decision)
{
    (void)amount;
    return decision;
}

std::string UniversityRuleHandler::processRequest(
    std::string amount, const std::string& decision,
    WorkflowContext& context, const std::string& stepId)
{
    (void)amount;
    bool passed = true;
    std::string compared;
    std::string reason;
    if (rule == "aps")
    {
        const int actual = std::stoi(valueOrEmpty(context, "aps"));
        const int required = std::stoi(valueOrEmpty(context, "requiredAps"));
        passed = actual >= required;
        compared = std::to_string(actual) + " >= " + std::to_string(required);
        reason = "APS is below the programme minimum.";
    }
    else if (rule == "english")
    {
        const int actual = std::stoi(valueOrEmpty(context, "englishLevel"));
        // NSC levels denote percentage bands: level 4 begins at 50%, 5 at 60%.
        const int lower[] = {0,0,30,40,50,60,70,80};
        const int required = std::stoi(valueOrEmpty(context, "requiredEnglish"));
        passed = actual >= 1 && actual <= 7 && lower[actual] >= required;
        compared = "NSC level " + std::to_string(actual) + "; required English >= " + std::to_string(required) + "%";
        reason = "English achievement is below the programme percentage requirement.";
    }
    else if (rule == "subject")
    {
        const std::string required = valueOrEmpty(context, "requiredSubject");
        passed = (required.empty() ?
            (hasSubject(valueOrEmpty(context, "subjects"), "Mathematics") || hasSubject(valueOrEmpty(context, "subjects"), "Mathematical Literacy")) :
            hasSubject(valueOrEmpty(context, "subjects"), required)) &&
            std::stoi(valueOrEmpty(context, "mathMark")) >= std::stoi(valueOrEmpty(context, "requiredMath"));
        compared = valueOrEmpty(context, "subjects") + " contains " + required;
        compared += "; mark " + valueOrEmpty(context, "mathMark") + "% >= " + valueOrEmpty(context, "requiredMath") + "%";
        const bool correctType = required.empty() || hasSubject(valueOrEmpty(context, "subjects"), required);
        reason = correctType ? "Mathematics / Mathematical Literacy mark is below " + valueOrEmpty(context, "requiredMath") + "%." :
            "This programme requires Mathematics; Mathematical Literacy does not qualify.";
    }
    else if (rule == "capacity")
    {
        const int seats = std::stoi(valueOrEmpty(context, "seatsTaken"));
        const int threshold = std::stoi(valueOrEmpty(context, "capacityThreshold"));
        passed = seats < threshold;
        compared = std::to_string(seats) + " < " + std::to_string(threshold);
        reason = "Programme capacity has been reached.";
    }

    appendRule(context, stepId, rule, passed, compared, passed ? "" : reason);
    const std::string result = passed && decision != "REJECT" ? decision : "REJECT";
    return getNext() ? getNext()->processRequest(amount, result, context, stepId)
                     : result;
}
