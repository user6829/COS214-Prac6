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

static std::string askNationality()
{
    return askChoice("Applicant status:", {"South African citizen", "International applicant"});
}

static std::string askUniversityProgramme()
{
    return askChoice("Choose your undergraduate programme:",
                     {"BSc Computer Science", "BCom Accounting",
                      "BA Media Studies", "BEng Civil Engineering"});
}

static std::string askDate()
{
    while (true) {
        std::string s = askRequired("Required delivery date (DD/MM/YYYY or YYYY-MM-DD): ");
        int d=0, m=0, y=0; char a=0, b=0;
        std::istringstream in(s);
        if (s.find('/') != std::string::npos) in >> d >> a >> m >> b >> y;
        else in >> y >> a >> m >> b >> d;
        char extra = 0;
        if (!in || (in >> extra) || a != (s.find('/') != std::string::npos ? '/' : '-') || b != a) {
            std::cout << RED << "Use DD/MM/YYYY or YYYY-MM-DD.\n" << RESET; continue;
        }
        if (y < currentYear() || y > currentYear()+10 || m < 1 || m > 12 || d < 1 || d > 31) {
            std::cout << RED << "Enter a real delivery date from this year through the next 10 years.\n" << RESET; continue;
        }
        static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        int maxDay = days[m-1];
        if (m == 2 && ((y%4==0 && y%100!=0) || y%400==0)) maxDay = 29;
        if (d > maxDay) { std::cout << RED << "That date does not exist on the calendar.\n" << RESET; continue; }
        std::ostringstream out;
        out << (d<10?"0":"") << d << "/" << (m<10?"0":"") << m << "/" << y;
        return out.str();
    }
}

static std::string askQuantity()
{
    while (true) {
        std::string s = askRequired("Quantity and unit (e.g. 500 bags): ");
        std::istringstream in(s); double n=0; in >> n;
        if (in && n > 0.0 && n <= 1.0e9) {
            in >> std::ws;
            if (!in.eof()) return s;
        }
        std::cout << RED << "Enter a positive quantity followed by its unit (e.g. 500 bags, 12 tonnes).\n" << RESET;
    }
}

    static void printField(const std::string &label, const std::string &value)
{
        std::cout << "    " << label;
        if (label.size() < 20)
            std::cout << std::string(20 - label.size(), '.');
        std::cout << " " << value << "\n";
    }

static void printUniversityCheck(int number, const std::string &title,
                                     const std::vector<std::pair<std::string, std::string> > &fields,
                                     bool passed, const std::string &reason)
{
    std::cout << "\n " << number << ". " << title << "\n";
    for (size_t i = 0; i < fields.size(); ++i)
        printField(fields[i].first, fields[i].second);
    printField("Result", std::string(passed ? GREEN : RED) +
                            (passed ? "[PASS]" : "[FAIL]") + RESET);
    if (!passed)
        std::cout << "    " << reason << "\n";
}

    static Document* buildUniversityDocument(const UniversityApplication& application, const std::string& kind)
    {
        return Director::constructUniversityDocument(*engine, application.instanceId, kind == "contract");
    }

    static bool ensureDocumentsDirectory()
    {
    #ifdef _WIN32
        return _mkdir("documents") == 0 || errno == EEXIST;
    #else
        return mkdir("documents", 0755) == 0 || errno == EEXIST;
    #endif
    }

    static void saveUniversityDocument(const UniversityApplication &application,
                                       Document *document,
                                       const std::string &suffix)
    {
        if (!engine->canGenerateDocuments(application.instanceId))
            throw std::runtime_error("Workflow documents are not available");
        if (!ensureDocumentsDirectory())
        {
            std::cout << RED << "Could not create documents directory; display only.\n" << RESET;
            return;
        }
        const std::string path = "documents/" + application.studentNumber + "_" + suffix + ".txt";
        std::ofstream output(path.c_str());
        if (!output)
        {
            std::cout << RED << "Could not save " << path << "; display only.\n" << RESET;
            return;
        }
        output << document->render();
        std::cout << "Saved " << path << " (overwritten if it already existed).\n";
    }

    static void setUniversityApplicationContext(WorkflowInstance *instance,
                                                const UniversityApplication &application)
    {
        WorkflowContext *context = instance->getContext();
        context->setData("studentName", application.firstName + " " + application.surname);
        context->setData("firstName", application.firstName);
        context->setData("surname", application.surname);
        context->setData("programme", application.programme);
        context->setData("email", application.email);
        context->setData("nationality", application.nationality);
        context->setData("aps", std::to_string(application.aps));
        context->setData("englishLevel", std::to_string(application.englishLevel));
        context->setData("matricYear", std::to_string(application.matricYear));
        context->setData("subjects", joinSubjects(application.subjects));
        context->setData("studentNumber", application.studentNumber);
        context->setData("attempt", std::to_string(application.attempt));
        ProgrammeRule *rule = findProgrammeRule(application.programme);
        if (rule)
        {
            context->setData("requiredAps", std::to_string(rule->minimumAps));
            context->setData("requiredSubject", rule->requiredSubject);
            context->setData("seatsTaken", std::to_string(rule->seatsTaken));
            context->setData("capacityThreshold", std::to_string(rule->threshold));
        }
    }

    static void displayUniversityChecks(UniversityApplication& application, WorkflowInstance* instance)
    {
        application.reasons.clear();
        WorkflowContext* context = instance->getContext();
        ProgrammeRule* rule = findProgrammeRule(application.programme);
        const bool apsPass = context->getData("rule:undergrad-aps:1:passed") == "true";
        printUniversityCheck(1, "Academic score (APS)",
                             {{"Your APS", std::to_string(application.aps)},
                              {"Required", std::to_string(rule->minimumAps)}},
                             apsPass,
                             "Your APS of " + std::to_string(application.aps) +
                             " is below the " + std::to_string(rule->minimumAps) +
                             " required for this programme.");
        if (!apsPass)
            application.reasons.push_back("APS too low");
        const bool englishPass = context->getData("rule:undergrad-aps:2:passed") == "true";
        printUniversityCheck(2, "English level",
                             {{"Your level", std::to_string(application.englishLevel)},
                              {"Required", "4"}},
                             englishPass,
                             "Your English level of " + std::to_string(application.englishLevel) +
                             " is below the required level 4.");
        if (!englishPass)
            application.reasons.push_back("English level too low");
        const std::string selectedSubjects = joinSubjects(application.subjects);
        const bool subjectPass = context->getData("rule:undergrad-aps:3:passed") == "true";
        printUniversityCheck(3, "Required subjects",
                             {{"Selected", selectedSubjects},
                              {"Required", std::string(rule->requiredSubject).empty()
                                               ? "No special subject"
                                               : rule->requiredSubject}},
                             subjectPass,
                             "The required subject is not among the subjects you selected.");
        if (!subjectPass)
            application.reasons.push_back("Missing required subject: " + std::string(rule->requiredSubject));
        const bool capacityPass = context->getData("rule:undergrad-aps:4:passed") == "true";
        printUniversityCheck(4, "Programme space",
                             {{"Programme", application.programme},
                              {"Seats taken", std::to_string(rule->seatsTaken)},
                              {"Capacity", std::to_string(rule->capacity)},
                              {"Threshold", std::to_string(rule->threshold)}},
                             capacityPass,
                             "This programme is currently at capacity.");
        if (!capacityPass)
            application.reasons.push_back("Programme at capacity");

        std::cout << "------------------------------------------------------------\n"
                  << " Summary: " << (apsPass + englishPass + subjectPass + capacityPass)
                  << " of 4 checks passed.\n";
    }

    static bool processUniversityApplication(UniversityApplication &application,
                                             WorkflowInstance *instance,
                                             const std::string &resubmissionNote)
    {
        application.reasons.clear();
        setUniversityApplicationContext(instance, application);
        std::cout << "\nStep 3 of 5 - Processing application attempt " << application.attempt << "\n"
                  << "The workflow engine is checking each stage and requirement.\n"
                  << "------------------------------------------------------------\n";
        if (!resubmissionNote.empty()) std::cout << resubmissionNote << "\n";
        processAvailableWork(instance); // Main delegates ordering, checks and transitions to the facade.
        application.result = engine->getOutcome(application.instanceId);
        if (application.result != "ACCEPTED") displayUniversityChecks(application, instance);
        const bool accepted = application.result == "ACCEPTED";
        engine->recordEvent(application.instanceId,
            std::string(accepted ? "Accepted - all checks passed (attempt " : "Rejected - checks failed (attempt ") +
            std::to_string(application.attempt) + ")");
        if (accepted) ++findProgrammeRule(application.programme)->seatsTaken;
        gUniversityApplications[application.instanceId] = application;
        return accepted;
    }

    static void sendAdapterAndVisitorSummary(const std::string &label,
                                             WorkflowInstance *instance,
                                             const std::string &result);

    static void showUniversityResult(const UniversityApplication &application)
    {
        const bool accepted = application.result == "ACCEPTED";
        std::cout << "\nStep 4 of 5 - Application result\n"
                  << (accepted ? GREEN : RED)
                  << "========================================\n"
                  << "Result: " << application.result << "\n"
                  << "Attempt: " << application.attempt << "\n"
                  << "Applicant: " << application.firstName << " " << application.surname << "\n"
                  << "Student number: " << application.studentNumber << "\n"
                  << "Instance ID: " << application.instanceId << "\n"
                  << "Programme: " << application.programme << "\n";
        if (!accepted)
        {
            std::cout << "Reasons:\n";
            for (size_t i = 0; i < application.reasons.size(); ++i)
                std::cout << "- " << application.reasons[i] << "\n";
        }
        std::cout << "========================================\n" << RESET
                  << "\nStep 5 of 5 - Application complete\n"
                  << "Your application is complete. Thank you.\n";
        WorkflowInstance *instance = engine->getInstance(application.instanceId);
        if (instance)
            sendAdapterAndVisitorSummary(application.studentNumber, instance,
                                         application.result);
    }

    static UniversityApplication *findUniversityApplication(const std::string &key);
    static void menuEditUniversity(const std::string &key);

    static void attachUniversityObservers(WorkflowInstance *instance)
    {
        if (gObservers.empty())
        {
            gObservers.push_back(std::unique_ptr<EventObserver>(new NotificationObserver()));
            gObservers.push_back(std::unique_ptr<EventObserver>(new StateObserver()));
        }
        for (size_t i = 0; i < gObservers.size(); ++i)
            instance->attach(gObservers[i].get());
        instance->attach(&gUniversityHistory);
        if (instance->getContext()->getData("definitionId") == "construction-procurement")
            instance->attach(ledger);
    }

    static void menuStartUniversity()
    {
        heading("Start a university application");
        UniversityApplication application;
        application.studentNumber = generateStudentNumber();

        std::cout << "\nStep 1 of 5 - Application started\n";
        std::map<std::string, std::string> initial;
        initial["definitionId"] = "undergraduate-admissions";
        initial["studentNumber"] = application.studentNumber;
        initial["ID"] = "";
        WorkflowInstance *instance = engine->startWorkflow("undergraduate-admissions", initial);
        application.instanceId = instance->getInstanceID();
        attachUniversityObservers(instance);
        engine->recordEvent(instance->getInstanceID(), "Application received (attempt 1)");
        gUniversityApplications[application.instanceId] = application;
        std::cout << "Generated student number: " << application.studentNumber << "\n"
                  << "Workflow instance ID: " << application.instanceId << "\n";

        std::cout << "\nStep 2 of 5 - Applicant information\n";
        application.firstName = askRequired("First name: ");
        application.surname = askRequired("Surname: ");
        application.programme = askUniversityProgramme();
        application.email = askEmail();
        application.nationality = askNationality();
        application.aps = askIntRange("APS (out of 42): ", 0, 42);
        application.englishLevel = askIntRange("NSC English level (1-7): ", 1, 7);
        application.matricYear = askIntRange("Matric year: ", 1900, currentYear());
        application.subjects = askSubjects();
        std::cout << GREEN << "Confirmation email sent to " << application.email << ".\n" << RESET;
        gUniversityApplications[application.instanceId] = application;
        const bool accepted = processUniversityApplication(application, instance, "");
        showUniversityResult(application);
        if (!accepted)
        {
            const std::string answer = trim(ask("Would you like to edit your application and resubmit it now? (y/n) "));
            if (answer == "y" || answer == "Y")
                menuEditUniversity(application.instanceId);
        }
    }

    static void printEditableUniversityApplication(const UniversityApplication &application)
    {
        std::cout << "\nWhich part of your application would you like to edit?\n"
                  << "  1. First name (current: " << application.firstName << ")\n"
                  << "  2. Surname (current: " << application.surname << ")\n"
                  << "  3. Email (current: " << application.email << ")\n"
                  << "  4. Nationality (current: " << application.nationality << ")\n"
                  << "  5. Degree / programme (current: " << application.programme << ")\n"
                  << "  6. APS score (current: " << application.aps << ")\n"
                  << "  7. English level (current: " << application.englishLevel << ")\n"
                  << "  8. Matric year (current: " << application.matricYear << ")\n"
                  << "  9. High-school subjects (current: "
                  << joinSubjects(application.subjects) << ")\n"
                  << " 10. Cancel\n";
    }

    static std::string universityFieldValue(const UniversityApplication &application,
                                             int field)
    {
        switch (field)
        {
        case 1: return application.firstName;
        case 2: return application.surname;
        case 3: return application.email;
        case 4: return application.nationality;
        case 5: return application.programme;
        case 6: return std::to_string(application.aps);
        case 7: return std::to_string(application.englishLevel);
        case 8: return std::to_string(application.matricYear);
        case 9: return joinSubjects(application.subjects);
        default: return "";
        }
    }
static void menuEditUniversity(const std::string &key)
    {
        UniversityApplication *stored = findUniversityApplication(key);
        if (!stored)
        {
            std::cout << RED << "Unknown application: " << key << "\n" << RESET;
            return;
        }
        UniversityApplication original = *stored;
        UniversityApplication proposed = original;
        bool changed = false;
        while (true)
        {
            printEditableUniversityApplication(proposed);
            const int field = askIntRange("Choose a field: ", 1, 10);
            if (field == 10)
            {
                if (!changed)
                    std::cout << "No changes made.\n";
                return;
            }
            switch (field)
            {
            case 1: proposed.firstName = askRequired("First name: "); break;
            case 2: proposed.surname = askRequired("Surname: "); break;
            case 3: proposed.email = askEmail(); break;
            case 4: proposed.nationality = askNationality(); break;
            case 5: proposed.programme = askUniversityProgramme(); break;
            case 6: proposed.aps = askIntRange("APS (out of 42): ", 0, 42); break;
            case 7: proposed.englishLevel = askIntRange("NSC English level (1-7): ", 1, 7); break;
            case 8: proposed.matricYear = askIntRange("Matric year: ", 1900, currentYear()); break;
            case 9: proposed.subjects = askSubjects(); break;
            default: break;
            }
            changed = true;
            const std::string again = trim(ask("Edit another field? (y/n) "));
            if (again != "y" && again != "Y")
                break;
        }

        std::cout << "\nProposed changes:\n";
        bool anyDifference = false;
        std::string changeSummary;
        for (int field = 1; field <= 9; ++field)
        {
            const std::string before = universityFieldValue(original, field);
            const std::string after = universityFieldValue(proposed, field);
            if (before != after)
            {
                anyDifference = true;
                const char *names[] = {"", "First name", "Surname", "Email",
                                       "Nationality", "Degree / programme", "APS score",
                                       "English level", "Matric year", "High-school subjects"};
                std::cout << "  " << names[field] << ": " << before << " -> " << after << "\n";
                if (!changeSummary.empty())
                    changeSummary += "; ";
                changeSummary += names[field];
                changeSummary += ": " + before + " -> " + after;
            }
        }
        if (!anyDifference)
        {
            std::cout << "No values changed; the original application remains unchanged.\n";
            return;
        }
        const std::string confirm = trim(ask("Would you like to resubmit your application? (y/n) "));
        if (confirm != "y" && confirm != "Y")
        {
            std::cout << "Changes discarded. The original application remains unchanged.\n";
            return;
        }

        WorkflowInstance *instance = engine->getInstance(original.instanceId);
        if (!instance)
        {
            std::cout << RED << "The workflow instance could not be found.\n" << RESET;
            return;
        }
        ProgrammeRule *oldRule = findProgrammeRule(original.programme);
        if (original.result == "ACCEPTED" && oldRule && oldRule->seatsTaken > 0)
            --oldRule->seatsTaken;

        engine->rollbackInstance(original.instanceId, 0);
        proposed.attempt = original.attempt + 1;
        attachUniversityObservers(instance);
        engine->recordEvent(instance->getInstanceID(), "Application edited and resubmitted (attempt " +
                            std::to_string(proposed.attempt) + ")");
        gUniversityHistory.record(proposed.instanceId, "Inactive",
                                  "Application edited and resubmitted (attempt " +
                                  std::to_string(proposed.attempt) + "): " + changeSummary);
        *stored = proposed;
        const bool accepted = processUniversityApplication(*stored, instance,
            "Resubmission attempt " + std::to_string(proposed.attempt) +
            " uses the same workflow definition and engine checks.");
        showUniversityResult(*stored);
        if (!accepted)
            std::cout << "The latest attempt was rejected; any previously allocated seat was released.\n";
    }

    static std::string generateRequestReference()
    {
        std::ostringstream out;
        out << "REQ-" << std::setw(6) << std::setfill('0') << gRequestSequence++;
        return out.str();
    }

    static ProcurementRequest *findProcurementRequest(const std::string &key)
    {
        std::map<std::string, ProcurementRequest>::iterator direct =
            gProcurementRequests.find(key);
        if (direct != gProcurementRequests.end())
            return &direct->second;
        for (std::map<std::string, ProcurementRequest>::iterator it =
                 gProcurementRequests.begin(); it != gProcurementRequests.end(); ++it)
            if (it->second.reference == key)
                return &it->second;
        return nullptr;
    }

    static void printProcurementCheck(int number, const ProcurementCheck &check)
    {
        std::cout << "\n    " << number << ". " << check.name << "\n"
                  << "       Compared.......... " << check.compared << "\n"
                  << "       Result............. "
                  << (check.passed ? GREEN : RED)
                  << (check.passed ? "[PASS]" : "[FAIL]") << RESET << "\n";
        if (!check.passed)
            std::cout << "       " << check.reason << "\n";
    }

    static void setProcurementContext(WorkflowInstance *instance,
                                      const ProcurementRequest &request)
    {
        WorkflowContext *context = instance->getContext();
        context->setData("requestReference", request.reference);
        context->setData("requester", request.data.firstName + " " + request.data.surname);
        context->setData("firstName", request.data.firstName);
        context->setData("surname", request.data.surname);
        context->setData("requesterRole", request.data.role);
        context->setData("projectName", request.data.project);
        context->setData("material", request.data.material);
        context->setData("quantity", request.data.quantity);
        context->setData("supplier", request.data.supplier);
        context->setData("amount", std::to_string(request.data.amount));
        context->setData("deliveryDate", request.data.deliveryDate);
        context->setData("notes", request.data.notes);
    }

    static void sendAdapterAndVisitorSummary(const std::string &label,
                                             WorkflowInstance *instance,
                                             const std::string &result)
    {
        try
        {
            LegacyiHookAPI legacy;
            LegacyiHookAdapter adapter(&legacy);
            adapter.sendPayload(instance->getContext()->getAll());
            std::cout << GREEN << "Record sent to the finance system (" << label
                      << ").\n" << RESET;
        }
        catch (const std::exception &e)
        {
            std::cout << RED << "Finance-system record could not be sent: "
                      << e.what() << "\n" << RESET;
        }

        MetricsReportVisitor visitor;
        engine->exportMetrics(&visitor);
        std::cout << "Session workflow summary (" << result << "): "
                  << visitor.getMetrics() << "\n";
    }

    static Document* buildProcurementDocument(const ProcurementRequest& request)
    {
        return Director::constructProcurementDocument(*engine, request.instanceId);
    }

    static void saveProcurementDocument(const ProcurementRequest &request,
                                        const Document &document)
    {
        if (!ensureDocumentsDirectory())
        {
            std::cout << RED << "Could not create documents directory; display only.\n"
                      << RESET;
            return;
        }
        const std::string suffix = request.result == "APPROVED"
            ? "approval_letter" : "rejection_letter";
        const std::string path = "documents/" + request.reference + "_" + suffix + ".txt";
        std::ofstream output(path.c_str());
        if (!output)
        {
            std::cout << RED << "Could not save " << path << "; display only.\n" << RESET;
            return;
        }
        output << document.render();
        std::cout << "Saved " << path << " (overwritten if it already existed).\n";
    }

    static void menuProcurementDocuments()
    {
        heading("Generate procurement documents");
        if (gProcurementRequests.empty())
        {
            std::cout << "No requests started this session.\n";
            return;
        }
        std::string key = askRequired("Request reference or instance ID: ");
        ProcurementRequest *request = findProcurementRequest(key);
        if (!request)
        {
            std::cout << RED << "Unknown request: " << key << "\n" << RESET;
            return;
        }
        if (!engine->canGenerateDocuments(request->instanceId))
        {
            std::cout << RED << "Documents are available only after the request is finished.\n"
                      << RESET;
            return;
        }
        const std::string prefix = "documents/" + request->reference + "_";
        std::remove((prefix + "approval_letter.txt").c_str());
        std::remove((prefix + "rejection_letter.txt").c_str());
        Document *document = buildProcurementDocument(*request);
        document->print();
        saveProcurementDocument(*request, *document);
        delete document;
    }

    static void menuProcurementStatus()
    {
        heading("View request status");
        if (gProcurementRequests.empty())
        {
            std::cout << "No requests started this session.\n";
            return;
        }
        for (std::map<std::string, ProcurementRequest>::const_iterator it =
                 gProcurementRequests.begin(); it != gProcurementRequests.end(); ++it)
            std::cout << "  " << it->second.reference << " (" << it->first << ") - "
                      << it->second.data.firstName << " " << it->second.data.surname
                      << " - " << it->second.data.project << " - "
                      << it->second.data.material << " - R" << it->second.data.amount
                      << " - " << (it->second.result.empty() ? "IN PROGRESS" : it->second.result)
                      << "\n";
        ProcurementRequest *request =
            findProcurementRequest(askRequired("Request reference or instance ID: "));
        if (!request)
        {
            std::cout << RED << "Unknown request.\n" << RESET;
            return;
        }
        WorkflowInstance *instance = engine->getInstance(request->instanceId);
        std::cout << "Requester: " << request->data.firstName << " "
                  << request->data.surname << "\n"
                  << "Request reference: " << request->reference << "\n"
                  << "Instance ID: " << request->instanceId << "\n"
                  << "State: " << (instance ? instance->getState() : "Unknown") << "\n"
                  << "Result: " << (request->result.empty() ? "IN PROGRESS" : request->result)
                  << "\n";
        for (size_t i = 0; i < request->reasons.size(); ++i)
            std::cout << "Reason: " << request->reasons[i] << "\n";
    }

    static void menuProcurementHistory()
    {
        heading("Request history");
        if (gProcurementRequests.empty())
        {
            std::cout << "No requests started this session.\n";
            return;
        }
        ProcurementRequest *request =
            findProcurementRequest(askRequired("Request reference or instance ID: "));
        if (!request)
        {
            std::cout << RED << "Unknown request.\n" << RESET;
            return;
        }
        const std::vector<AuditLoggerObserver::Entry> *history =
            gUniversityHistory.getHistory(request->instanceId);
        std::cout << "REQUEST HISTORY - " << request->reference << " ("
                  << request->instanceId << ")\n"
                  << "Project: " << request->data.project << "  Material: "
                  << request->data.material << "  Cost: R" << request->data.amount << "\n"
                  << "------------------------------------------------------------\n"
                  << " #  Time      State              Note\n";
        if (history)
            for (size_t i = 0; i < history->size(); ++i)
                std::cout << " " << (i + 1) << "  " << (*history)[i].time
                          << "  " << (*history)[i].state << "  "
                          << (*history)[i].note << "\n";
        else
            std::cout << "No history recorded.\n";
        std::cout << "Engine snapshots: " << engine->getAuditCaretaker()->size(request->instanceId) << "\n";
        std::cout << "------------------------------------------------------------\n";
    }

    static ProcurementData collectProcurementInformation()
    {
        ProcurementData data;
        data.firstName = askRequired("First name: ");
        data.surname = askRequired("Surname: ");
        data.role = askChoice("Requester role:",
                                      {"Site manager", "Project manager",
                                       "Quantity surveyor"});
        data.project = askRequired("Construction project name: ");
        data.material = askChoice("Material required:",
                                          {"Concrete", "Cement", "Steel",
                                           "Bricks", "Aggregates", "Other"});
        if (data.material == "Other")
            data.material = askRequired("Specify the material: ");
        data.quantity = askQuantity();
        data.supplier = askChoice("Preferred supplier:",
                                          {"BuildPro Supplies", "SA Materials",
                                           "Civils Direct", "Other"});
        if (data.supplier == "Other")
            data.supplier = askRequired("Specify the supplier: ");
        data.amount = 0.0;
        data.amount = std::stod(askPositiveAmount("Estimated cost in ZAR (e.g. 125000): "));
        data.deliveryDate = askDate();
        data.notes = ask("Purpose/notes (optional): ");
        return data;
    }

