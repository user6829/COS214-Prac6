/**
 * @file FixedScenarios.cpp
 * @brief Interactive TUI: South African University Admissions, driven by the
 *        generic TIVIDY workflow engine (COS214 Practical 6).
 *
 * Demonstrates that the SAME generic engine (unmodified) can drive a real
 * South African university admissions scenario across BOTH industries:
 *
 *   Informational industry  ->  Admissions Office   (undergraduate)
 *   Transactional industry  ->  Faculty Board       (construction)
 *
 * South African metrics used:
 *   - APS (Admission Point Score), out of 42 (6 subjects x 7)
 *   - NSC English achievement level, 1 - 7
 *      7 = 80-100%, 6 = 70-79%, 5 = 60-69%, 4 = 50-59% (Bachelor's pass min),
 *      3 = 40-49%, 2 = 30-39%, 1 = 0-29%
 *
 * Routing on APS:
 *   APS >= highApsThreshold  ->  senior-reviewer
 *   APS <  highApsThreshold  ->  admissions-officer
 *
 * Requirement coverage (Section 5):
 *   - Start an instance               -> menu 1
 *   - Inspect available work          -> menu 2
 *   - Take actions                    -> menus 3, 4
 *   - Observe routing & state changes -> observers attached on start
 *   - View history                    -> menu 5
 *   - Use both industries             -> menus 1, 6
 *   - Realistic test data             -> seeded definitions
 *   - Error handling                  -> every engine call wrapped in try/catch
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <sstream>
#include <memory>
#include <vector>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

// Engine
#include "TividyEngineFacade.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkflowContext.h"
#include "WorkflowComponent.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"

// State / routing / approval
#include "WorkItemState.h"
#include "ActiveState.h"
#include "InactiveState.h"
#include "CompletedState.h"
#include "RejectedState.h"
#include "RoutingStrategy.h"
#include "RoleBasedRouting.h"
#include "ValueThresholdRouting.h"
#include "ApprovalHandler.h"
#include "HandlerA.h"
#include "HandlerB.h"

// Observers
#include "EventObserver.h"
#include "NotificationObserver.h"
#include "StateObserver.h"
#include "AuditLoggerObserver.h"

// History
#include "AuditCaretaker.h"
#include "WorkflowMemento.h"

// Visitor
#include "WorkflowVisitor.h"
#include "MetricsReportVisitor.h"

// Documents (both industries)
#include "Document.h"
#include "DocumentFactory.h"
#include "InformationalFactory.h"
#include "TransactionalFactory.h"

// Adapter
#include "ExternalSystemConnector.h"
#include "LegacyiHookAdapter.h"
#include "LegacyiHookAPI.h"
#include "ProcurementRules.h"

namespace FixedScenarios
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

struct ProcurementRequest
{
    std::string instanceId;
    std::string reference;
    ProcurementData data;
    std::string result;
    std::vector<std::string> reasons;
};

class UniversityHistoryObserver : public EventObserver
{
public:
    struct Entry
    {
        std::string time;
        std::string state;
        std::string note;
    };
private:
    std::map<std::string, std::vector<Entry> > entries;
    std::map<std::string, std::string> lastState;

public:
    void update(SubjectWorkFlowInstance &subject) override
    {
        const std::string state = subject.getState();
        if (state != "Inactive" && state != "Active" &&
            state != "DefectResolution" && state != "Rejected" &&
            state != "Completed")
            return;
        if (state.empty() || state == lastState[subject.getInstanceID()])
            return;
        std::time_t now = std::time(nullptr);
        std::tm *local = std::localtime(&now);
        char buffer[16] = {};
        if (local)
            std::strftime(buffer, sizeof(buffer), "%H:%M:%S", local);
        record(subject.getInstanceID(), state, subject.getEventName());
        lastState[subject.getInstanceID()] = state;
    }

    void record(const std::string &instanceId, const std::string &state,
                const std::string &note)
    {
        std::time_t now = std::time(nullptr);
        std::tm *local = std::localtime(&now);
        char buffer[16] = {};
        if (local)
            std::strftime(buffer, sizeof(buffer), "%H:%M:%S", local);
        Entry entry;
        entry.time = buffer;
        entry.state = state;
        entry.note = note;
        entries[instanceId].push_back(entry);
    }

    const std::vector<Entry> *get(const std::string &instanceId) const
    {
        std::map<std::string, std::vector<Entry> >::const_iterator it = entries.find(instanceId);
        return it == entries.end() ? nullptr : &it->second;
    }
};

// =============================================================================
// Globals
// =============================================================================
static TividyEngineFacade gEngine;
static std::vector<EventObserver *> gObservers;
static UniversityHistoryObserver gUniversityHistory;
static std::map<std::string, UniversityApplication> gUniversityApplications;
static std::vector<ProgrammeRule> gProgrammeRules;
static ProcurementRules gProcurementRules;
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
    rule();
    std::cout << BLUE << s << RESET << "\n";
    rule();
}

static std::string ask(const std::string &prompt)
{
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s))
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

static std::string currentDate()
{
    std::time_t now = std::time(nullptr);
    std::tm *local = std::localtime(&now);
    char buffer[32] = {};
    if (local)
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", local);
    return buffer;
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
        std::string s = trim(ask(prompt));
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
        if (value == subject)
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

static std::string askEnglishLevel()
{
    const std::vector<std::string> levels = {
        "Level 1 (0-29%)", "Level 2 (30-39%)", "Level 3 (40-49%)",
        "Level 4 (50-59%)", "Level 5 (60-69%)", "Level 6 (70-79%)",
        "Level 7 (80-100%)"
    };
    std::cout << "Choose your NSC English achievement level:\n";
    for (size_t i = 0; i < levels.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << levels[i] << "\n";
    return std::to_string(askIntRange("Choose: ", 1, 7));
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

// =============================================================================
// Build a South African university workflow definition.
// =============================================================================
static WorkflowDefinition *buildUniversityDefinition(const std::string &industry,
                                                     const std::string &defId,
                                                     double highApsThreshold)
{
    CompositeStage *root = new CompositeStage(defId + "-root",
                                              industry + ": undergraduate admissions");

    // -------------------------------------------------------------------------
    // Stage 1 — Intake
    // -------------------------------------------------------------------------
    CompositeStage *intake = new CompositeStage(defId + "-intake", "Intake");

    LeafWorkItem *submitApp = new LeafWorkItem(defId + "-submit",
                                               "Submit application form");
    LeafWorkItem *payFee = new LeafWorkItem(defId + "-payfee",
                                            "Pay application fee");

    submitApp->setState(new ActiveState());
    payFee->setState(new InactiveState());

    submitApp->setRoutingStrategy(new RoleBasedRouting("applicant"));
    payFee->setRoutingStrategy(new RoleBasedRouting("fee-collector"));

    HandlerA<std::string> *feeReviewer = new HandlerA<std::string>();
    payFee->setApprovalHandler(feeReviewer);

    intake->add(std::shared_ptr<WorkflowComponent>(submitApp));
    intake->add(std::shared_ptr<WorkflowComponent>(payFee));

    // -------------------------------------------------------------------------
    // Stage 2 — Academic Screening (APS + English)
    // -------------------------------------------------------------------------
    CompositeStage *screening = new CompositeStage(defId + "-screening",
                                                   "Academic Screening");

    LeafWorkItem *reviewAps = new LeafWorkItem(defId + "-aps",
                                               "Review APS and matric results");
    reviewAps->setState(new ActiveState());

    reviewAps->setRoutingStrategy(new ValueThresholdRouting(
        "aps", highApsThreshold, "admissions-officer", "senior-reviewer"));

    HandlerA<std::string> *apsReviewer = new HandlerA<std::string>();
    HandlerB<std::string> *apsEscalator = new HandlerB<std::string>();
    apsReviewer->setNext(apsEscalator);
    reviewAps->setApprovalHandler(apsReviewer);

    LeafWorkItem *englishCheck = new LeafWorkItem(defId + "-english",
                                                  "Verify NSC English achievement (level 4 minimum)");
    englishCheck->setState(new InactiveState());
    englishCheck->setRoutingStrategy(new RoleBasedRouting("language-reviewer"));

    screening->add(std::shared_ptr<WorkflowComponent>(reviewAps));
    screening->add(std::shared_ptr<WorkflowComponent>(englishCheck));

    // -------------------------------------------------------------------------
    // Stage 3 — Offer / Decision
    // -------------------------------------------------------------------------
    CompositeStage *offer = new CompositeStage(defId + "-offer",
                                               "Offer Decision");

    LeafWorkItem *issueOffer = new LeafWorkItem(defId + "-issue-offer",
                                                "Issue offer of admission or rejection");
    issueOffer->setState(new InactiveState());
    issueOffer->setRoutingStrategy(new RoleBasedRouting("faculty-board"));

    HandlerA<std::string> *offerReviewer = new HandlerA<std::string>();
    HandlerB<std::string> *offerEscalator = new HandlerB<std::string>();
    offerReviewer->setNext(offerEscalator);
    issueOffer->setApprovalHandler(offerReviewer);

    offer->add(std::shared_ptr<WorkflowComponent>(issueOffer));

    // -------------------------------------------------------------------------
    // Assemble
    // -------------------------------------------------------------------------
    root->add(std::shared_ptr<WorkflowComponent>(intake));
    root->add(std::shared_ptr<WorkflowComponent>(screening));
    root->add(std::shared_ptr<WorkflowComponent>(offer));

    WorkflowDefinition *def = new WorkflowDefinition(defId, industry);
    def->setRootElement(root);
    return def;
}

// =============================================================================
// Construction material procurement workflow (same reusable engine)
// =============================================================================
static WorkflowDefinition *buildProcurementDefinition()
{
    const std::string id = "procurement";
    CompositeStage *root = new CompositeStage("proc-root", "Construction Material Procurement");
    CompositeStage *request = new CompositeStage("proc-request-stage", "Request and Specification");
    CompositeStage *sourcing = new CompositeStage("proc-sourcing-stage", "Supplier and Cost Review");
    CompositeStage *delivery = new CompositeStage("proc-delivery-stage", "Delivery and Inspection");

    LeafWorkItem *submit = new LeafWorkItem("proc-submit", "Submit material request");
    submit->setState(new ActiveState());
    submit->setRoutingStrategy(new RoleBasedRouting("site-manager"));
    LeafWorkItem *spec = new LeafWorkItem("proc-spec", "Verify material specification");
    spec->setState(new InactiveState());
    spec->setRoutingStrategy(new RoleBasedRouting("quantity-surveyor"));
    LeafWorkItem *quote = new LeafWorkItem("proc-quote", "Review supplier quotation");
    quote->setState(new ActiveState());
    quote->setRoutingStrategy(new ValueThresholdRouting("amount", 100000.0, "procurement-officer", "procurement-manager"));
    HandlerA<std::string> *reviewer = new HandlerA<std::string>();
    HandlerB<std::string> *escalation = new HandlerB<std::string>();
    reviewer->setNext(escalation);
    quote->setApprovalHandler(reviewer);
    LeafWorkItem *approve = new LeafWorkItem("proc-approve", "Approve purchase order");
    approve->setState(new InactiveState());
    approve->setRoutingStrategy(new RoleBasedRouting("finance-approver"));
    HandlerA<std::string> *purchaseReviewer = new HandlerA<std::string>();
    approve->setApprovalHandler(purchaseReviewer);
    LeafWorkItem *inspect = new LeafWorkItem("proc-inspect", "Inspect delivered materials");
    inspect->setState(new InactiveState());
    inspect->setRoutingStrategy(new RoleBasedRouting("site-inspector"));
    inspect->setDeferred(true); // happens after delivery, outside this request's approval run

    request->add(std::shared_ptr<WorkflowComponent>(submit));
    request->add(std::shared_ptr<WorkflowComponent>(spec));
    sourcing->add(std::shared_ptr<WorkflowComponent>(quote));
    sourcing->add(std::shared_ptr<WorkflowComponent>(approve));
    delivery->add(std::shared_ptr<WorkflowComponent>(inspect));
    root->add(std::shared_ptr<WorkflowComponent>(request));
    root->add(std::shared_ptr<WorkflowComponent>(sourcing));
    root->add(std::shared_ptr<WorkflowComponent>(delivery));
    WorkflowDefinition *def = new WorkflowDefinition(id, "construction-material-procurement");
    def->setRootElement(root);
    return def;
}

// =============================================================================
// Seed both industries
// =============================================================================
static void seedDefinitions()
{
    if (!validateProgrammeRules())
        throw std::runtime_error("Programme capacity validation failed.");

    gEngine.registerDefinition("undergraduate-admissions",
                               buildUniversityDefinition("informational",
                                                         "undergrad",
                                                         35.0)); // APS >= 35 -> senior-reviewer
    gEngine.registerDefinition("construction-procurement", buildProcurementDefinition());

    // Pass/fail policy is data: it is read from rule files and evaluated by the engine.
    gEngine.loadRules("undergraduate-admissions",
                      WorkflowRule::readFile("rules/undergraduate-admissions.rules"));
    gEngine.loadRules("construction-procurement",
                      WorkflowRule::readFile("rules/construction-procurement.rules"));
}

// =============================================================================
// Walk the definition tree
// =============================================================================
static void walk(WorkflowComponent *node, int depth)
{
    if (!node)
        return;
    std::string indent(depth * 2, ' ');

    CompositeStage *stage = dynamic_cast<CompositeStage *>(node);
    if (stage)
    {
        std::cout << indent << "[Stage] " << stage->getName()
                  << "  (" << stage->getId() << ")\n";
        const std::vector<std::shared_ptr<WorkflowComponent>> &kids =
            stage->getChildren();
        for (size_t i = 0; i < kids.size(); ++i)
            walk(kids[i].get(), depth + 1);
        return;
    }

    LeafWorkItem *leaf = dynamic_cast<LeafWorkItem *>(node);
    if (leaf)
    {
        std::cout << indent << "[Work item] " << leaf->getName()
                  << "  (" << leaf->getId() << ")\n";

        WorkItemState *st = leaf->getCurrentState();
        std::cout << indent << "    state:   "
                  << (st ? st->getStateName() : std::string("none")) << "\n";

        RoutingStrategy *rs = leaf->getRoutingStrategy();
        if (rs)
        {
            try
            {
                std::cout << indent << "    routing rule: "
                          << rs->describe() << "\n";
            }
            catch (const std::exception &e)
            {
                std::cout << indent << "    routing note: " << e.what() << "\n";
            }
        }
    }
}

// =============================================================================
// Print step ids for a chosen workflow
// =============================================================================
static void printStepIds(const std::string &prefix)
{
    if (prefix == "proc") {
        std::cout << "  proc-submit    Submit material request\n"
                  << "  proc-spec      Verify material specification\n"
                  << "  proc-quote     Review supplier quotation\n"
                  << "  proc-approve   Approve purchase order\n"
                  << "  proc-inspect   Inspect delivered materials\n";
        return;
    }
    std::cout << "  " << prefix << "-submit      Submit application form\n"
              << "  " << prefix << "-payfee      Pay application fee\n"
              << "  " << prefix << "-aps         Review APS and matric results\n"
              << "  " << prefix << "-english     Verify NSC English level\n"
              << "  " << prefix << "-issue-offer Issue offer / rejection\n";
}

static void setUniversityState(WorkflowInstance *instance,
                                   const std::string &state,
                                   const std::string &event)
    {
        WorkflowContext *context = instance->getContext();
        context->setData("State", state);
        context->setData("Event", event);
        instance->setEvent();
        instance->setState();
    }

static void printStepRecord(int number, const StepRunRecord &step)
{
    std::cout << "\n  3." << number << " " << step.stageName << "\n"
              << "    Work item: " << step.stepName << " (" << step.stepId << ")\n";
    if (step.outcome == "DEFERRED")
    {
        std::cout << "    State: Inactive - awaiting a later event (not part of this approval run)\n";
        return;
    }
    if (step.outcome == "BLOCKED")
    {
        std::cout << "    Not run: " << step.note << "\n";
    }
    else
    {
        std::string action = "Executed";
        if (step.outcome == "APPROVED") action = "Executed and approved";
        else if (step.outcome == "REJECTED") action = "Executed and rejected";
        std::cout << "    Routing: " << step.routing << "\n"
                  << "    Routed role: " << step.assignee << "\n"
                  << "    Action: " << action << " by " << step.assignee << "\n"
                  << "    State transition: " << step.stateBefore << " -> " << step.stateAfter << "\n";
    }
    for (size_t i = 0; i < step.rules.size(); ++i)
    {
        const RuleResult &rule = step.rules[i];
        std::cout << "    Check: " << rule.name << " "
                  << (rule.passed ? GREEN : RED) << (rule.passed ? "[PASS]" : "[FAIL]") << RESET << "\n"
                  << "      " << rule.compared << "\n";
        if (!rule.passed)
            std::cout << "      " << rule.reason << "\n";
    }
}

static void printRunSummary(const WorkflowRunReport &report)
{
    size_t total = 0, failed = report.failedRules.size();
    for (size_t i = 0; i < report.steps.size(); ++i)
        total += report.steps[i].rules.size();
    std::cout << "------------------------------------------------------------\n"
              << " Summary: " << (total - failed) << " of " << total << " checks passed.\n";
}

static std::vector<std::string> failureReasons(const WorkflowRunReport &report)
{
    std::vector<std::string> reasons;
    for (size_t i = 0; i < report.failedRules.size(); ++i)
        reasons.push_back(report.failedRules[i].reason);
    return reasons;
}

    class UniversityTextDocument : public Document
    {
    private:
        std::string text;

    public:
        explicit UniversityTextDocument(const std::string &content) : text(content) {}
        void print() override
        {
            std::cout << text << "\n";
        }
        const std::string &getText() const { return text; }
    };

    static UniversityTextDocument *buildUniversityDocument(
        const UniversityApplication &application,
        const std::string &kind)
    {
        const bool contract = kind == "contract";
        DocumentFactory *factory = contract
            ? static_cast<DocumentFactory *>(new TransactionalFactory())
            : static_cast<DocumentFactory *>(new InformationalFactory());
        Document *templateDocument = factory->createInstance(contract ? "contract" : "guideline");
        delete templateDocument;
        delete factory;

        std::time_t now = std::time(nullptr);
        std::tm *local = std::localtime(&now);
        char date[32] = {};
        if (local)
            std::strftime(date, sizeof(date), "%Y-%m-%d", local);

        std::ostringstream content;
        content << "========================================\n"
                << "TIVIDY UNIVERSITY " << (contract ? "CONTRACT" : "LETTER") << "\n"
                << "========================================\n"
                << "Date: " << date << "\n"
                << "Applicant: " << application.firstName << " " << application.surname << "\n"
                << "Student number: " << application.studentNumber << "\n"
                << "Instance ID: " << application.instanceId << "\n"
                << "Attempt: " << application.attempt << "\n"
                << "Programme: " << application.programme << "\n"
                << "Result: " << application.result << "\n";
        if (contract)
        {
            content << "Parties: TIVIDY University and the applicant\n"
                    << "Terms: fees, academic requirements and student conduct apply.\n"
                    << "Signature (University): ____________________\n"
                    << "Signature (Applicant):  ____________________\n";
        }
        else if (application.result == "ACCEPTED")
        {
            content << "Next steps: accept the offer, pay the applicable fees and register.\n";
        }
        else
        {
            content << "Reasons for rejection:\n";
            for (size_t i = 0; i < application.reasons.size(); ++i)
                content << "- " << application.reasons[i] << "\n";
        }
        content << "========================================\n";
        return new UniversityTextDocument(content.str());
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
                                       UniversityTextDocument *document,
                                       const std::string &suffix)
    {
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
        output << document->getText();
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

        ProgrammeRule *programme = findProgrammeRule(application.programme);
        if (!programme)
            throw std::runtime_error("No programme data configured for " + application.programme);
        context->setData("minAps", std::to_string(programme->minimumAps));
        context->setData("requiredSubject", programme->requiredSubject);
        context->setData("seatsTaken", std::to_string(programme->seatsTaken));
        context->setData("seatThreshold", std::to_string(programme->threshold));
        context->setData("capacity", std::to_string(programme->capacity));
    }

    static bool processUniversityApplication(UniversityApplication &application,
                                             WorkflowInstance *instance,
                                             const std::string &resubmissionNote)
    {
        application.reasons.clear();
        setUniversityApplicationContext(instance, application);
        std::cout << "\nStep 3 of 5 - Processing application attempt "
                  << application.attempt << "\n"
                  << "The workflow engine is checking each stage and requirement.\n"
                  << "------------------------------------------------------------\n";
        if (!resubmissionNote.empty())
            std::cout << resubmissionNote << "\n";

        setUniversityState(instance, "Active", "Processing started (attempt " +
                           std::to_string(application.attempt) + ")");
        gUniversityHistory.record(instance->getInstanceID(), "Active",
                                  "Processing started (attempt " +
                                  std::to_string(application.attempt) + ")");

        // The engine routes every work item, evaluates the data-driven rules and decides
        // approve/reject. The driver only displays what the engine reports.
        int shown = 0;
        const std::string attemptText = std::to_string(application.attempt);
        const std::string instanceId = instance->getInstanceID();
        const WorkflowRunReport report = gEngine.runWorkflow(instanceId,
            [&](const StepRunRecord &step)
            {
                printStepRecord(++shown, step);
                if (step.outcome != "BLOCKED" && step.outcome != "DEFERRED")
                    gUniversityHistory.record(instanceId, step.stateAfter,
                                              "Processed " + step.stepId + " (attempt " + attemptText + ")");
            });
        printRunSummary(report);

        const bool accepted = report.approved;
        application.reasons = failureReasons(report);
        WorkflowContext *context = instance->getContext();
        if (!accepted)
        {
            setUniversityState(instance, "DefectResolution", "Admission checks failed (attempt " +
                               attemptText + ")");
            setUniversityState(instance, "Rejected", "Rejected - checks failed (attempt " +
                               attemptText + ")");
            application.result = "REJECTED";
            context->setData("Result", application.result);
            context->setData("rejectionReasons", joinSubjects(application.reasons));
        }
        else
        {
            ProgrammeRule *rule = findProgrammeRule(application.programme);
            if (rule)
                ++rule->seatsTaken; // seat is committed only after the engine approved the application
            application.result = "ACCEPTED";
            context->setData("Result", application.result);
            setUniversityState(instance, "Completed", "Accepted - all checks passed (attempt " +
                               attemptText + ")");
        }
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
        WorkflowInstance *instance = gEngine.getInstance(application.instanceId);
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
            gObservers.push_back(new NotificationObserver());
            gObservers.push_back(new StateObserver());
            gObservers.push_back(new AuditLoggerObserver());
        }
        for (size_t i = 0; i < gObservers.size(); ++i)
            instance->attach(gObservers[i]);
        instance->attach(&gUniversityHistory);
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
        WorkflowInstance *instance = gEngine.startWorkflow("undergraduate-admissions", initial);
        application.instanceId = instance->getInstanceID();
        attachUniversityObservers(instance);
        setUniversityState(instance, "Inactive", "Application received (attempt 1)");
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

        WorkflowInstance *instance = gEngine.getInstance(original.instanceId);
        if (!instance)
        {
            std::cout << RED << "The workflow instance could not be found.\n" << RESET;
            return;
        }
        ProgrammeRule *oldRule = findProgrammeRule(original.programme);
        if (original.result == "ACCEPTED" && oldRule && oldRule->seatsTaken > 0)
            --oldRule->seatsTaken;

        gEngine.rollbackInstance(original.instanceId, 0);
        proposed.attempt = original.attempt + 1;
        attachUniversityObservers(instance);
        setUniversityState(instance, "Inactive",
                           "Application edited and resubmitted (attempt " +
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
        {
            std::ostringstream amount;
            amount << std::fixed << std::setprecision(2) << request.data.amount;
            context->setData("amount", amount.str());
        }
        // Lookup values (authority limit, approved suppliers, remaining budget) for the engine's rules.
        const std::map<std::string, std::string> reference = gProcurementRules.referenceData(request.data);
        for (std::map<std::string, std::string>::const_iterator it = reference.begin();
             it != reference.end(); ++it)
            context->setData(it->first, it->second);
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
        gEngine.exportMetrics(&visitor);
        std::cout << "Session workflow summary (" << result << "): "
                  << visitor.getMetrics() << "\n";
    }

    class ProcurementTextDocument : public Document
    {
    private:
        std::string text;

    public:
        explicit ProcurementTextDocument(const std::string &content) : text(content) {}
        void print() override { std::cout << text << "\n"; }
        const std::string &getText() const { return text; }
    };

    static ProcurementTextDocument *buildProcurementDocument(
        const ProcurementRequest &request)
    {
        InformationalFactory factory;
        Document *templateDocument = factory.createInstance("guideline");
        delete templateDocument;

        std::ostringstream content;
        content << "========================================\n"
                << "TIVIDY PROCUREMENT "
                << (request.result == "APPROVED" ? "APPROVAL LETTER" : "REJECTION LETTER")
                << "\n========================================\n"
                << "Date: " << currentDate() << "\n"
                << "Requester: " << request.data.firstName << " "
                << request.data.surname << "\n"
                << "Role: " << request.data.role << "\n"
                << "Request reference: " << request.reference << "\n"
                << "Instance ID: " << request.instanceId << "\n"
                << "Project: " << request.data.project << "\n"
                << "Material: " << request.data.material << "\n"
                << "Quantity: " << request.data.quantity << "\n"
                << "Supplier: " << request.data.supplier << "\n"
                << "Estimated cost: R" << std::fixed << std::setprecision(2)
                << request.data.amount << "\n"
                << "Result: " << request.result << "\n";
        if (request.result == "APPROVED")
        {
            content << "Purchase-order reference: PO-" << request.reference.substr(4) << "\n"
                    << "Next steps: arrange delivery and complete the inspection.\n";
        }
        else
        {
            content << "Reasons for rejection:\n";
            for (size_t i = 0; i < request.reasons.size(); ++i)
                content << "- " << request.reasons[i] << "\n";
        }
        content << "========================================\n";
        return new ProcurementTextDocument(content.str());
    }

    static void saveProcurementDocument(const ProcurementRequest &request,
                                        const ProcurementTextDocument &document)
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
        output << document.getText();
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
        if (request->result.empty())
        {
            std::cout << RED << "Documents are available only after the request is finished.\n"
                      << RESET;
            return;
        }
        const std::string prefix = "documents/" + request->reference + "_";
        std::remove((prefix + "approval_letter.txt").c_str());
        std::remove((prefix + "rejection_letter.txt").c_str());
        ProcurementTextDocument *document = buildProcurementDocument(*request);
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
        WorkflowInstance *instance = gEngine.getInstance(request->instanceId);
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
        const std::vector<UniversityHistoryObserver::Entry> *history =
            gUniversityHistory.get(request->instanceId);
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
        std::cout << "------------------------------------------------------------\n";
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
        WorkflowInstance *instance = gEngine.startWorkflow("construction-procurement", initial);
        request.instanceId = instance->getInstanceID();
        attachUniversityObservers(instance);
        setUniversityState(instance, "Inactive", "Request received");
        gProcurementRequests[request.instanceId] = request;
        std::cout << "Generated request reference: " << request.reference << "\n"
                  << "Workflow instance ID: " << request.instanceId << "\n"
                  << "Your request has been registered.\n";

        std::cout << "\nStep 2 of 5 - Requester information\n";
        request.data.firstName = askRequired("First name: ");
        request.data.surname = askRequired("Surname: ");
        request.data.role = askChoice("Requester role:",
                                      {"Site manager", "Project manager",
                                       "Quantity surveyor"});
        request.data.project = askRequired("Construction project name: ");
        request.data.material = askChoice("Material required:",
                                          {"Concrete", "Cement", "Steel",
                                           "Bricks", "Aggregates", "Other"});
        if (request.data.material == "Other")
            request.data.material = askRequired("Specify the material: ");
        request.data.quantity = askQuantity();
        request.data.supplier = askChoice("Preferred supplier:",
                                          {"BuildPro Supplies", "SA Materials",
                                           "Civils Direct", "Other"});
        if (request.data.supplier == "Other")
            request.data.supplier = askRequired("Specify the supplier: ");
        request.data.amount = 0.0;
        request.data.amount = std::stod(askPositiveAmount("Estimated cost in ZAR (e.g. 125000): "));
        request.data.deliveryDate = askDate();
        request.data.notes = ask("Purpose/notes (optional): ");
        setProcurementContext(instance, request);
        gProcurementRequests[request.instanceId] = request;

        std::cout << "\nStep 3 of 5 - Processing request\n"
                  << "The workflow engine is processing the request in order.\n"
                  << "------------------------------------------------------------\n";
        setUniversityState(instance, "Active", "Processing started");
        int shown = 0;
        const WorkflowRunReport report = gEngine.runWorkflow(request.instanceId,
            [&](const StepRunRecord &step) { printStepRecord(++shown, step); });
        printRunSummary(report);
        request.reasons = failureReasons(report);
        const bool approved = report.approved;
        request.result = approved ? "APPROVED" : "REJECTED";
        if (approved)
        {
            gProcurementRules.commit(request.data); // budget is committed only after the engine approved
            setUniversityState(instance, "Completed", "Request approved");
        }
        else
        {
            setUniversityState(instance, "DefectResolution", "Procurement checks failed");
            setUniversityState(instance, "Rejected", "Request rejected");
        }
        gProcurementRequests[request.instanceId] = request;

        std::cout << "\nStep 4 of 5 - Request result\n"
                  << (approved ? GREEN : RED)
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
        if (!approved)
            for (size_t i = 0; i < request.reasons.size(); ++i)
                std::cout << "Reason: " << request.reasons[i] << "\n";
        std::cout << "========================================\n" << RESET;
        std::cout << "\nStep 5 of 5 - Request complete\n"
                  << "Your request is complete. Thank you.\n";
        sendAdapterAndVisitorSummary(request.reference, instance, request.result);
    }


// =============================================================================
// Menu 1 — Start a new application
// =============================================================================
static void menuStart(const std::string &scenario)
{
    heading("Start a new application");

    std::string defId;
    std::string prefix;
    if (scenario == "university") {
        defId = "undergraduate-admissions";
        prefix = "undergrad";
    } else {
        defId = "construction-procurement";
        prefix = "proc";
    }


    std::map<std::string, std::string> data;
    if (prefix == "proc") {
        data["requester"] = askRequired("Requester name: ");
        data["requesterRole"] = askChoice("Requester role:",
                                           {"Site manager", "Project manager",
                                            "Quantity surveyor"});
        data["projectName"] = askRequired("Construction project name: ");
        data["material"] = askChoice("Material required:",
                                      {"Concrete", "Cement", "Steel",
                                       "Bricks", "Aggregates", "Other"});
        if (data["material"] == "Other")
            data["material"] = askRequired("Specify the material: ");
        data["quantity"] = askQuantity();
        data["supplier"] = askChoice("Preferred supplier:",
                                      {"BuildPro Supplies", "SA Materials",
                                       "Civils Direct", "Other"});
        if (data["supplier"] == "Other")
            data["supplier"] = askRequired("Specify the supplier: ");
        data["amount"] = askPositiveAmount("Estimated cost in ZAR (e.g. 125000): ");
        data["deliveryDate"] = askDate();
        data["State"] = "Request submitted";
        data["Event"] = "Material request submitted";
    } else {
        data["studentName"] = askRequired("Student full name: ");
        data["email"] = askEmail();
        data["nationality"] = askNationality();
        data["programme"] = askUniversityProgramme();
        int aps = askIntRange("APS (out of 42, e.g. 38): ", 0, 42);
        data["aps"] = std::to_string(aps);
        data["englishLevel"] = askEnglishLevel();
        int year = askIntRange("Matric year (e.g. 2024): ", 1900, currentYear());
        data["matricYear"] = std::to_string(year);
        data["applicationFeePaid"] = "false";
        data["State"] = "Submitted";
        data["Event"] = "Application submitted";
    }
    data["ID"] = "";
    data["definitionId"] = defId;

    try
    {
        // The student number is system-generated, never entered by the applicant.
        // It is assigned only after all application fields have passed validation.
        if (prefix != "proc")
            data["studentNumber"] = generateStudentNumber();

        WorkflowInstance *inst = gEngine.startWorkflow(defId, data);

        if (gObservers.empty())
        {
            gObservers.push_back(new NotificationObserver());
            gObservers.push_back(new StateObserver());
            gObservers.push_back(new AuditLoggerObserver());
        }
        for (size_t i = 0; i < gObservers.size(); ++i)
            inst->attach(gObservers[i]);

        std::cout << "\n>> Application registered successfully.\n";
        if (prefix != "proc")
            std::cout << GREEN << "   Generated student number: "
                      << data["studentNumber"] << RESET << "\n"
                      << "   Keep this number for future reference.\n";
        std::cout << "   Workflow instance ID: " << inst->getInstanceID() << "\n"
                  << "   Keep the instance ID for workflow actions and history.\n"
                  << "   The following step ids belong to this workflow:\n";
        printStepIds(prefix);
        if (prefix != "proc")
            std::cout << GREEN << "   Confirmation email sent to " << data["email"]
                      << ".\n" << RESET
                      << "   Your application has been received. Keep your student number "
                      << "and instance ID for future updates.\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
}

// =============================================================================
// Menu 2 — Inspect an application
// =============================================================================
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
    WorkflowInstance *instance = gEngine.getInstance(application->instanceId);
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
    const std::vector<UniversityHistoryObserver::Entry> *history =
        gUniversityHistory.get(application->instanceId);
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
    std::cout << "--------------------------------------------------------\n";
}

static void menuInspect()
{
    heading("Inspect an application");
    std::string iid = askRequired("Instance id (e.g. INST-1): ");

    try
    {
        WorkflowInstance *inst = gEngine.getInstance(iid);
        if (!inst)
        {
            std::cout << "No such application.\n";
            return;
        }

        WorkflowContext *ctx = inst->getContext();
        std::cout << "Instance: " << inst->getInstanceID() << "\n"
                  << "  state:  " << inst->getState() << "\n"
                  << "  event:  " << inst->getEventName() << "\n";

        if (ctx)
        {
            const std::map<std::string, std::string> &all = ctx->getAll();
            for (std::map<std::string, std::string>::const_iterator it = all.begin();
                 it != all.end(); ++it)
            {
                if (it->first == "aps")
                    std::cout << "  " << std::setw(18) << "APS" << " = "
                              << it->second << " / 42\n";
                else if (it->first == "englishLevel")
                    std::cout << "  " << std::setw(18) << "English (NSC)" << " = "
                              << it->second << " / 7\n";
                else
                    std::cout << "  " << std::setw(18) << it->first
                              << " = " << it->second << "\n";
            }
        }

        std::string defId;
        try {
            defId = ctx->getData("definitionId");
        } catch (const std::out_of_range&) {
            std::cout << RED << "This instance has no workflow definition recorded.\n" << RESET;
            return;
        }

        WorkflowDefinition *def = gEngine.getDefinition(defId);
        if (def)
        {
            rule();
            walk(def->getRootElement(), 0);
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
}

// =============================================================================
// Menu 3 — Execute a step
// =============================================================================
static void menuExecute(const std::string &scenario)
{
    heading(scenario == "university"
                ? "Continue a university application"
                : "Continue a construction request");
    std::string iid = askRequired("Instance id: ");

    std::cout << "\nAvailable steps:\n"
              << "Choose the step id shown for your workflow:\n";
    if (scenario == "university") {
        std::cout << "  --- University ---\n";
        printStepIds("undergrad");
    } else {
        std::cout << "  --- Construction procurement ---\n";
        printStepIds("proc");
    }

    std::string sid = askRequired("Step id: ");
    try
    {
        gEngine.executeStep(iid, sid);
        std::cout << "Executed step: " << sid << "\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
}

// =============================================================================
// Menu 4 — Record a decision
// =============================================================================
static void menuDecision(const std::string &scenario)
{
    heading(scenario == "university"
                ? "Record a university decision"
                : "Approve or reject a procurement step");
    std::string iid = askRequired("Instance id: ");
    std::string sid = askRequired(scenario == "university"
                                      ? "Step id awaiting an admission decision: "
                                      : "Step id awaiting procurement approval: ");
    std::string actorRole = askRequired("Role recording the decision (must match the routed assignee): ");
    std::string dec;
    while (true) {
        dec = askRequired("Decision (APPROVE / REJECT / WAITLIST): ");
        for (std::string::size_type i=0; i<dec.size(); ++i) dec[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(dec[i])));
        if (dec == "APPROVE" || dec == "REJECT" || dec == "WAITLIST") break;
        std::cout << RED << "Choose APPROVE, REJECT, or WAITLIST.\n" << RESET;
    }

    try
    {
        gEngine.processApproval(iid, sid, dec, actorRole);
        std::cout << "Recorded decision: " << dec << "\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
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
    if (application->result != "ACCEPTED" && application->result != "REJECTED")
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
        UniversityTextDocument *document = buildUniversityDocument(
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
#if 0
static void printMenu()
{
    rule();
    std::cout << BLUE
              << "  _______ _____ _    _ _____ _____  __\n"
              << " |__   __|_   _| |  | |_   _|  __ \\\n"
              << "    | |    | | | |  | | | | | |  | |\n"
              << "    | |    | | | |__| | | | | |__| |\n"
              << "    |_|   _| |_ \\____/ _| |_|_____/\n"
              << "        REUSABLE WORKFLOW ENGINE\n" << RESET;
    std::cout << YELLOW << "  University admissions + construction procurement\n" << DIM << "  One engine. Different processes.\n" << RESET;
    rule();
}
#endif

static void printScenarioMenu(const std::string &scenario)
{
    rule();
    if (scenario == "university") {
    std::cout << BLUE << "  UNIVERSITY APPLICATIONS\n" << RESET
              << "  1. Start a university application\n"
              << "  2. View application status\n"
              << "  3. Edit and resubmit an application\n"
              << "  4. View application history\n"
              << "  5. Generate university documents\n"
              << "  6. Choose a different workflow scenario\n"
              << "  7. Exit\n";
    } else {
    std::cout << BLUE << "  CONSTRUCTION MATERIAL PROCUREMENT\n" << RESET
              << "  1. Start a material request\n"
              << "  2. View request status\n"
              << "  3. View request history\n"
              << "  4. Generate procurement documents\n";
    }
    if (scenario != "university")
        std::cout << "  9. Choose a different workflow scenario\n"
                  << "  0. Exit\n";
    rule();
}

static bool runScenario(const std::string &scenario)
{
    while (true) {
    printScenarioMenu(scenario);
    int choice = askIntRange("Choose an action: ", 0, 9);
    if (choice == 0)
        return false;
    if ((scenario == "university" && choice == 6) ||
        (scenario != "university" && choice == 9))
        return true;
    if (scenario == "university" && choice == 7)
        return false;

    switch (choice) {
    case 1:
        if (scenario == "university")
            menuStartUniversity();
        else if (scenario == "construction")
            menuStartProcurement();
        else
            menuStart(scenario);
        break;
    case 2:
        if (scenario == "university")
            menuInspectUniversity();
        else if (scenario == "construction")
            menuProcurementStatus();
        else
            menuInspect();
        break;
    case 3:
        if (scenario == "university")
            {
                std::string key = askUniversityInstanceId();
                if (!key.empty())
                    menuEditUniversity(key);
            }
            else if (scenario == "construction")
                menuProcurementHistory();
            else
                menuExecute(scenario);
            break;
        case 4:
            if (scenario == "university")
                menuUniversityHistory();
            else if (scenario == "construction")
                menuProcurementDocuments();
            else
                menuDecision(scenario);
            break;
        case 5:
            if (scenario == "university")
                menuDocuments();
            else
                std::cout << RED << "Choose one of the listed actions.\n" << RESET;
            break;
        case 6:
            if (scenario == "university")
                return true;
            std::cout << RED << "Choose one of the listed actions.\n" << RESET;
            break;
    case 7:
        if (scenario == "university")
            return false;
        std::cout << RED << "Choose one of the listed actions.\n" << RESET;
        break;
    case 8:
        std::cout << RED << "Choose one of the listed actions.\n" << RESET;
        break;
    default:
        std::cout << RED << "Choose one of the listed actions.\n" << RESET;
        break;
    }
    }
    return false;
}

// =============================================================================
static void runFixedScenario(const std::string &scenario)
{
    std::cout << CYAN << "\nWelcome to TIVIDY — workflows that adapt to the process.\n\n" << RESET;
    static bool seeded = false;
    if (!seeded)
    {
        seedDefinitions();
        seeded = true;
    }

    try {
        runScenario(scenario);
    } catch (const std::exception &e) {
        std::cout << RED << "\nSession ended: " << e.what() << "\n" << RESET;
    }
}

void runUniversityScenario()
{
    runFixedScenario("university");
}

void runConstructionScenario()
{
    runFixedScenario("construction");
}

void cleanup()
{
    for (size_t i = 0; i < gObservers.size(); ++i)
        delete gObservers[i];
    gObservers.clear();
}

} // namespace FixedScenarios