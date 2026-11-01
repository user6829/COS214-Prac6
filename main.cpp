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
#include "ValueThresholdRouting.h"
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
// The storyboard views are fed from the existing facade and caretaker.
std::string contextValue(WorkflowInstance* instance, const std::string& key)
{
    const auto& data = instance->getContext()->getAll();
    const auto found = data.find(key);
    return found == data.end() ? "" : found->second;
}
void historyScreen(const std::string& id)
{
    TerminalUI::View view;
    view.kind="history"; view.title="Execution History"; view.heading="EXECUTION HISTORY - " + id;
    view.columns={"#", "Date / Time", "Event", "State"};
    std::string previous;
    AuditCaretaker* caretaker=engine->getAuditCaretaker();
    for(int i=0;i<caretaker->size(id);++i) {
        WorkflowMemento* saved=caretaker->getSnapshot(id,i);
        const auto* values=saved->getState();
        const std::string event=values->at("eventName"), state=values->at("state");
        if(previous==state+event) continue;
        previous=state+event;
        std::string timestamp=saved->getTimestamp();
        try {
            std::size_t parsed=0;
            const std::time_t time=static_cast<std::time_t>(std::stoll(timestamp,&parsed));
            if(parsed==timestamp.size()) {
                const std::tm* local=std::localtime(&time); char formatted[32]={};
                if(local && std::strftime(formatted,sizeof(formatted),"%Y-%m-%d %H:%M:%S",local)) timestamp=formatted;
            }
        } catch(const std::exception&) {}
        view.rows.push_back({std::to_string(view.rows.size()+1),timestamp,event,state});
        view.actions.push_back({std::to_string(view.rows.size()),"View Event"});
    }
    for(;;) {
        const std::string choice=TerminalUI::present(view);
        if(choice=="back" || view.rows.empty()) break;
        const auto& row=view.rows.at(static_cast<std::size_t>(std::stoi(choice)-1));
        TerminalUI::View detail; detail.kind="details"; detail.title="Audit Event"; detail.heading="RECORDED EVENT";
        detail.details={{"Sequence",row[0],2},{"Date / Time",row[1],8},{"Event",row[2],1},{"State",row[3],2}};
        detail.actions={{"back","Back"}}; TerminalUI::present(detail);
    }
}
void uiLeaves(WorkflowComponent* component,std::vector<LeafWorkItem*>& leaves)
{
    if(LeafWorkItem* leaf=dynamic_cast<LeafWorkItem*>(component)) { leaves.push_back(leaf); return; }
    if(CompositeStage* stage=dynamic_cast<CompositeStage*>(component))
        for(const auto& child : stage->getChildren()) uiLeaves(child.get(),leaves);
}
TerminalUI::View progressView(const std::string& id)
{
    WorkflowInstance* instance=engine->getInstance(id);
    TerminalUI::View view;
    view.kind="progress"; view.title="Workflow Instance"; view.heading="WORKFLOW PROGRESS - "+id;
    view.badge=instance->getState(); view.tone=instance->getState()=="Rejected"?5:instance->getState()=="Completed"?4:7;
    LeafWorkItem* current=engine->getNextWorkItem(id);
    std::vector<LeafWorkItem*> leaves;
    uiLeaves(engine->getDefinition(contextValue(instance,"definitionId"))->getRootElement(),leaves);
    std::vector<std::vector<std::string> > tasks;
    int activeIndex=0;
    for(LeafWorkItem* item : leaves) {
        if(item->isDeferred() || !item->isEnabled(*instance->getContext())) continue;
        const std::string status=contextValue(instance,"workitem:"+item->getId()+":status");
        const bool done=status=="COMPLETED" || status=="APPROVED" || status=="SKIPPED";
        if(item==current) activeIndex=static_cast<int>(tasks.size());
        tasks.push_back({item->getName(),done?"done":item==current?"current":"upcoming"});
        view.details.push_back({item->getName(),status.empty()?"UPCOMING":status,done?4:item==current?7:9});
    }
    // A sliding window of the real ordered tasks; no simulated parallel activity.
    if(!current) activeIndex=std::max(0,static_cast<int>(tasks.size())-1);
    const int start=std::max(0,std::min(activeIndex-1,std::max(0,static_cast<int>(tasks.size())-4)));
    for(int i=start;i<static_cast<int>(tasks.size()) && i<start+4;++i) view.rows.push_back(tasks[i]);
    std::string role=current?contextValue(instance,"assignee:"+current->getId()):"--";
    const bool routed=!role.empty();
    if(current && role.empty() && current->getRoutingStrategy()) role=current->getRoutingStrategy()->determineAssignee(*current,*instance->getContext());
    view.summary={{"Current task",current?current->getName():"No remaining work",7},
        {routed?"Assigned role":"Routing role",role,2},
        {"Workflow state",instance->getState(),view.tone}};
    view.sideTitle="CURRENT STATUS";
    view.actions={{"history","View History"},{"action","Take Action"},{"back","Back"}};
    if(!current) view.actions.erase(view.actions.begin()+1);
    return view;
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
    int mathMinimum;
    int englishMinimum;
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
    int mathMark = 0;
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
static const char *YELLOW = "\033[33;1m";
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
    gProgrammeRules.push_back({"BSc Computer Science", 34, "Mathematics", 120, 100, 0, 60, 50});
    gProgrammeRules.push_back({"BCom Accounting", 30, "Mathematics", 150, 120, 0, 50, 50});
    gProgrammeRules.push_back({"BA Media Studies", 28, "", 100, 95, 94, 50, 50}); // pre-seeded so the capacity rejection can be demonstrated
    gProgrammeRules.push_back({"BEng Civil Engineering", 36, "Mathematics", 100, 90, 0, 70, 60});
    gProgrammeRules.push_back({"BSc Information Technology",30,"Mathematics",120,100,0,50,50});
    gProgrammeRules.push_back({"BEng Electrical Engineering",36,"Mathematics",100,90,0,70,60});
    gProgrammeRules.push_back({"BSc Actuarial Science",40,"Mathematics",100,80,0,80,60});
    gProgrammeRules.push_back({"Bachelor of Education",26,"",150,130,0,50,50});
    gProgrammeRules.push_back({"Bachelor of Laws (LLB)",32,"",150,130,0,50,60});
    gProgrammeRules.push_back({"BSc Data Science",36,"Mathematics",100,90,0,70,60});
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
    selected.push_back(askChoice("Choose ONE mathematics type:", {"Mathematics", "Mathematical Literacy"}));
    std::cout << "Choose at least 7 NSC subjects:\n";
    while (selected.size() < 7)
    {
        std::cout << "Selected " << selected.size() << " / 7 minimum.\n";
        std::vector<std::string> available;
        for (const auto& candidate : subjects)
            if (candidate != "Mathematics" && candidate != "Mathematical Literacy" &&
                std::find(selected.begin(), selected.end(), candidate) == selected.end())
                available.push_back(candidate);
        std::string subject = askChoice("Subject " + std::to_string(selected.size() + 1) + ": ", available);
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
    initialiseProgrammeRules();
    std::vector<std::string> names;
    for (const auto& rule : gProgrammeRules) names.push_back(rule.name);
    if(TerminalUI::active()) {
        TerminalUI::View view;
        view.kind="programme"; view.title="New Application"; view.heading="SELECT A DEGREE PROGRAMME";
        view.sideTitle="REQUIREMENTS"; view.columns={"No.","Programme","Min APS"};
        for(std::size_t i=0;i<gProgrammeRules.size();++i) {
            const auto& rule=gProgrammeRules[i];
            view.rows.push_back({std::to_string(i+1),rule.name,std::to_string(rule.minimumAps)});
            view.actions.push_back({std::to_string(i),rule.name});
            view.rowDetails.push_back({{"",rule.name,2},{"Min APS",std::to_string(rule.minimumAps),7},
                {"Subject",std::string(rule.requiredSubject).empty()?"Mathematics OR Mathematical Literacy":rule.requiredSubject,1},
                {"Math mark",">= "+std::to_string(rule.mathMinimum)+"%",1},
                {"English",">= "+std::to_string(rule.englishMinimum)+"%",1},
                {"Seats",std::to_string(rule.seatsTaken)+" / "+std::to_string(rule.threshold),8},
                {"","Simulation requirements",8}});
            view.actions.back().value=std::to_string(i+1);
        }
        const std::string selected=TerminalUI::present(view);
        if(selected=="back") return "";
        return names.at(static_cast<std::size_t>(std::stoi(selected)-1));
    }
    return askChoice("Choose your undergraduate programme:", names);
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
        output.flush();
        if (!output) throw std::runtime_error("Could not finish saving document");
        engine->recordEvent(application.instanceId, "DOCUMENT_SAVED:" + path);
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
        context->setData("mathMark", std::to_string(application.mathMark));
        context->setData("studentNumber", application.studentNumber);
        context->setData("attempt", std::to_string(application.attempt));
        ProgrammeRule *rule = findProgrammeRule(application.programme);
        if (rule)
        {
            context->setData("requiredAps", std::to_string(rule->minimumAps));
            context->setData("requiredSubject", rule->requiredSubject);
            context->setData("requiredMath", std::to_string(rule->mathMinimum));
            context->setData("requiredEnglish", std::to_string(rule->englishMinimum));
            context->setData("seatsTaken", std::to_string(rule->seatsTaken));
            context->setData("capacityThreshold", std::to_string(rule->threshold));
        }
    }

    static void displayUniversityChecks(UniversityApplication& application, WorkflowInstance* instance)
    {
        application.reasons.clear();
        WorkflowContext* context = instance->getContext();
        const int count = std::stoi(context->getData("rule:undergrad-aps:count"));
        for (int i = 1; i <= count; ++i) {
            const std::string prefix = "rule:undergrad-aps:" + std::to_string(i) + ":";
            const bool passed = context->getData(prefix + "passed") == "true";
            printUniversityCheck(i, context->getData(prefix + "name"),
                {{"Comparison", context->getData(prefix + "compared")}}, passed, context->getData(prefix + "reason"));
            if (!passed) application.reasons.push_back(context->getData(prefix + "reason"));
        }
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
        if (application.result.empty()) application.result = "PENDING REVIEW";
        displayUniversityChecks(application, instance);
        const bool accepted = application.result == "ACCEPTED";
        engine->recordEvent(application.instanceId,
            std::string(accepted ? "Accepted (attempt " : application.result == "REJECTED" ? "Rejected - checks failed (attempt " : "Screening passed - pending review (attempt ") +
            std::to_string(application.attempt) + ")");
        if (accepted) ++findProgrammeRule(application.programme)->seatsTaken;
        gUniversityApplications[application.instanceId] = application;
        return accepted;
    }

    static void sendAdapterAndVisitorSummary(const std::string &label,
                                             WorkflowInstance *instance,
                                             const std::string &result);

static void availableWork(bool university, bool decide, const std::string& selectedId = "");
static void workflowScreen(const std::string& id);
static std::vector<TerminalUI::Detail> ruleDetails(WorkflowInstance* instance,const std::string& step)
{
    std::vector<TerminalUI::Detail> result;
    const std::string count=UI::contextValue(instance,"rule:"+step+":count");
    if(count.empty()) return result;
    for(int i=1;i<=std::stoi(count);++i) {
        const std::string prefix="rule:"+step+":"+std::to_string(i)+":";
        const bool passed=UI::contextValue(instance,prefix+"passed")=="true";
        const std::string name=UI::contextValue(instance,prefix+"name");
        std::string comparison=UI::contextValue(instance,prefix+"compared");
        if(step=="undergrad-aps" && name=="subject")
            comparison=(containsSubject(UI::contextValue(instance,"subjects"),"Mathematics")?"Mathematics":"Mathematical Literacy")+
                std::string(" ")+UI::contextValue(instance,"mathMark")+"% / min "+UI::contextValue(instance,"requiredMath")+"%";
        result.push_back({(passed?"[PASS] ":"[FAIL] ")+name,comparison,passed?4:5});
        const std::string reason=UI::contextValue(instance,prefix+"reason");
        if(!reason.empty()) result.push_back({"Reason",reason,5});
    }
    return result;
}
static void routingScreen(const std::string& id)
{
    WorkflowInstance* instance=engine->getInstance(id);
    LeafWorkItem* item=engine->getNextWorkItem(id);
    if(!item) return;
    const bool university=UI::contextValue(instance,"definitionId")=="undergraduate-admissions";
    TerminalUI::View view;
    view.kind="routing"; view.title="Routing Information"; view.heading=university?"ROUTING DECISION":"PROCUREMENT ROUTING";
    view.badge="Assigned to: "+UI::contextValue(instance,"assignee:"+item->getId()); view.tone=4;
    view.details={{"Reference",id,2},{"Current task",item->getName(),1},
        {"Strategy",dynamic_cast<ValueThresholdRouting*>(item->getRoutingStrategy())?"ValueThresholdRouting":"RoleBasedRouting",2},
        {"Routing rule",item->getRoutingStrategy()->describe(),2},
        {university?"Applicant APS":"Amount (ZAR)",UI::contextValue(instance,university?"aps":"amount"),7},
        {"Assigned role",UI::contextValue(instance,"assignee:"+item->getId()),4},
        {"Work-item status",UI::contextValue(instance,"workitem:"+item->getId()+":status"),7},
        {"Workflow state",instance->getState(),7}};
    if(!university) {
        view.details.insert(view.details.begin()+1,{"Material",UI::contextValue(instance,"material")+" / "+UI::contextValue(instance,"quantity"),1});
        view.details.push_back({"Finance approval","Separate finance-approver decision; live funds checked",8});
    }
    view.actions={{"continue","Continue"},{"back","Back"}};
    TerminalUI::present(view);
}
static void workflowScreen(const std::string& id)
{
    for(;;) {
        const std::string action=TerminalUI::present(UI::progressView(id));
        if(action=="history") UI::historyScreen(id);
        else if(action=="action" && engine->getNextWorkItem(id)) {
            availableWork(UI::contextValue(engine->getInstance(id),"definitionId")=="undergraduate-admissions",true,id);
        } else break;
    }
}
static void outcomeScreen(const std::string& id,bool university)
{
    WorkflowInstance* instance=engine->getInstance(id);
    const std::string outcome=engine->getOutcome(id);
    const bool pending=outcome.empty(), rejected=outcome=="REJECTED";
    const bool manual=!UI::contextValue(instance,"reviewerReason").empty();
    TerminalUI::View view;
    view.kind="result"; view.title=pending?"Application Result":"Decision Result";
    view.heading=university?"UNIVERSITY ADMISSIONS":"CONSTRUCTION PROCUREMENT";
    view.tone=rejected?5:4;
    view.badge=pending?"SCREENING PASSED":rejected?(university?"APPLICATION REJECTED":"REQUEST REJECTED"):
        university?"APPLICATION APPROVED":"REQUEST APPROVED";
    view.subtitle=pending?"Requirements passed. Submitted for human review.":rejected?
        (manual?"Rejected by the reviewer.":"Automatic screening found unmet requirements."):"The workflow is complete.";
    view.details={{"Reference",id,2},{university?"Programme":"Material",UI::contextValue(instance,university?"programme":"material"),1}};
    const std::string step=university?"undergrad-aps":UI::contextValue(instance,"workitem:proc-approve:status")=="REJECTED"?"proc-approve":"proc-quote";
    if(pending && university) {
        view.details.push_back({"APS",UI::contextValue(instance,"aps")+" / min "+UI::contextValue(instance,"requiredAps"),4});
        view.details.push_back({"Mathematics",UI::contextValue(instance,"mathMark")+"% / min "+UI::contextValue(instance,"requiredMath")+"%",4});
        view.details.push_back({"English","NSC level "+UI::contextValue(instance,"englishLevel")+" / min "+UI::contextValue(instance,"requiredEnglish")+"%",4});
    }
    if(rejected) {
        if(manual) view.details.push_back({"Reviewer reason",UI::contextValue(instance,"reviewerReason"),5});
        const auto rules=ruleDetails(instance,step);
        for(const auto& rule:rules) if(rule.tone==5) view.details.push_back(rule);
    }
    view.details.push_back({"Final result",pending?"PENDING REVIEW":outcome,pending?7:rejected?5:4});
    view.sideTitle="STATUS";
    view.summary={{"State",pending?"PENDING REVIEW":instance->getState(),pending?7:rejected?5:4},
        {"Documents",pending?"Not available until final decision":"Available from Documents menu",pending?9:4}};
    if(!pending) {
        view.summary.clear();
        view.details.push_back({"Workflow state",instance->getState(),rejected?5:4});
        view.details.push_back({"Documents","Available from the Documents menu",4});
    }
    if(pending) view.actions={{"workflow","View Workflow"},{"back","Back"}};
    else view.actions={{"history","View History"},{"details","View Details"},{"back","Back"}};
    for(;;) {
        const std::string action=TerminalUI::present(view);
        if(action=="history") UI::historyScreen(id);
        else if(action=="workflow") { routingScreen(id); workflowScreen(id); }
        else if(action=="details") {
            TerminalUI::View details; details.kind="details"; details.title="Screening Details";
            details.heading="REQUIREMENTS CHECK - "+id; details.details=ruleDetails(instance,step);
            details.actions={{"back","Back"}}; TerminalUI::present(details);
        } else break;
    }
}

    static void showUniversityResult(const UniversityApplication &application)
    {
        if(TerminalUI::active()) { outcomeScreen(application.instanceId,true); return; }
        const bool accepted = application.result == "ACCEPTED";
        std::cout << "\nStep 4 of 5 - Application result\n"
                  << (accepted ? GREEN : application.result == "PENDING REVIEW" ? YELLOW : RED)
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
                  << "Inspect Available Work for the next action, or Documents for a final outcome.\n";
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

    static bool collectApplicationForm(UniversityApplication& application)
    {
        application.programme=askUniversityProgramme();
        if(application.programme.empty()) return false;
        std::vector<std::string> others={"English Home Language","English First Additional Language","Physical Sciences",
            "Life Sciences","Accounting","Business Studies","Geography","History","Information Technology","Life Orientation"};
        std::vector<TerminalUI::Field> fields={
            {"Student number",application.studentNumber,{},true},
            {"First name"},{"Surname"},{"Programme",application.programme,{},true},
            {"Email"},{"Applicant status","South African citizen",{"South African citizen","International applicant"}},
            {"APS (0-42)"},{"Mathematics type","Mathematics",{"Mathematics","Mathematical Literacy"}},
            {"Mathematics mark (%)"},{"English NSC level (1-7)"},{"Matric year"}};
        for(int i=0;i<6;++i) fields.push_back({"School subject "+std::to_string(i+2),others[i],others});
        const auto valid=[](const std::vector<TerminalUI::Field>& f)->std::string {
            for(const auto& field:f) if(trim(field.value).empty()) return field.label+" is required.";
            for(const auto& entry:std::vector<std::pair<int,int> >{{6,42},{8,100},{9,7},{10,currentYear()}}) {
                const std::string text=trim(f[entry.first].value);
                std::istringstream in(text); int value; char extra;
                const int minimum=entry.first==10?1900:entry.first==9?1:0;
                if(!(in>>value) || (in>>extra) || value<minimum || value>entry.second)
                    return f[entry.first].label+": enter a whole number from "+std::to_string(minimum)+" to "+std::to_string(entry.second)+".";
            }
            const auto at=f[4].value.find('@'), dot=f[4].value.find('.',at==std::string::npos?0:at);
            if(at==std::string::npos || at==0 || dot==std::string::npos || dot<=at+1 || dot+1>=f[4].value.size() || f[4].value.find(' ')!=std::string::npos)
                return "Enter a valid email address, for example applicant@example.com.";
            std::set<std::string> selected;
            for(std::size_t i=11;i<f.size();++i)
                if(!selected.insert(f[i].value).second) return "Duplicate subject: "+f[i].value+". Select distinct school subjects.";
            return "";
        };
        if(!TerminalUI::form("New Application",fields,valid)) return false;
        application.firstName=trim(fields[1].value); application.surname=trim(fields[2].value);
        application.email=trim(fields[4].value); application.nationality=fields[5].value;
        application.aps=std::stoi(fields[6].value); application.mathMark=std::stoi(fields[8].value);
        application.englishLevel=std::stoi(fields[9].value); application.matricYear=std::stoi(fields[10].value);
        application.subjects={fields[7].value};
        for(std::size_t i=11;i<fields.size();++i) application.subjects.push_back(fields[i].value);
        return true;
    }

    static void menuStartUniversity()
    {
        heading("Start a university application");
        UniversityApplication application;
        application.studentNumber = generateStudentNumber();
        const bool structured=TerminalUI::active();
        if(structured && !collectApplicationForm(application)) return;

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
        if(!structured) {
        application.firstName = askRequired("First name: ");
        application.surname = askRequired("Surname: ");
        application.programme = askUniversityProgramme();
        application.email = askEmail();
        application.nationality = askNationality();
        application.aps = askIntRange("APS (out of 42): ", 0, 42);
        application.englishLevel = askIntRange("NSC English level (1-7): ", 1, 7);
        application.matricYear = askIntRange("Matric year: ", 1900, currentYear());
        application.subjects = askSubjects();
        application.mathMark = askIntRange("Mathematics / Mathematical Literacy mark (%): ", 0, 100);
        }
        std::cout << GREEN << "Confirmation email sent to " << application.email << ".\n" << RESET;
        gUniversityApplications[application.instanceId] = application;
        processUniversityApplication(application, instance, "");
        showUniversityResult(application);
        if (application.result == "REJECTED" && !TerminalUI::active())
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
                  << "  9. High-school subjects and mathematics mark (current: "
                  << joinSubjects(application.subjects) << "; mathematics mark " << application.mathMark << "% )\n"
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
        case 9: return joinSubjects(application.subjects) + "; mathematics mark " + std::to_string(application.mathMark) + "%";
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
            case 5: { const std::string programme=askUniversityProgramme(); if(!programme.empty()) proposed.programme=programme; break; }
            case 6: proposed.aps = askIntRange("APS (out of 42): ", 0, 42); break;
            case 7: proposed.englishLevel = askIntRange("NSC English level (1-7): ", 1, 7); break;
            case 8: proposed.matricYear = askIntRange("Matric year: ", 1900, currentYear()); break;
            case 9: proposed.subjects = askSubjects(); proposed.mathMark = askIntRange("Mathematics / Mathematical Literacy mark (%): ", 0, 100); break;
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
        for (const std::string kind : {"acceptance_letter", "contract", "rejection_letter"})
            std::remove(("documents/" + original.studentNumber + "_" + kind + ".txt").c_str());
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
            std::cout << "The latest attempt was screened; any previously allocated seat was released.\n";
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
        output.flush();
        if (!output) throw std::runtime_error("Could not finish saving document");
        engine->recordEvent(request.instanceId, "DOCUMENT_SAVED:" + path);
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
        if(TerminalUI::active()) { UI::historyScreen(request->instanceId); return; }
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

    static void displayProcurementResult(const ProcurementRequest& request)
    {
        if(TerminalUI::active()) { outcomeScreen(request.instanceId,false); return; }
        std::cout << "\nStep 4 of 5 - Request result\n"
                  << (request.result == "APPROVED" ? GREEN : request.result == "PENDING REVIEW" ? YELLOW : RED)
                  << "========================================\n"
                  << "Result: " << request.result << "\n"
                  << "Requester: " << request.data.firstName << " "
                  << request.data.surname << "\n"
                  << "Request reference: " << request.reference << "\n"
                  << "Instance ID: " << request.instanceId << "\n"
                  << "Project: " << request.data.project << "\n"
                  << "Material: " << request.data.material << "\n"
                  << "Quantity: " << request.data.quantity << "\n"
                  << "Estimated cost: R" << request.data.amount << "\n";
        if (request.result != "APPROVED")
            for (size_t i = 0; i < request.reasons.size(); ++i)
                std::cout << "Reason: " << request.reasons[i] << "\n";
        std::cout << "========================================\n" << RESET;
        std::cout << "\nStep 5 of 5 - Request complete\n"
                  << "Inspect Available Work for the next action, or Documents for a final outcome.\n";
    }

    static void menuStartProcurement()
    {
        heading("Start a material request");
        ProcurementRequest request;
        request.reference = generateRequestReference();
        std::cout << "\nStep 1 of 5 - Material request started\n";
        std::map<std::string, std::string> initial;
        initial["definitionId"] = "construction-procurement";
        initial["requestReference"] = request.reference;
        initial["ID"] = "";
        WorkflowInstance *instance = engine->startWorkflow("construction-procurement", initial);
        request.instanceId = instance->getInstanceID();
        attachUniversityObservers(instance);
        engine->recordEvent(instance->getInstanceID(), "Request received");
        gProcurementRequests[request.instanceId] = request;
        std::cout << "Generated request reference: " << request.reference << "\n"
                  << "Workflow instance ID: " << request.instanceId << "\n"
                  << "Your request has been registered.\n";

        std::cout << "\nStep 2 of 5 - Requester information\n";
        request.data = collectProcurementInformation();
        setProcurementContext(instance, request);
        instance->getContext()->setData(
            "remainingBudget",
            std::to_string(ledger->remainingFor(request.data.material)));
        gProcurementRequests[request.instanceId] = request;

        std::cout << "\nStep 3 of 5 - Processing request\n"
                  << "The workflow engine is processing the request in order.\n"
                  << "------------------------------------------------------------\n";
        processAvailableWork(instance);
        std::cout << "\n  3.5 Delivery and Inspection\n"
                  << "    Work item: Inspect delivered materials (proc-inspect)\n"
                  << "    State: Inactive - awaiting delivery (outside this request's approval process)\n";
        WorkflowContext* context = instance->getContext();
        const int count = std::stoi(context->getData("rule:proc-quote:count"));
        int passed = 0;
        for (int i = 1; i <= count; ++i) {
            const std::string prefix = "rule:proc-quote:" + std::to_string(i) + ":";
            ProcurementCheck check = {context->getData(prefix + "name"),
                context->getData(prefix + "compared"),
                context->getData(prefix + "passed") == "true", context->getData(prefix + "reason")};
            printProcurementCheck(i, check);
            if (!check.passed) request.reasons.push_back(check.reason);
            else ++passed;
        }
        request.result = engine->getOutcome(request.instanceId);
        if (request.result.empty()) request.result = "PENDING REVIEW";
        const bool approved = request.result == "APPROVED";
        engine->recordEvent(request.instanceId, approved ? "Request approved" : request.result == "REJECTED" ? "Request rejected" : "Screening passed - pending review");
        std::cout << "------------------------------------------------------------\n"
                  << " Summary: " << passed << " of " << count << " checks passed.\n";
        gProcurementRequests[request.instanceId] = request;

        displayProcurementResult(request);
        sendAdapterAndVisitorSummary(request.reference, instance, request.result);
    }


static std::string askUniversityInstanceId()
{
    if (gUniversityApplications.empty())
    {
        std::cout << "No applications started this session.\n";
        return "";
    }
    std::cout << "Applications started this session:\n";
    for (std::map<std::string, UniversityApplication>::const_iterator it = gUniversityApplications.begin();
         it != gUniversityApplications.end(); ++it)
        std::cout << "  " << it->second.studentNumber << " (" << it->first << ") - "
                  << it->second.firstName << " " << it->second.surname
                  << " - " << it->second.programme
                  << " - " << (it->second.result.empty() ? "IN PROGRESS" : it->second.result)
                  << "\n";
    return askRequired("Instance ID or student number: ");
}

static UniversityApplication *findUniversityApplication(const std::string &key)
{
    std::map<std::string, UniversityApplication>::iterator direct = gUniversityApplications.find(key);
    if (direct != gUniversityApplications.end())
        return &direct->second;
    for (std::map<std::string, UniversityApplication>::iterator it = gUniversityApplications.begin();
         it != gUniversityApplications.end(); ++it)
        if (it->second.studentNumber == key)
            return &it->second;
    return nullptr;
}

static void menuInspectUniversity()
{
    heading("View application status");
    const std::string key = askUniversityInstanceId();
    if (key.empty())
        return;
    UniversityApplication *application = findUniversityApplication(key);
    if (!application)
    {
        std::cout << RED << "Unknown application: " << key << "\n" << RESET;
        return;
    }
    WorkflowInstance *instance = engine->getInstance(application->instanceId);
    if(TerminalUI::active()) { workflowScreen(application->instanceId); return; }
    std::cout << "Applicant: " << application->firstName << " " << application->surname << "\n"
              << "Programme: " << application->programme << "\n"
              << "Student number: " << application->studentNumber << "\n"
              << "Instance ID: " << application->instanceId << "\n"
              << "Attempt: " << application->attempt << "\n"
              << "State: " << (instance ? instance->getState() : "Unknown") << "\n"
              << "Result: " << (application->result.empty() ? "IN PROGRESS" : application->result) << "\n";
    for (size_t i = 0; i < application->reasons.size(); ++i)
        std::cout << "Reason: " << application->reasons[i] << "\n";
}

static void menuUniversityHistory()
{
    heading("Application history");
    const std::string key = askUniversityInstanceId();
    if (key.empty())
        return;
    UniversityApplication *application = findUniversityApplication(key);
    if (!application)
    {
        std::cout << RED << "Unknown application: " << key << "\n" << RESET;
        return;
    }
    if(TerminalUI::active()) { UI::historyScreen(application->instanceId); return; }
    const std::vector<AuditLoggerObserver::Entry> *history =
        gUniversityHistory.getHistory(application->instanceId);
    std::cout << "APPLICATION HISTORY - " << application->studentNumber
              << " (" << application->instanceId << ")\n"
              << "Programme: " << application->programme << "\n"
              << "--------------------------------------------------------\n"
              << " #  Time      State              Note\n";
    if (history)
    {
        for (size_t i = 0; i < history->size(); ++i)
            std::cout << " " << (i + 1) << "  " << (*history)[i].time
                      << "  " << (*history)[i].state << "  "
                      << (*history)[i].note << "\n";
    }
    else
        std::cout << "No history recorded.\n";
    std::cout << "Engine snapshots: " << engine->getAuditCaretaker()->size(application->instanceId) << "\n";
    std::cout << "--------------------------------------------------------\n";
}

// =============================================================================
// Menu 6 — Both industries: document factories
// =============================================================================
static void menuDocuments()
{
    heading("Generate university documents");
    const std::string key = askUniversityInstanceId();
    if (key.empty())
        return;
    UniversityApplication *application = findUniversityApplication(key);
    if (!application)
    {
        std::cout << RED << "Unknown application: " << key << "\n" << RESET;
        return;
    }
    if (!engine->canGenerateDocuments(application->instanceId) ||
        (application->result != "ACCEPTED" && application->result != "REJECTED"))
    {
        std::cout << RED << "Documents are available only after the application is finished.\n" << RESET;
        return;
    }

    const std::string documentPrefix = "documents/" + application->studentNumber + "_";
    std::remove((documentPrefix + "acceptance_letter.txt").c_str());
    std::remove((documentPrefix + "contract.txt").c_str());
    std::remove((documentPrefix + "rejection_letter.txt").c_str());

    std::vector<std::pair<std::string, std::string> > requested;
    if (application->result == "ACCEPTED")
    {
        requested.push_back(std::make_pair("acceptance letter", "acceptance_letter"));
        requested.push_back(std::make_pair("university contract", "contract"));
    }
    else
        requested.push_back(std::make_pair("rejection letter", "rejection_letter"));

    for (size_t i = 0; i < requested.size(); ++i)
    {
        Document *document = buildUniversityDocument(
            *application, requested[i].second == "contract" ? "contract" : "letter");
        std::cout << "\n--- " << requested[i].first << " ---\n";
        document->print();
        saveUniversityDocument(*application, document, requested[i].second);
        delete document;
    }
}

// =============================================================================
// Menu
// =============================================================================
void printScenarioMenu(const std::string &scenario)
{
    TerminalUI::title(scenario == "university" ? "University Application" : "Construction Procurement");
    std::vector<TerminalUI::Choice> entries = {
        {"1", scenario == "university" ? "New Application" : "New Material Request"},
        {"2", "View Available Work"}, {"3", "Take Action - Review / Approve / Reject"},
        {"4", "Status"}, {"5", "Execution History"},
        {"6", scenario == "university" ? "Programme Information" : "Budget Summary"},
        {"7", "Documents"}};
    if (scenario == "university") entries.push_back({"8", "Edit / Resubmit Application"});
    entries.push_back({"0", "Back"});
    TerminalUI::View menu;
    const bool university=scenario=="university";
    menu.kind="menu"; menu.title=university?"University Admissions":"Construction Procurement";
    menu.heading=university?"UNIVERSITY ADMISSIONS":"CONSTRUCTION MATERIAL PROCUREMENT";
    menu.subtitle=university?"Manage undergraduate applications":"Material requests and purchase-order approval";
    menu.sideTitle=university?"AT A GLANCE":"OVERVIEW";
    int pendingCount=0,approvedCount=0,rejectedCount=0;
    if(university) for(const auto& entry:gUniversityApplications) {
        const std::string result=engine->getOutcome(entry.first);
        if(result=="ACCEPTED") ++approvedCount; else if(result=="REJECTED") ++rejectedCount;
        else if(engine->getNextWorkItem(entry.first)) ++pendingCount;
    }
    else for(const auto& entry:gProcurementRequests) {
        const std::string result=engine->getOutcome(entry.first);
        if(result=="APPROVED") ++approvedCount; else if(result=="REJECTED") ++rejectedCount;
        else if(engine->getNextWorkItem(entry.first)) ++pendingCount;
    }
    menu.summary={{"Pending review",std::to_string(pendingCount),7},
        {university?"Accepted":"Approved",std::to_string(approvedCount),4},
        {"Rejected",std::to_string(rejectedCount),5}};
    if(university) menu.summary.push_back({"Programmes",std::to_string(gProgrammeRules.size()),2});
    else {
        double committed=0;
        for(const std::string material:{"Concrete","Cement","Steel","Bricks","Aggregates"}) committed+=ledger->committedFor(material);
        std::ostringstream amount; amount<<"R "<<std::fixed<<std::setprecision(2)<<committed;
        menu.summary.push_back({"Committed",amount.str(),7});
    }
    TerminalUI::configure(menu);
    TerminalUI::choices(entries);
    for (const auto& entry : entries) std::cout << "  " << entry.value << ". " << entry.label << "\n";

}

namespace {
std::string stageFor(WorkflowComponent* node, const std::string& id)
{
    CompositeStage* stage = dynamic_cast<CompositeStage*>(node);
    if (!stage) return "";
    for (const auto& child : stage->getChildren()) {
        if (child->getId() == id) return stage->getName();
        const std::string nested = stageFor(child.get(), id);
        if (!nested.empty()) return nested;
    }
    return "";
}
}

void presentStep(WorkflowInstance* instance, LeafWorkItem& item, bool before,
                 const std::string& previousState)
{
    WorkflowContext* context = instance->getContext();
    const bool university = context->getData("definitionId") == "undergraduate-admissions";
    const std::string stage = stageFor(engine->getDefinition(context->getData("definitionId"))->getRootElement(), item.getId());
    if (before) {
        if (university && item.getId() == "undergrad-issue-offer")
            displayUniversityChecks(gUniversityApplications.at(instance->getInstanceID()), instance);
        std::string number = item.getId();
        if (!university) {
            const std::string ids[] = {"proc-submit", "proc-spec", "proc-quote", "proc-approve"};
            for (int i = 0; i < 4; ++i) if (item.getId() == ids[i]) number = std::to_string(i + 1);
        }
        std::string routing = item.getRoutingStrategy()->describe();
        if (item.getId() == "undergrad-submit") routing = "Applicant role; the application is being submitted.";
        else if (item.getId() == "undergrad-payfee") routing = "Fee collector role; the intake fee must be recorded.";
        else if (item.getId() == "undergrad-english") routing = "Language reviewer role; the English achievement level is checked.";
        else if (item.getId() == "undergrad-issue-offer") routing = "Faculty board role; the completed checks are ready for a decision.";
        else if (item.getId() == "proc-submit") routing = "Site manager role; the requester submits the request.";
        else if (item.getId() == "proc-spec") routing = "Quantity surveyor role; the specification is verified.";
        else if (item.getId() == "proc-approve") routing = "Finance approver role; the purchase order is reviewed.";
        else routing = item.getRoutingStrategy()->describe() + "; assigned " +
            item.getRoutingStrategy()->determineAssignee(item, *context);
        std::cout << "\n  3." << number << " " << stage << "\n"
                  << "    Work item: " << item.getName() << " (" << item.getId() << ")\n"
                  << "    Routing: " << routing << "\n";
        if (!university) std::cout << "    State before: " << previousState << "\n";
    } else {
        const std::string role = context->getData("assignee:" + item.getId());
        const std::string status = context->getData("workitem:" + item.getId() + ":status");
        std::cout << "    Routed role: " << role << "\n"
                  << "    Action: " << (status == "APPROVED" ? "Executed and approved" :
                       status == "REJECTED" ? "Rejected after validation" : "Executed") << " by " << role << "\n";
        if (university) {
            std::cout << "    State transition: " << previousState << " -> " << instance->getState() << "\n";
            gUniversityHistory.record(instance->getInstanceID(), instance->getState(),
                "Processed " + item.getId() + " (attempt " + context->getData("attempt") + ")");
        } else std::cout << "    State after: " << instance->getState() << "\n";
    }
}

void processAvailableWork(WorkflowInstance* instance)
{
    const std::string id = instance->getInstanceID();
    while (LeafWorkItem* item = engine->getNextWorkItem(id)) {
        const std::string step = item->getId();
        const bool review = step == "undergrad-aps" || step == "proc-quote" || step == "proc-approve";
        if (!review) { engine->advanceWorkflow(id); continue; }
        engine->executeStep(id, step);
        std::cout << "Screening / review: " << item->getName() << "\nRouting: "
                  << item->getRoutingStrategy()->describe() << "\nAssigned role: "
                  << instance->getContext()->getData("assignee:" + step) << "\nStatus: "
                  << instance->getContext()->getData("workitem:" + step + ":status") << "\n";
        if (instance->getState() == "DefectResolution") {
            engine->processApproval(id, step, "REJECT", instance->getContext()->getData("assignee:" + step));
        }
        break;
    }
}

static void availableWork(bool university, bool decide, const std::string& selectedId)
{
    std::vector<std::string> ids;
    if (university) { for (const auto& entry : gUniversityApplications) ids.push_back(entry.first); }
    else { for (const auto& entry : gProcurementRequests) ids.push_back(entry.first); }
    std::vector<std::string> pending;
    for (const auto& id : ids) {
        LeafWorkItem* item = engine->getNextWorkItem(id);
        if (!item) continue;
        WorkflowInstance* instance = engine->getInstance(id);
        WorkflowContext* context = instance->getContext();
        const auto data = context->getAll();
        const auto status = data.find("workitem:" + item->getId() + ":status");
        if (status == data.end() || (status->second != "PENDING_APPROVAL" && status->second != "WAITLISTED")) continue;
        pending.push_back(id);
        std::cout << id << " | " << (university ? context->getData("studentName") : context->getData("requestReference"))
                  << " | " << (university ? context->getData("programme") : context->getData("projectName"))
                  << " | " << item->getName() << " | " << context->getData("assignee:" + item->getId())
                  << " | " << context->getData("workitem:" + item->getId() + ":status")
                  << " | " << instance->getState() << "\n";
    }
    std::string id=selectedId;
    if(TerminalUI::active() && id.empty()) {
        TerminalUI::View table;
        table.kind="work"; table.title="Available Work";
        table.heading=university?"PENDING WORK ITEMS (UNIVERSITY)":"PENDING WORK ITEMS (PROCUREMENT)";
        table.columns={"No.","Reference",university?"Programme":"Material","Assigned role","State"};
        for(std::size_t i=0;i<pending.size();++i) {
            WorkflowInstance* candidate=engine->getInstance(pending[i]);
            LeafWorkItem* next=engine->getNextWorkItem(pending[i]);
            table.rows.push_back({std::to_string(i+1),pending[i],UI::contextValue(candidate,university?"programme":"material"),
                UI::contextValue(candidate,"assignee:"+next->getId()),"PENDING REVIEW"});
            table.actions.push_back({std::to_string(i+1),pending[i]});
        }
        const std::string choice=TerminalUI::present(table);
        if(choice=="refresh") { availableWork(university,decide); return; }
        if(choice=="back" || pending.empty()) return;
        id=pending.at(static_cast<std::size_t>(std::stoi(choice)-1));
    } else if(id.empty()) {
        if(pending.empty()) { std::cout<<"No pending work.\n"; return; }
        id=askChoice("Select pending instance:",pending);
    }
    WorkflowInstance* instance = engine->getInstance(id);
    WorkflowContext* context = instance->getContext();
    LeafWorkItem* item = engine->getNextWorkItem(id);
    if (university) {
        if(!TerminalUI::active()) showUniversityResult(gUniversityApplications.at(id));
        std::cout << "APS: " << context->getData("aps") << " | English NSC level: " << context->getData("englishLevel")
                  << " | Mathematics mark: " << context->getData("mathMark") << "%\nSubjects: " << context->getData("subjects") << "\n";
    } else {
        const ProcurementRequest& request = gProcurementRequests.at(id);
        if(!TerminalUI::active()) displayProcurementResult(request);
        std::cout << "Supplier: " << request.data.supplier << " | Requester role: " << request.data.role
                  << " | Available budget R" << ledger->remainingFor(request.data.material) << "\n";
    }
    std::cout << "Routing: " << item->getRoutingStrategy()->describe() << "\nAssigned role: "
              << context->getData("assignee:" + item->getId()) << "\n";
    const auto data = context->getAll();
    for (const auto& entry : data)
        if (entry.first.find("rule:" + item->getId() + ":") == 0)
            std::cout << entry.first << " = " << entry.second << "\n";
    if(TerminalUI::active() && !decide) { routingScreen(id); workflowScreen(id); return; }
    if (!decide) return;
    std::string decision;
    if(TerminalUI::active()) {
        TerminalUI::View review;
        review.kind="review"; review.title="Take Action"; review.heading=university?"REVIEW APPLICATION":"REVIEW MATERIAL REQUEST";
        review.details={{"Reference",id,2},{university?"Applicant":"Project",UI::contextValue(instance,university?"studentName":"projectName"),1},
            {university?"Programme":"Material",UI::contextValue(instance,university?"programme":"material"),1}};
        const auto rules=ruleDetails(instance,item->getId());
        review.details.insert(review.details.end(),rules.begin(),rules.end());
        if(!university) {
            review.details.push_back({"Quantity",UI::contextValue(instance,"quantity"),1});
            review.details.push_back({"Supplier",UI::contextValue(instance,"supplier"),1});
            review.details.push_back({"Requester role",UI::contextValue(instance,"requesterRole"),1});
            std::ostringstream amount,balance;
            amount<<"R "<<std::fixed<<std::setprecision(2)<<std::stod(UI::contextValue(instance,"amount"));
            balance<<"R "<<std::fixed<<std::setprecision(2)<<ledger->remainingFor(UI::contextValue(instance,"material"));
            review.details.push_back({"Estimated amount",amount.str(),1});
            review.details.push_back({"Live budget",balance.str(),7});
        }
        review.sideTitle="CURRENT STATE";
        review.summary={{"Status","PENDING REVIEW",7},{"Assigned to",UI::contextValue(instance,"assignee:"+item->getId()),2},
            {"Task",item->getName(),1}};
        review.actions={{"APPROVE","Approve"},{"REJECT","Reject"},{"Back","Cancel"}};
        decision=TerminalUI::present(review);
        if(decision=="back") return;
    } else decision = askChoice("Reviewer decision:", {"APPROVE", "REJECT", "Back"});
    if (decision == "Back") return;
    const std::string role = askChoice("Acting role (simulation, not authentication):", university ?
        std::vector<std::string>{"admissions-officer", "senior-reviewer"} :
        std::vector<std::string>{"procurement-officer", "procurement-manager", "finance-approver"});
    const std::string reason = askRequired("Decision reason: ");
    if (university) {
        ProgrammeRule* rule = findProgrammeRule(gUniversityApplications.at(id).programme);
        context->setData("seatsTaken", std::to_string(rule->seatsTaken));
    }
    engine->processApproval(id, item->getId(), decision, role);
    context->setData("reviewerReason", reason);
    engine->recordEvent(id, "MANUAL_DECISION:" + role + ":" + decision + ":" + reason);
    processAvailableWork(instance);
    const std::string result = engine->getOutcome(id);
    if (university) {
        UniversityApplication& application = gUniversityApplications.at(id);
        application.result = result.empty() ? "PENDING REVIEW" : result;
        if (result == "ACCEPTED") ++findProgrammeRule(application.programme)->seatsTaken;
        if (result == "REJECTED") { displayUniversityChecks(application, instance); application.reasons.push_back(reason); }
        showUniversityResult(application);
    } else {
        ProcurementRequest& request = gProcurementRequests.at(id);
        request.result = result.empty() ? "PENDING REVIEW" : result;
        if (result == "REJECTED") {
            request.reasons.push_back(reason);
            const std::string prefix = "rule:" + item->getId() + ":";
            const int count = std::stoi(context->getData(prefix + "count"));
            for (int i = 1; i <= count; ++i)
                if (context->getData(prefix + std::to_string(i) + ":passed") == "false")
                    request.reasons.push_back(context->getData(prefix + std::to_string(i) + ":reason"));
        }
        displayProcurementResult(request);
    }
}

void handleAction(const std::string& scenario, int action)
{
    const bool university = scenario == "university";
    switch (action) {
    case 1: if (university) menuStartUniversity(); else menuStartProcurement(); break;
    case 2: availableWork(university, false); break;
    case 3: availableWork(university, true); break;
    case 4: if (university) menuInspectUniversity(); else menuProcurementStatus(); break;
    case 5: if (university) menuUniversityHistory(); else menuProcurementHistory(); break;
    case 6:
        if(TerminalUI::active()) {
            if(university) { askUniversityProgramme(); return; }
            TerminalUI::View budget; budget.kind="history"; budget.title="Budget Summary"; budget.heading="MATERIAL BUDGET ACCOUNTS";
            budget.columns={"No.","Material","Budget / Committed","Available (R)"};
            int index=0;
            for(const std::string material:{"Concrete","Cement","Steel","Bricks","Aggregates"}) {
                std::ostringstream total,available;
                total<<std::fixed<<std::setprecision(2)<<ledger->budgetFor(material)<<" / "<<ledger->committedFor(material);
                available<<std::fixed<<std::setprecision(2)<<ledger->remainingFor(material);
                budget.rows.push_back({std::to_string(++index),material,total.str(),available.str()});
                budget.actions.push_back({"back","Back"});
            }
            TerminalUI::present(budget); return;
        }
        if (university) {
            for (const auto& rule : gProgrammeRules)
                std::cout << rule.name << " | APS " << rule.minimumAps << " | "
                          << (std::string(rule.requiredSubject).empty() ? "Mathematics OR Mathematical Literacy" : rule.requiredSubject)
                          << " >= " << rule.mathMinimum << "% | English >= " << rule.englishMinimum
                          << "% | Seats " << rule.seatsTaken << "/" << rule.threshold << " (capacity " << rule.capacity << ")\n";
            std::cout << "Simulation requirements, not official admission criteria. English level 4 = 50-59%, 5 = 60-69%, 6 = 70-79%, 7 = 80-100%.\n";
        } else {
            for (const std::string material : {"Concrete", "Cement", "Steel", "Bricks", "Aggregates"})
                std::cout << material << " | Budget R" << ledger->budgetFor(material) << " | Committed R"
                          << ledger->committedFor(material) << " | Available R" << ledger->remainingFor(material) << "\n";
        }
        break;
    case 7: if (university) menuDocuments(); else menuProcurementDocuments(); break;
    case 8: if (university) { const std::string key = askUniversityInstanceId(); if (!key.empty()) menuEditUniversity(key); } break;
    default: std::cout << "Choose one of the listed actions.\n";
    }
}

void cleanup()
{
    for (const auto& entry : gUniversityApplications) {
        WorkflowInstance* instance = engine->getInstance(entry.first);
        if (instance) {
            for (const auto& observer : gObservers) instance->detach(observer.get());
            instance->detach(&gUniversityHistory);
            instance->detach(ledger);
        }
    }
    for (const auto& entry : gProcurementRequests) {
        WorkflowInstance* instance = engine->getInstance(entry.first);
        if (instance) {
            for (const auto& observer : gObservers) instance->detach(observer.get());
            instance->detach(&gUniversityHistory);
            instance->detach(ledger);
        }
    }
    gObservers.clear();
}
} // namespace FixedUI

// ---- Configurable workflow forms and presentation ----
namespace GenericUI
{
using namespace GenericDriver;
namespace
{
struct HistoryEntry
{
    std::string timestamp;
    std::string state;
    std::string event;
    std::string note;
};

struct InstanceRecord
{
    std::string reference;
    std::string definitionId;
    std::string status;
    std::vector<std::string> completed;
    std::vector<HistoryEntry> history;
    std::string result;
    std::string documentPath;
    std::map<std::string, std::string> data;
};

std::string lower(std::string value);
std::string typeName(WorkflowType type);
std::string structureName(WorkflowStructure structure);

using UI::engine;
std::map<std::string, WorkflowDefinitionSpec> gDefinitions;
std::map<std::string, InstanceRecord> gInstances;
std::vector<std::unique_ptr<EventObserver> > gObservers;
unsigned long gDefinitionSequence = 1;


const char* GREEN = "\033[32;1m";
const char* RED = "\033[31;1m";
const char* CYAN = "\033[36m";
const char* RESET = "\033[0m";

void rule()
{
    std::cout << "------------------------------------------------------------\n";
}

void heading(const std::string& text)
{
    TerminalUI::title(text);
    rule();
    std::cout << text << "\n";
    rule();
}

std::string trim(const std::string& value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string ask(const std::string& prompt)
{
    std::string value;
    if (!UI::readUserInput(value, prompt))
        throw std::runtime_error("Input ended before the operation was completed.");
    return trim(value);
}

int askInt(const std::string& prompt, int minimum, int maximum)
{
    for (;;)
    {
        std::string raw;
        if (!UI::readUserInput(raw, prompt, true, minimum, maximum))
            throw std::runtime_error("Input ended before the operation was completed.");
        const std::string value = trim(raw);
        std::istringstream input(value);
        int result = 0;
        char extra = '\0';
        if ((input >> result) && !(input >> extra) &&
            result >= minimum && result <= maximum)
            return result;
        std::cout << "  Please enter a number from " << minimum << " to "
                  << maximum << ".\n";
    }
}

std::string required(const std::string& prompt, size_t maximum = 100)
{
    for (;;)
    {
        const std::string value = ask(prompt);
        if (!value.empty() && value.size() <= maximum)
            return value;
        std::cout << "  This value is required and may contain at most "
                  << maximum << " characters.\n";
    }
}

double number(const std::string& prompt, bool positive = true)
{
    for (;;)
    {
        const std::string value = ask(prompt);
        char* end = nullptr;
        const double result = std::strtod(value.c_str(), &end);
        if (end != value.c_str() && *end == '\0' && std::isfinite(result) && (!positive || result > 0.0))
            return result;
        std::cout << "  Please enter " << (positive ? "a positive " : "a ")
                  << "number.\n";
    }
}

std::string methodName(ExecutionMethod method)
{
    return method == ExecutionMethod::Manual ? "Manual" : "Automated";
}

std::string typeName(WorkflowType type)
{
    switch (type)
    {
    case WorkflowType::Case: return "Case";
    case WorkflowType::Project: return "Project";
    case WorkflowType::Approval: return "Approval";
    case WorkflowType::DocumentManagement: return "Document Management";
    }
    return "";
}

std::string structureName(WorkflowStructure structure)
{
    switch (structure)
    {
    case WorkflowStructure::RuleDriven: return "Rule-driven";
    case WorkflowStructure::StateMachine: return "State machine";
    case WorkflowStructure::Parallel: return "Parallel";
    case WorkflowStructure::Sequential: return "Sequential";
    }
    return "";
}

std::string fieldTypeName(FieldType type)
{
    return type == FieldType::Text ? "Text" : "Number";
}

std::string lower(std::string value)
{
    for (size_t i = 0; i < value.size(); ++i)
        value[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
    return value;
}

bool validFieldName(const std::string& value)
{
    if (value.empty())
        return false;
    for (size_t i = 0; i < value.size(); ++i)
        if (!(std::isalnum(static_cast<unsigned char>(value[i])) || value[i] == '_'))
            return false;
    const std::string key = lower(value);
    static const char* reserved[] = {
        "id", "state", "event", "assignee", "workitem", "approval", "scenario", "result", "definitionid", "budgetcommitted"
    };
    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); ++i)
        if (key == reserved[i])
            return false;
    return true;
}

std::string nowText()
{
    std::time_t now = std::time(nullptr);
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return buffer;
}

void addHistory(InstanceRecord& record, const std::string& state,
                const std::string& event, const std::string& note)
{
    record.history.push_back({nowText(), state, event, note});
}

std::vector<std::string> templateSequence(WorkflowType type,
                                          WorkflowStructure structure)
{
    return WorkflowTemplateFactory::sequence(type, structure);
}

std::string signature(const WorkflowDefinitionSpec& spec)
{
    std::ostringstream result;
    for (size_t i = 0; i < spec.sequence.size(); ++i)
        result << "|" << spec.sequence[i];
    return result.str();
}

void verifyTemplates()
{
    std::set<std::string> signatures;
    for (int type = 0; type < 4; ++type)
        for (int structure = 0; structure < 4; ++structure)
        {
            WorkflowDefinitionSpec spec;
            spec.type = static_cast<WorkflowType>(type);
            spec.structure = static_cast<WorkflowStructure>(structure);
            spec.sequence = templateSequence(spec.type, spec.structure);
            if (!signatures.insert(signature(spec)).second)
                std::cout << "Internal error: duplicate workflow template.\n";
        }
}

void collectLeaves(WorkflowComponent* node, std::vector<LeafWorkItem*>& leaves)
{
    CompositeStage* stage = dynamic_cast<CompositeStage*>(node);
    if (stage)
    {
        const std::vector<std::shared_ptr<WorkflowComponent> >& children =
            stage->getChildren();
        for (size_t i = 0; i < children.size(); ++i)
            collectLeaves(children[i].get(), leaves);
        return;
    }
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(node);
    if (leaf)
        leaves.push_back(leaf);
}

void attachObservers(WorkflowInstance* instance)
{
    if (gObservers.empty())
    {
        gObservers.push_back(std::unique_ptr<EventObserver>(new NotificationObserver()));
        gObservers.push_back(std::unique_ptr<EventObserver>(new StateObserver()));
        gObservers.push_back(std::unique_ptr<EventObserver>(new AuditLoggerObserver()));
    }
    for (size_t i = 0; i < gObservers.size(); ++i)
        instance->attach(gObservers[i].get());
}

WorkflowDefinitionSpec* findDefinition(const std::string& id)
{
    std::map<std::string, WorkflowDefinitionSpec>::iterator it = gDefinitions.find(id);
    return it == gDefinitions.end() ? nullptr : &it->second;
}

std::string tuple(const WorkflowDefinitionSpec& spec)
{
    return methodName(spec.method) + " / " + typeName(spec.type) + " / " +
           structureName(spec.structure);
}

std::string statusColour(const std::string& status)
{
    return status == "Completed" ? GREEN : (status == "Rejected" ? RED : CYAN);
}

std::string createDocument(const WorkflowDefinitionSpec& spec,
                           InstanceRecord& instance)
{
    if (!engine->canGenerateDocuments(instance.reference))
        throw std::runtime_error("Result documents require a terminal engine outcome");
#ifdef _WIN32
    const int created = _mkdir("documents");
#else
    const int created = mkdir("documents", 0755);
#endif
    if (created != 0 && errno != EEXIST) throw std::runtime_error("Cannot create documents directory");
    const std::string path = "documents/" + instance.reference + "_" +
                             lower(typeName(spec.type)) + "_result.txt";
    std::map<std::string, std::string> metadata;
    metadata["document:workflow"] = spec.name;
    metadata["document:industry"] = spec.industry;
    metadata["document:tuple"] = tuple(spec);
    metadata["document:history:count"] = std::to_string(instance.history.size());
    for (std::size_t i = 0; i < instance.history.size(); ++i) {
        const std::string prefix = "document:history:" + std::to_string(i) + ":";
        metadata[prefix + "state"] = instance.history[i].state;
        metadata[prefix + "event"] = instance.history[i].event;
        metadata[prefix + "note"] = instance.history[i].note;
    }
    const bool transactional = spec.type == WorkflowType::Project || spec.type == WorkflowType::Approval;
    std::unique_ptr<Document> document(Director::constructResultDocument(*engine, instance.reference,
        metadata, spec.type == WorkflowType::DocumentManagement, transactional));
    std::ofstream output(path.c_str());
    if (!output) throw std::runtime_error("Unable to create result document: " + path);
    output << document->render();
    return path;
}

void printSequence(const WorkflowDefinitionSpec& spec)
{
    std::cout << "Workflow Sequence:\n";
    for (size_t i = 0; i < spec.sequence.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << spec.sequence[i]
                  << " - " << (spec.workItemRoles.empty() ? spec.roles[i % spec.roles.size()] : spec.workItemRoles[i])
                  << (spec.stageNames.empty() ? "" : " [Stage: " + spec.stageNames[i] + "]") << "\n";
}

void printReview(const WorkflowDefinitionSpec& spec)
{
    heading("REVIEW YOUR WORKFLOW");
    std::cout << "Execution Method: " << methodName(spec.method) << "\n"
              << "Workflow Type: " << typeName(spec.type) << "\n"
              << "Workflow Structure: " << structureName(spec.structure) << "\n"
              << "Industry: " << spec.industry << "\n"
              << "Process Name: " << spec.name << "\n"
              << "Roles:\n";
    for (size_t i = 0; i < spec.roles.size(); ++i)
        std::cout << "  - " << spec.roles[i] << "\n";
    std::cout << "Data Fields:\n";
    for (size_t i = 0; i < spec.fields.size(); ++i)
        std::cout << "  - " << spec.fields[i].name << " (" <<
            fieldTypeName(spec.fields[i].type) << ")\n";
    if (spec.structure == WorkflowStructure::Parallel)
        std::cout << "Note: branches are logically independent; the engine runs them "
                     "sequentially and then records the join.\n";
    if (spec.structure == WorkflowStructure::RuleDriven)
        std::cout << "Rule: " << spec.ruleField << " < " << spec.threshold
                  << " -> " << spec.belowPath << "; otherwise " << spec.abovePath << "\n";
    printSequence(spec);
    std::cout << "\n1. Confirm and register\n"
              << "2. Change execution method\n"
              << "3. Change workflow type\n"
              << "4. Change workflow structure\n"
              << "5. Change industry details\n"
              << "6. Cancel\n"
              << "7. Configure stages and work items\n";
}

ExecutionMethod chooseMethod()
{
    heading("STEP 1 OF 4 - PROCESS EXECUTION METHOD");
    std::cout << "1. Manual - People carry out and approve each step.\n"
              << "2. Automated - The system executes the process using configured rules.\n";
    return askInt("Choose: ", 1, 2) == 1 ? ExecutionMethod::Manual :
                                           ExecutionMethod::Automated;
}

WorkflowType chooseType()
{
    heading("STEP 2 OF 4 - PROCESS WORKFLOW TYPE");
    std::cout << "1. Case - Investigate and resolve cases, complaints or incidents.\n"
              << "2. Project - Plan and deliver projects through milestones.\n"
              << "3. Approval - Submit, evaluate and approve or reject requests.\n"
              << "4. Document Management - Create, review, publish and manage documents.\n";
    return static_cast<WorkflowType>(askInt("Choose: ", 1, 4) - 1);
}

WorkflowStructure chooseStructure()
{
    heading("STEP 3 OF 4 - PROCESS WORKFLOW STRUCTURE");
    std::cout << "1. Rule-driven - Rules choose the path.\n"
              << "2. State machine - Named states and allowed transitions.\n"
              << "3. Parallel - Independent branches complete before the join.\n"
              << "4. Sequential - Activities run in a fixed order.\n";
    const int choice = askInt("Choose: ", 1, 4);
    return choice == 1 ? WorkflowStructure::RuleDriven :
           choice == 2 ? WorkflowStructure::StateMachine :
           choice == 3 ? WorkflowStructure::Parallel :
                         WorkflowStructure::Sequential;
}

std::vector<std::string> defaultRoles(WorkflowType type)
{
    if (type == WorkflowType::Case) return {"Case Handler", "Investigator", "Supervisor"};
    if (type == WorkflowType::Project) return {"Project Coordinator", "Delivery Team", "Project Sponsor"};
    if (type == WorkflowType::Approval) return {"Requester", "Reviewer", "Approver"};
    return {"Author", "Reviewer", "Publisher"};
}

void collectFields(WorkflowDefinitionSpec& spec)
{
    const int fieldCount = askInt("Number of data fields (1-6): ", 1, 6);
    std::set<std::string> names;
    for (int i = 0; i < fieldCount; ++i)
    {
        FieldSpec field;
        for (;;)
        {
            field.name = required("  Field name: ");
            if (validFieldName(field.name) && names.insert(lower(field.name)).second)
                break;
            std::cout << "  Use unique letters, numbers and underscores only; "
                         "reserved names are not allowed.\n";
        }
        field.type = askInt("  1. Text  2. Number: ", 1, 2) == 1
            ? FieldType::Text : FieldType::Number;
        spec.fields.push_back(field);
    }

 }

void collectRoutingRule(WorkflowDefinitionSpec& spec)
{
    bool hasNumber = false;
    for (size_t i = 0; i < spec.fields.size(); ++i)
        hasNumber = hasNumber || spec.fields[i].type == FieldType::Number;
    if (spec.structure == WorkflowStructure::RuleDriven && !hasNumber)
        throw std::runtime_error("Rule-driven workflows require at least one number field.");
    if (hasNumber)
    {
        std::cout << "Configure optional routing rule? (y/n)\n";
        if (lower(ask("Choose: ")) == "y")
        {
            for (;;)
            {
                for (size_t i = 0; i < spec.fields.size(); ++i)
                    if (spec.fields[i].type == FieldType::Number)
                        std::cout << "  " << spec.fields[i].name << "\n";
                spec.ruleField = required("Numeric field: ");
                bool found = false;
                for (size_t i = 0; i < spec.fields.size(); ++i)
                    found = found || (spec.fields[i].name == spec.ruleField &&
                        spec.fields[i].type == FieldType::Number);
                if (found) break;
                std::cout << "  Choose one of the listed numeric fields.\n";
            }
            spec.threshold = number("Threshold: ");
            spec.belowPath = required("Path below threshold: ");
            spec.abovePath = required("Path at or above threshold: ");
        }
    }
    if (spec.structure == WorkflowStructure::RuleDriven && spec.ruleField.empty())
    {
        for (size_t i = 0; i < spec.fields.size(); ++i)
            if (spec.fields[i].type == FieldType::Number)
            {
                spec.ruleField = spec.fields[i].name;
                break;
            }
        spec.threshold = number("Threshold: ");
        spec.belowPath = required("Path below threshold: ");
        spec.abovePath = required("Path at or above threshold: ");
    }
}

void collectDetails(WorkflowDefinitionSpec& spec)
{
    heading("STEP 4 OF 4 - INDUSTRY APPLICATION DETAILS");
    spec.industry = required("Industry: ");
    spec.name = required("Process name: ");
    for (std::map<std::string, WorkflowDefinitionSpec>::const_iterator it = gDefinitions.begin();
         it != gDefinitions.end(); ++it)
        if (lower(it->second.name) == lower(spec.name))
            throw std::runtime_error("A workflow with that process name already exists.");

    spec.roles = defaultRoles(spec.type);
    std::cout << "Default roles:\n";
    for (size_t i = 0; i < spec.roles.size(); ++i)
    {
        const std::string replacement = ask("  " + spec.roles[i] + " (blank keeps default): ");
        if (!replacement.empty())
            spec.roles[i] = replacement;
    }

    collectFields(spec);
    collectRoutingRule(spec);
}

void configureStagesAndWorkItems(WorkflowDefinitionSpec& spec)
{
    heading("CONFIGURE STAGES AND WORK ITEMS");
    if (spec.structure == WorkflowStructure::RuleDriven)
        std::cout << "Task 2 is the rule gate; tasks 3 and 4 are the low/high alternatives. Other tasks are shared.\n";
    for (;;) {
        std::vector<std::string> names, stages, roles;
        const int stageCount = askInt("Number of stages (1-8): ", 1, 8);
        for (int stage = 0; stage < stageCount; ++stage) {
            const std::string name = required("Stage name: ");
            const int tasks = askInt("Number of work items in this stage (1-20): ", 1, 20);
            for (int task = 0; task < tasks; ++task) {
                names.push_back(required("Work item name: "));
                stages.push_back(name);
                const std::string role = ask("Assignee role (blank uses configured default): ");
                roles.push_back(role.empty() ? spec.roles[(names.size() - 1) % spec.roles.size()] : role);
            }
        }
        if (spec.structure == WorkflowStructure::RuleDriven && names.size() < 4) {
            std::cout << "A rule-driven workflow needs at least four tasks. Configure the stages again.\n";
            continue;
        }
        spec.sequence.swap(names);
        spec.stageNames.swap(stages);
        spec.workItemRoles.swap(roles);
        return;
    }
}

WorkflowDefinitionSpec defineWorkflow()
{
    WorkflowDefinitionSpec spec;
    spec.method = chooseMethod();
    spec.type = chooseType();
    spec.structure = chooseStructure();
    collectDetails(spec);
    spec.sequence = templateSequence(spec.type, spec.structure);
    for (;;)
    {
        printReview(spec);
        const int action = askInt("Choose: ", 1, 7);
        if (action == 1)
            break;
        if (action == 2) spec.method = chooseMethod();
        else if (action == 3 || action == 4 || action == 5) {
            if (action == 3) spec.type = chooseType();
            if (action == 4) spec.structure = chooseStructure();
            spec.fields.clear(); spec.ruleField.clear();
            collectDetails(spec);
            if (action != 5) {
                spec.sequence = templateSequence(spec.type, spec.structure);
                spec.stageNames.clear(); spec.workItemRoles.clear();
            }
        } else if (action == 7) configureStagesAndWorkItems(spec);
        else throw std::runtime_error("Workflow definition cancelled.");
    }
    spec.id = "WF-" + std::to_string(gDefinitionSequence++);
    return spec;
}

void printRuleEvaluation(const WorkflowDefinitionSpec& spec,
                         const InstanceRecord& instance)
{
    if (spec.ruleField.empty())
        return;
    const std::string value = instance.data.find(spec.ruleField) == instance.data.end()
        ? "" : instance.data.find(spec.ruleField)->second;
    std::cout << "    Rule: " << spec.ruleField << " = " << value
              << ", threshold = " << spec.threshold << "\n";
    char* end = nullptr;
    const double numeric = std::strtod(value.c_str(), &end);
    if (end != value.c_str() && *end == '\0')
        std::cout << "    Path: " << (numeric < spec.threshold ? spec.belowPath : spec.abovePath)
                  << " (skipped: " << (numeric < spec.threshold ? spec.abovePath : spec.belowPath)
                  << ")\n";
}

std::map<std::string, std::string> collectInstanceData(const WorkflowDefinitionSpec& spec)
{
    heading("ENTER WORKFLOW DATA");
    std::map<std::string, std::string> data;
    for (size_t i = 0; i < spec.fields.size(); ++i)
    {
        if (spec.fields[i].type == FieldType::Number)
        {
            std::ostringstream prompt;
            prompt << spec.fields[i].name << " (number): ";
            std::ostringstream value;
            value << number(prompt.str());
            data[spec.fields[i].name] = value.str();
        }
        else
            data[spec.fields[i].name] = required(spec.fields[i].name + ": ");
    }
    return data;
}

void refreshRecord(InstanceRecord& record, WorkflowInstance* instance, WorkflowDefinition* definition)
{
    record.status = instance->getState();
    record.result = engine->getOutcome(instance->getInstanceID());
    record.completed.clear();
    std::vector<LeafWorkItem*> leaves;
    collectLeaves(definition->getRootElement(), leaves);
    for (LeafWorkItem* item : leaves) {
        const auto& payload = instance->getContext()->getAll();
        const auto status = payload.find("workitem:" + item->getId() + ":status");
        if (status != payload.end() && (status->second == "COMPLETED" || status->second == "APPROVED" || status->second == "SKIPPED"))
            record.completed.push_back(item->getName());
    }
}

void presentGenericTask(const WorkflowDefinitionSpec& spec, InstanceRecord& record,
    WorkflowInstance* instance, WorkflowDefinition* definition, LeafWorkItem& item, bool before)
{
    if (before) {
        const std::string role = item.getRoutingStrategy()->determineAssignee(item, *instance->getContext());
        std::cout << "\n     Work item: " << item.getName() << "\n"
                  << "     Step ID: " << item.getId() << "\n"
                  << "     Assigned role: " << role << "\n"
                  << "     Routing reason: " << item.getRoutingStrategy()->describe() << "\n";
        if (spec.method == ExecutionMethod::Automated)
            std::cout << "     Action: automatic execution by " << role << "\n";
    } else {
        refreshRecord(record, instance, definition);
        const std::string role = instance->getContext()->getData("assignee:" + item.getId());
        addHistory(record, record.status, instance->getEventName(), item.getName() + " by " + role);
        if (spec.structure == WorkflowStructure::RuleDriven && item.getId() == spec.id + "-2")
            printRuleEvaluation(spec, record);
    }
}

void printProcessing(const WorkflowDefinitionSpec& spec, InstanceRecord& record,
                     WorkflowInstance* instance, WorkflowDefinition* definition)
{
    const std::string id = instance->getInstanceID();
    if (record.history.empty()) addHistory(record, instance->getState(), "WORKFLOW_STARTED", "Workflow instance started");
    if (spec.method == ExecutionMethod::Automated) {
        engine->runToCompletion(id, [&](LeafWorkItem& item, bool before) {
            presentGenericTask(spec, record, instance, definition, item, before);
        });
    } else {
        // This is a terminal interaction loop, not a progression algorithm:
        // available work and every action are validated/selected by the facade.
        while (LeafWorkItem* item = engine->getNextWorkItem(id)) {
            presentGenericTask(spec, record, instance, definition, *item, true);
            std::cout << "     Action: 1 Execute  2 Approve  3 Reject  4 Waitlist  5 Stop\n";
            if(TerminalUI::active()) TerminalUI::configure(UI::progressView(id));
            TerminalUI::choices({{"1", "Execute"}, {"2", "Approve"}, {"3", "Reject"}, {"4", "Waitlist"}, {"5", "Stop"}});
            const int action = askInt("     Choose action: ", 1, 5);
            if (action == 5) {
                engine->recordEvent(id, "PAUSED");
                refreshRecord(record, instance, definition);
                addHistory(record, record.status, "PAUSED", "Manual execution paused; use Resume");
                return;
            }
            if (action == 1) {
                if (!instance->getWorkItemState(item->getId()).canExecute()) {
                    std::cout << "     Work is already executed; choose Approve, Reject or Waitlist.\n";
                    continue;
                }
                engine->executeStep(id, item->getId());
            } else engine->advanceWorkflow(id, action == 3 ? "REJECT" : action == 4 ? "WAITLIST" : "APPROVE");
            presentGenericTask(spec, record, instance, definition, *item, false);
            if (instance->getState() == "WAITLISTED") return;
        }
    }
    refreshRecord(record, instance, definition);
    if (engine->canGenerateDocuments(id)) {
        addHistory(record, record.status, instance->getEventName(), record.result);
        std::cout << statusColour(record.status) << "\n  Result: " << record.result << "\n" << RESET;
    }
}

void presentGenericCompletion(const WorkflowDefinitionSpec& spec, InstanceRecord& stored, WorkflowInstance* instance)
{
    if (!engine->canGenerateDocuments(instance->getInstanceID())) return;
    stored.documentPath = createDocument(spec, stored);
    std::cout << "Result document: " << stored.documentPath << "\n";
    if(TerminalUI::active()) {
        TerminalUI::View view=UI::progressView(instance->getInstanceID());
        view.details.insert(view.details.begin(),{"Document",stored.documentPath,4});
        view.actions={{"history","View History"},{"back","Back"}};
        if(TerminalUI::present(view)=="history") UI::historyScreen(instance->getInstanceID());
    }
    LegacyiHookAPI legacy;
    LegacyiHookAdapter adapter(&legacy);
    adapter.sendPayload(instance->getContext()->getAll());
    MetricsReportVisitor visitor;
    engine->exportMetrics(&visitor);
    std::cout << "Visitor summary:\n";
    visitor.displayMetrics();
}

void runInstance(const std::string& definitionId)
{
    WorkflowDefinitionSpec* spec = findDefinition(definitionId);
    if (!spec)
    {
        std::cout << "  Workflow definition not found.\n";
        return;
    }
    InstanceRecord record;
    record.definitionId = definitionId;
    record.status = "Not Started";
    record.data = collectInstanceData(*spec);
    record.data["scenario"] = spec->name;
    WorkflowInstance* instance = engine->startWorkflow(
        definitionId, record.data);
    record.reference = instance->getInstanceID();
    attachObservers(instance);
    record.status = "In Progress";
    gInstances[record.reference] = record;
    WorkflowDefinition* definition = engine->getDefinition(definitionId);
    heading("WORKFLOW PROCESSING - " + spec->name);
    std::cout << "Reference: " << record.reference << "\n"
              << "Tuple: " << tuple(*spec) << "\n";
    printProcessing(*spec, gInstances[record.reference], instance, definition);
    InstanceRecord& stored = gInstances[record.reference];
    if (stored.status == "Completed") presentGenericCompletion(*spec, stored, instance);
}

void chooseAndRun()
{
    if (gDefinitions.empty())
    {
        std::cout << "No workflows are defined. Choose option 1 first.\n";
        return;
    }
    heading("SELECT A DEFINED WORKFLOW");
    int number = 1;
    std::vector<std::string> ids;
    for (std::map<std::string, WorkflowDefinitionSpec>::const_iterator it = gDefinitions.begin();
         it != gDefinitions.end(); ++it)
    {
        std::cout << number++ << ". " << it->second.name << " | "
                  << it->second.industry << " | " << tuple(it->second) << "\n";
        ids.push_back(it->first);
    }
    const int selected = askInt("Choose: ", 1, static_cast<int>(ids.size()));
    runInstance(ids[static_cast<size_t>(selected - 1)]);
}

void viewDefinitions()
{
    heading("DEFINED WORKFLOWS");
    if (gDefinitions.empty()) { std::cout << "No workflows are defined.\n"; return; }
    for (std::map<std::string, WorkflowDefinitionSpec>::const_iterator it = gDefinitions.begin();
         it != gDefinitions.end(); ++it)
    {
        std::cout << it->second.id << " | " << it->second.name << " | "
                  << it->second.industry << " | " << tuple(it->second) << "\n";
        printSequence(it->second);
    }
}

void viewStatus()
{
    heading("WORKFLOW STATUS");
    if (gInstances.empty()) { std::cout << "No workflow instances exist.\n"; return; }
    if(TerminalUI::active()) {
        std::vector<std::string> ids;
        for(const auto& entry:gInstances) ids.push_back(entry.first);
        const std::string reference=FixedUI::askChoice("Select workflow instance:",ids);
        const std::string action=TerminalUI::present(UI::progressView(reference));
        if(action=="history") UI::historyScreen(reference);
        if(action=="action") {
            auto& record=gInstances.at(reference);
            printProcessing(gDefinitions.at(record.definitionId),record,engine->getInstance(reference),engine->getDefinition(record.definitionId));
        }
        return;
    }
    for (std::map<std::string, InstanceRecord>::const_iterator it = gInstances.begin();
         it != gInstances.end(); ++it)
    {
        const WorkflowDefinitionSpec& spec = gDefinitions[it->second.definitionId];
        std::cout << it->first << " | " << spec.name << " | "
                  << tuple(spec) << " | " << statusColour(it->second.status)
                  << it->second.status << RESET << "\n"
                  << "  Completed: " << it->second.completed.size()
                  << "  Remaining: " << (spec.sequence.size() - it->second.completed.size())
                  << "\n";
    }
}

void viewHistory()
{
    heading("WORKFLOW HISTORY");
    const std::string reference = required("Instance reference: ");
    std::map<std::string, InstanceRecord>::const_iterator it = gInstances.find(reference);
    if (it == gInstances.end()) { std::cout << "  Unknown instance.\n"; return; }
    if(TerminalUI::active()) { UI::historyScreen(reference); return; }
    AuditCaretaker* caretaker = engine->getAuditCaretaker();
    for (int i = 0; i < caretaker->size(reference); ++i) {
        WorkflowMemento* snapshot = caretaker->getSnapshot(reference, i);
        std::cout << "  Snapshot " << i << " | " << snapshot->getTimestamp() << " | "
                  << snapshot->getState()->at("state") << " | " << snapshot->getState()->at("eventName") << "\n";
    }
    for (size_t i = 0; i < it->second.history.size(); ++i)
        std::cout << "  " << it->second.history[i].timestamp << " | "
                  << it->second.history[i].state << " | "
                  << it->second.history[i].event << " | "
                  << it->second.history[i].note << "\n";
}

void generateDocument()
{
    heading("GENERATE WORKFLOW RESULT DOCUMENT");
    const std::string reference = required("Instance reference: ");
    std::map<std::string, InstanceRecord>::iterator it = gInstances.find(reference);
    if (it == gInstances.end()) { std::cout << "  Unknown instance.\n"; return; }
    if (it->second.status != "Completed" && it->second.status != "Rejected")
    {
        std::cout << "  The workflow is unfinished; complete it before generating a result.\n";
        return;
    }
    it->second.documentPath = createDocument(gDefinitions[it->second.definitionId], it->second);
    std::cout << "  Document regenerated: " << it->second.documentPath << "\n";
}

} // namespace

void printMenu()
{
    TerminalUI::title("Generic Workflow Builder");
    heading("GENERIC WORKFLOW DRIVER");
    TerminalUI::View menu;
    menu.kind="menu"; menu.title="Generic Workflow Builder"; menu.heading="GENERIC WORKFLOW BUILDER";
    menu.subtitle="Configure, register and execute workflow definitions"; menu.sideTitle="BUILDER TOOLS";
    menu.summary={{"+","Define stages",4},{"+","Add work items",4},{"+","Set routing",4},
        {"+","Add rules / conditions",4},{"+","Publish definition",4}};
    TerminalUI::configure(menu);

    std::cout << "1. Define a new workflow\n"
              << "2. Run a defined workflow\n"
              << "3. View defined workflows\n"
              << "4. View workflow status\n"
              << "5. View workflow history\n"
              << "6. Generate workflow result document\n"
              << "7. Choose a different workflow scenario\n"
              << "8. Exit\n"
              << "9. Resume a paused or waitlisted workflow\n";
}

void handleAction(int choice)
{
    if (choice == 1) {
        WorkflowDefinitionSpec spec = defineWorkflow();
        engine->registerDefinition(spec.id, WorkflowTemplateFactory::createDefinition(spec));
        gDefinitions[spec.id] = spec;
        std::cout << "Workflow registered as " << spec.id << ".\n";
        if (lower(ask("Run it now? (y/n): ")) == "y") runInstance(spec.id);
    } else if (choice == 2) chooseAndRun();
    else if (choice == 3) viewDefinitions();
    else if (choice == 4) viewStatus();
    else if (choice == 5) viewHistory();
    else if (choice == 6) generateDocument();
    else if (choice == 9) {
        const std::string id = required("Instance reference: ");
        auto it = gInstances.find(id);
        if (it == gInstances.end()) throw std::invalid_argument("Unknown instance");
        WorkflowInstance* instance = engine->getInstance(id);
        printProcessing(gDefinitions.at(it->second.definitionId), it->second, instance,
                        engine->getDefinition(it->second.definitionId));
        if (engine->canGenerateDocuments(id))
            presentGenericCompletion(gDefinitions.at(it->second.definitionId), it->second, instance);
    }
}

void cleanup()
{
    for (const auto& entry : gInstances) {
        WorkflowInstance* instance = engine->getInstance(entry.first);
        if (instance) for (const auto& observer : gObservers) instance->detach(observer.get());
    }
    gObservers.clear();
}
} // namespace GenericUI

// ---- Application menu and session composition ----
namespace
{
void printLauncher()
{
    TerminalUI::title("Dashboard");
    int pending = 0, completed = 0, finished = 0;
    const auto ids = UI::engine->getInstanceIds();
    for (const auto& id : ids) {
        WorkflowInstance* instance = UI::engine->getInstance(id);
        if (instance->getState() == "Completed") { ++completed; ++finished; }
        else if(instance->getState()=="Rejected") ++finished;
        else ++pending;
    }
    std::cout << "Registered workflows: " << UI::engine->getAllDefinitions().size()
              << " | Pending instances: " << pending << " | Finished instances: " << finished << "\n";
    for (std::size_t i = ids.size() > 3 ? ids.size() - 3 : 0; i < ids.size(); ++i) {
        WorkflowInstance* instance = UI::engine->getInstance(ids[i]);
        std::cout << ids[i] << " | " << instance->getState() << " | " << instance->getEventName() << "\n";
    }
    TerminalUI::View dashboard;
    dashboard.kind="menu"; dashboard.title="Dashboard"; dashboard.heading="";
    dashboard.sideTitle="SYSTEM OVERVIEW";
    dashboard.summary={{"Workflows",std::to_string(UI::engine->getAllDefinitions().size()),2},
        {"Pending Work",std::to_string(pending),7},{"Completed",std::to_string(completed),4},
        {"Engine","READY",4}};
    TerminalUI::configure(dashboard);
    TerminalUI::choices({{"1", "University Admissions"}, {"2", "Construction Procurement"},
                         {"3", "Generic Workflow Builder"}, {"0", "Exit"}});
    std::cout << "================================================================\n"
              << "  TIVIDY - choose a workflow\n"
              << "================================================================\n"
              << "  1. Scenario 1 - University application\n"
              << "     Apply for an undergraduate programme\n"
              << "  2. Scenario 2 - Construction material procurement\n"
              << "     Submit, review and approve a material request\n"
              << "  3. Scenario 3 - Generic workflow driver\n"
              << "     Define a new workflow (method / type / structure) and run it\n"
              << "  0. Exit\n"
              << "================================================================\n";
}

bool readLauncherChoice(int& choice)
{
    std::string input;
    if (!UI::readUserInput(input, "Choose an option: ", true, 0, 3))
        return false;
    try
    {
        std::size_t parsed = 0;
        choice = std::stoi(input, &parsed);
        if (parsed != input.size() || choice < 0 || choice > 3)
            throw std::invalid_argument("out of range");
        return true;
    }
    catch (const std::exception&)
    {
        choice = -1;
        std::cout << "Enter a whole number from 0 to 3.\n";
        return true;
    }
}
}

namespace {
// Menu orchestration lives here; scenario modules retain their reusable forms,
// business configuration and document presentation. Only the facade advances work.
void runScenarioMenu(int scenario)
{
    const std::string fixed = scenario == 1 ? "university" : "construction";
    TerminalUI::title(scenario == 1 ? "University Application" : scenario == 2 ? "Construction Procurement" : "Generic Workflow Builder");
    if (scenario != 3)
        std::cout << "\033[36m\nWelcome to TIVIDY — workflows that adapt to the process.\n\n\033[0m";
    while (std::cin) {
        if (scenario == 3) GenericUI::printMenu();
        else FixedUI::printScenarioMenu(fixed);
        std::string input;
        if (!UI::readUserInput(input, scenario == 3 ? "Choose: " : "Choose an action: ", true, scenario == 3 ? 1 : 0, 9)) return;
        try {
            std::istringstream menuInput(input);
            int action = -1;
            char extra = 0;
            if (!(menuInput >> action) || (menuInput >> extra) || action < (scenario == 3 ? 1 : 0) || action > 9) {
                std::cout << "Enter a whole number from " << (scenario == 3 ? 1 : 0) << " to 9.\n";
                continue;
            }
            if ((scenario == 1 && action == 0) ||
                (scenario == 2 && (action == 0 || action == 9)) ||
                (scenario == 3 && (action == 7 || action == 8))) return;
            if (scenario == 3) GenericUI::handleAction(action);
            else FixedUI::handleAction(fixed, action);
            TerminalUI::showOutput("Results / history / documents");
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << "\n";
            if (!std::cin) return;
            TerminalUI::showOutput("Action needs attention");
        }
    }
}
}

int main(int argc, char** argv)
{
    bool interactive = TerminalUI::available();
    const char* plain = std::getenv("TIVIDY_PLAIN");
    if (plain && std::string(plain) == "1") interactive = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--plain") interactive = false;
        else if (argument == "--tui") interactive = true;
        else if (argument == "--help") {
            std::cout << "Usage: tividy [--plain | --tui]\nInteractive terminals use ncurses; redirected input keeps the original CLI.\n";
            return 0;
        } else { std::cerr << "Unknown option: " << argument << "\n"; return 1; }
    }
    if (interactive && !TerminalUI::available()) {
        std::cerr << "The TUI needs an interactive terminal and TERM; use --plain.\n";
        return 1;
    }
    std::unique_ptr<TerminalUI> terminal;
    try { terminal.reset(new TerminalUI(interactive)); }
    catch (const std::exception& error) { std::cerr << error.what() << "\n"; return 1; }
    BudgetLedger balances;
    TividyEngineFacade session;
    UI::engine = &session;
    UI::ledger = &balances;
    try {
        if (!FixedUI::validateProgrammeRules()) throw std::runtime_error("Invalid programme configuration");
        GenericUI::verifyTemplates();
        UniversityWorkflowDirector university;
        ProcurementWorkflowDirector procurement(balances);
        session.registerDefinition("undergraduate-admissions", university.constructWorkflow());
        session.registerDefinition("construction-procurement", procurement.constructWorkflow());
        while (std::cin) {
            printLauncher();
            int choice = -1;
            if (!readLauncherChoice(choice) || choice == 0) break;
            if (choice >= 1 && choice <= 3) runScenarioMenu(choice);
        }
    } catch (const std::exception& error) {
        std::cout << "\nSession ended: " << error.what() << "\n";
    }
    FixedUI::cleanup();
    GenericUI::cleanup();
    std::cout << "Goodbye.\n";
    const bool restoreScreen = TerminalUI::active();
    terminal.reset();
    if (restoreScreen) std::cout << "Goodbye.\n";
    return 0;
}
