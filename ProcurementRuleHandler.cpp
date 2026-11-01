#include "ProcurementRuleHandler.h"
#include "BudgetLedger.h"
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <limits>
namespace {
std::string value(const WorkflowContext& context, const std::string& key)
{
    try { return context.getData(key); }
    catch (const std::out_of_range&) { return ""; }
}
double amountOf(const WorkflowContext& context)
{
    const std::string text = value(context, "amount");
    try {
        std::size_t parsed = 0;
        const double amount = std::stod(text, &parsed);
        if (parsed == text.size() && std::isfinite(amount) && amount > 0.0) return amount;
    } catch (const std::exception&) {}
    return std::numeric_limits<double>::quiet_NaN();
}
std::string money(double amount)
{
    std::ostringstream out;
    out << "R" << std::fixed << std::setprecision(2) << amount;
    return out.str();
}
bool validDeliveryDate(const std::string& text)
{
    int day = 0, month = 0, year = 0;
    char first = 0, second = 0, extra = 0;
    std::istringstream input(text);
    if (text.find('/') != std::string::npos) input >> day >> first >> month >> second >> year;
    else input >> year >> first >> month >> second >> day;
    if (!input || (input >> extra) || first != (text.find('/') != std::string::npos ? '/' : '-') || second != first)
        return false;
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    const int current = local ? local->tm_year + 1900 : 2026;
    if (year < current || year > current + 10 || month < 1 || month > 12 || day < 1) return false;
    static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    const int limit = month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
        ? 29 : days[month - 1];
    return day <= limit;
}

}
ProcurementRuleHandler::ProcurementRuleHandler(const std::string& name, BudgetLedger& balances)
    : rule(name), ledger(balances)
{
    if (rule != "specification" && rule != "supplier" && rule != "authority" && rule != "funding")
        throw std::invalid_argument("Unknown procurement rule: " + rule);
}

ProcurementRuleHandler::CheckResult ProcurementRuleHandler::evaluate(const WorkflowContext& context) const
{
    const double amount = amountOf(context);
    if (rule == "specification") {
        std::istringstream quantity(value(context, "quantity"));
        double count = 0.0;
        std::string unit;
        quantity >> count >> unit;
        const bool passed = !value(context, "material").empty() && quantity &&
            std::isfinite(count) && count > 0.0 && !unit.empty() &&
            std::isfinite(amount) && validDeliveryDate(value(context, "deliveryDate"));
        return {"Specification", "Material, positive quantity and valid delivery date", passed,
            passed ? "" : "The material, quantity or delivery date is invalid."};
    }
    if (rule == "supplier") {
        const std::string supplier = value(context, "supplier");
        const bool passed = supplier == "BuildPro Supplies" || supplier == "SA Materials" || supplier == "Civils Direct";
        return {"Supplier", supplier + " is on the approved supplier list", passed,
            passed ? "" : "The selected supplier is not approved for procurement."};
    }
    if (rule == "authority") {
        const std::string role = value(context, "requesterRole");
        const double limit = role == "Site manager" ? 150000.0 : role == "Quantity surveyor" ? 300000.0 :
                             role == "Project manager" ? 1000000.0 : 0.0;
        const bool passed = std::isfinite(amount) && amount <= limit;
        return {"Requester authority", money(amount) + " <= authority limit " + money(limit), passed,
            passed ? "" : "The request exceeds your approval limit of " + money(limit) + "."};
    }
    const std::string material = value(context, "material");
    const bool passed = ledger.canFund(material, amount);
    return {"Funding", money(amount) + " requested; " + money(ledger.remainingFor(material)) +
        " remaining for " + material, passed, passed ? "" : "Insufficient funds remain for " + material + "."};
}

std::string ProcurementRuleHandler::processRequest(std::string, const std::string&)
{
    throw std::logic_error("Procurement approval requires its instance context");
}
std::string ProcurementRuleHandler::processRequest(std::string amount,
    const std::string& decision, WorkflowContext& context, const std::string& stepId)
{
    const CheckResult check = evaluate(context);
    appendRule(context, stepId, check.name, check.passed, check.compared, check.reason);
    const std::string result = check.passed && decision != "REJECT" ? decision : "REJECT";
    return getNext() ? getNext()->processRequest(amount, result, context, stepId) : result;
}
