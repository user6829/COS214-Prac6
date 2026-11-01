#include "ProcurementRules.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace
{
std::string number(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}
}

ProcurementRules::ProcurementRules()
{
    budgets.push_back({"Concrete", 1000000.0, 100000.0});
    budgets.push_back({"Cement", 600000.0, 100000.0});
    budgets.push_back({"Steel", 500000.0, 470000.0}); // pre-seeded so the insufficient-funds rejection can be demonstrated
    budgets.push_back({"Bricks", 700000.0, 100000.0});
    budgets.push_back({"Aggregates", 800000.0, 100000.0});
}

double ProcurementRules::authorityLimit(const std::string &role) const
{
    if (role == "Site manager")
        return 150000.0;
    if (role == "Quantity surveyor")
        return 300000.0;
    if (role == "Project manager")
        return 1000000.0;
    return 0.0;
}

double ProcurementRules::escalationThreshold() const
{
    return 100000.0;
}

double ProcurementRules::budgetFor(const std::string &material) const
{
    for (size_t i = 0; i < budgets.size(); ++i)
        if (budgets[i].material == material)
            return budgets[i].budget;
    return 0.0;
}

double ProcurementRules::committedFor(const std::string &material) const
{
    for (size_t i = 0; i < budgets.size(); ++i)
        if (budgets[i].material == material)
            return budgets[i].committed;
    return 0.0;
}

double ProcurementRules::remainingFor(const std::string &material) const
{
    return budgetFor(material) - committedFor(material);
}

std::map<std::string, std::string> ProcurementRules::referenceData(const ProcurementData &data) const
{
    std::map<std::string, std::string> values;
    values["authorityLimit"] = number(authorityLimit(data.role));
    values["approvedSuppliers"] = "BuildPro Supplies,SA Materials,Civils Direct";
    values["materialRemaining"] = number(remainingFor(data.material));
    values["escalationThreshold"] = number(escalationThreshold());
    return values;
}

void ProcurementRules::commit(const ProcurementData &data)
{
    for (size_t i = 0; i < budgets.size(); ++i)
    {
        if (budgets[i].material == data.material)
        {
            budgets[i].committed += data.amount;
            return;
        }
    }
}
