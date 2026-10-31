/** @file main.cpp
 * @brief Three scenario menus, forms and presentation using one original TIVIDY engine.
 * FixedUI and GenericUI group private UI helpers, not execution engines.
 * Domain checks, balances and document assembly remain in the existing pattern participants.
 */
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>
#include "TividyEngineFacade.h"
#include "TerminalUI.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkflowContext.h"
#include "WorkflowComponent.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"
#include "RoutingStrategy.h"
#include "EventObserver.h"
#include "NotificationObserver.h"
#include "StateObserver.h"
#include "AuditLoggerObserver.h"
#include "AuditCaretaker.h"
#include "WorkflowMemento.h"
#include "MetricsReportVisitor.h"
#include "Document.h"
#include "Director.h"
#include "LegacyiHookAdapter.h"
#include "LegacyiHookAPI.h"
#include "BudgetLedger.h"
#include "ProcurementRuleHandler.h"
#include "UniversityWorkflowDirector.h"
#include "ProcurementWorkflowDirector.h"
#include "WorkflowTemplateFactory.h"
#ifdef _WIN32
#include <direct.h>
#endif

namespace UI {
TividyEngineFacade* engine = nullptr; // borrowed from main's session
BudgetLedger* ledger = nullptr;      // borrowed; ledger outlives engine

// Presentation adapter only. Original validators receive exactly the same strings.
bool readUserInput(std::string& value, const std::string& prompt,
                   bool numeric = false, int minimum = 0, int maximum = 0)
{
    if (TerminalUI::active()) {
        if (TerminalUI::read(value, prompt, numeric, minimum, maximum)) return true;
        std::cin.setstate(std::ios::eofbit);
        return false;
    }
    std::cout << prompt;
    return static_cast<bool>(std::getline(std::cin, value));
}
}

// ---- University and procurement forms and presentation ----
namespace FixedUI
{

struct ProgrammeRule
{
    const char *name;
    int minimumAps;
    const char *requiredSubject;
    int capacity;
    int threshold;
    int seatsTaken;
};

struct UniversityApplication
{
    std::string instanceId;
    std::string studentNumber;
    std::string firstName;
    std::string surname;
    std::string programme;
    std::string email;
    std::string nationality;
    int aps;
    int englishLevel;
    int matricYear;
    std::vector<std::string> subjects;
    std::string result;
    std::vector<std::string> reasons;
    int attempt;

    UniversityApplication()
        : aps(0), englishLevel(1), matricYear(0), attempt(1)
    {
    }
};

struct ProcurementData {
    std::string firstName, surname, role, project, material, quantity, supplier;
    double amount{0.0};
    std::string deliveryDate, notes;
};
using ProcurementCheck = ProcurementRuleHandler::CheckResult;

struct ProcurementRequest
{
    std::string instanceId;
    std::string reference;
    ProcurementData data;
    std::string result;
    std::vector<std::string> reasons;
};

// =============================================================================
// Globals
// =============================================================================
using UI::engine;
using UI::ledger;
void processAvailableWork(WorkflowInstance* instance);
static std::vector<std::unique_ptr<EventObserver> > gObservers;
static AuditLoggerObserver gUniversityHistory;
static std::map<std::string, UniversityApplication> gUniversityApplications;
static std::vector<ProgrammeRule> gProgrammeRules;

static std::map<std::string, ProcurementRequest> gProcurementRequests;
static unsigned long gRequestSequence = 1;

// =============================================================================
// I/O helpers
// =============================================================================
static const char *RESET = "\033[0m";
static const char *CYAN = "\033[36m";
static const char *BLUE = "\033[34;1m";
static const char *GREEN = "\033[32;1m";
static const char *RED = "\033[31;1m";
static void rule() { std::cout << CYAN << "================================================================\n" << RESET; }
static void heading(const std::string &s)
{
    TerminalUI::title(s);
    rule();
    std::cout << BLUE << s << RESET << "\n";
    rule();
}

static std::string ask(const std::string &prompt)
{
    std::string s;
    if (!UI::readUserInput(s, prompt))
        throw std::runtime_error("Input ended before the operation was completed.");
    if (!s.empty() && s[s.size() - 1] == '\r')
        s.erase(s.size() - 1);
    return s;
}

static std::string trim(const std::string &value)
{
    const std::string whitespace = " \t\r\n";
    const std::string::size_type first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) return "";
    const std::string::size_type last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

static int currentYear()
{
    std::time_t now = std::time(nullptr);
    std::tm *local = std::localtime(&now);
    return local ? local->tm_year + 1900 : 2026;
}

static std::string askRequired(const std::string &prompt)
{
    while (true) {
        std::string value = trim(ask(prompt));
        if (!value.empty()) return value;
        std::cout << RED << "This field is required. Please enter a value.\n" << RESET;
    }
}

static int askIntRange(const std::string &prompt, int minimum, int maximum)
{
    while (true) {
        std::string raw;
        if (!UI::readUserInput(raw, prompt, true, minimum, maximum))
            throw std::runtime_error("Input ended before the operation was completed.");
        std::string s = trim(raw);
        std::istringstream iss(s);
        long long value;
        char extra;
        if ((iss >> value) && !(iss >> extra) && value >= minimum && value <= maximum)
            return static_cast<int>(value);
        std::cout << RED << "Enter a whole number from " << minimum << " to " << maximum << ".\n" << RESET;
    }
}

static std::string askChoice(const std::string &title,
                             const std::vector<std::string> &options)
{
    std::cout << title << "\n";
    for (size_t i = 0; i < options.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << options[i] << "\n";
    int choice = askIntRange("Choose: ", 1, static_cast<int>(options.size()));
    return options[static_cast<size_t>(choice - 1)];
}

static std::string askEmail()
{
    while (true) {
        std::string email = trim(ask("Email address for application confirmation: "));
        const std::string::size_type at = email.find('@');
        const std::string::size_type dot = email.find('.', at == std::string::npos ? 0 : at);
        if (at != std::string::npos && at > 0 && dot != std::string::npos &&
            dot > at + 1 && dot + 1 < email.size())
            return email;
        std::cout << RED << "Enter a valid email address, for example applicant@example.com.\n" << RESET;
    }
}

static std::string askPositiveAmount(const std::string &prompt)
{
    while (true) {
        std::string s = trim(ask(prompt));
        if (s.empty()) {
            std::cout << RED << "This field is required.\n" << RESET;
            continue;
        }
        char *end = nullptr;
        double value = std::strtod(s.c_str(), &end);
        if (end != s.c_str() && *end == '\0' && value > 0.0 && value <= 1.0e12) {
            std::ostringstream out;
            out << value;
            return out.str();
        }
        std::cout << RED << "Enter a valid positive number (digits, optional decimal point; no R or commas).\n" << RESET;
    }
}

// Student numbers are assigned by the system after an application is accepted
// into the workflow. They are not supplied by applicants.
static std::string generateStudentNumber()
{
    static unsigned long sequence = 1;
    std::ostringstream out;
    out << 'u' << (currentYear() % 100) << std::setw(6) << std::setfill('0') << sequence++;
    return out.str();
}

static void initialiseProgrammeRules()
{
    if (!gProgrammeRules.empty())
        return;
    gProgrammeRules.push_back({"BSc Computer Science", 34, "Mathematics", 120, 100, 0});
    gProgrammeRules.push_back({"BCom Accounting", 30, "Mathematics", 150, 120, 0});
    gProgrammeRules.push_back({"BA Media Studies", 28, "", 100, 95, 94}); // pre-seeded so the capacity rejection can be demonstrated
    gProgrammeRules.push_back({"BEng Civil Engineering", 36, "Mathematics", 100, 90, 0});
}

static bool validateProgrammeRules()
{
    initialiseProgrammeRules();
    for (size_t i = 0; i < gProgrammeRules.size(); ++i)
    {
        const ProgrammeRule &rule = gProgrammeRules[i];
        if (rule.capacity < 100 || rule.threshold >= rule.capacity ||
            rule.threshold < 0 || rule.seatsTaken < 0 ||
            rule.seatsTaken > rule.capacity)
        {
            std::cerr << "Internal error: invalid programme capacity data for "
                      << rule.name << ".\n";
            return false;
        }
    }
    return true;
}

static ProgrammeRule *findProgrammeRule(const std::string &programme)
{
    initialiseProgrammeRules();
    for (size_t i = 0; i < gProgrammeRules.size(); ++i)
        if (programme == gProgrammeRules[i].name)
            return &gProgrammeRules[i];
    return nullptr;
}

static bool containsSubject(const std::string &subjects, const std::string &subject)
{
    std::istringstream input(subjects);
    std::string value;
    while (std::getline(input, value, ','))
        if (trim(value) == subject)
            return true;
    return false;
}

static std::string joinSubjects(const std::vector<std::string> &subjects)
{
    std::ostringstream output;
    for (size_t i = 0; i < subjects.size(); ++i)
    {
        if (i != 0)
            output << ", ";
        output << subjects[i];
    }
    return output.str();
}

static std::vector<std::string> askSubjects()
{
    const std::vector<std::string> subjects = {
        "English Home Language", "English First Additional Language",
        "Mathematics", "Mathematical Literacy", "Physical Sciences",
        "Life Sciences", "Accounting", "Business Studies", "Geography",
        "History", "Information Technology", "Life Orientation"
    };
    std::vector<std::string> selected;
    std::cout << "Choose at least 7 NSC subjects:\n";
    while (selected.size() < 7)
    {
        std::cout << "Selected " << selected.size() << " / 7 minimum.\n";
        std::string subject = askChoice("Subject " + std::to_string(selected.size() + 1) + ": ", subjects);
        if (std::find(selected.begin(), selected.end(), subject) != selected.end())
        {
            std::cout << RED << "That subject was already selected.\n" << RESET;
            continue;
        }
        if ((subject == "Mathematics" && containsSubject(joinSubjects(selected), "Mathematical Literacy")) ||
            (subject == "Mathematical Literacy" && containsSubject(joinSubjects(selected), "Mathematics")))
        {
            std::cout << RED << "Mathematics and Mathematical Literacy cannot both be selected.\n" << RESET;
            continue;
        }
        selected.push_back(subject);
    }
    return selected;
} 
