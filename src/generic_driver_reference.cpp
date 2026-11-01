/**
 * @file main.cpp
 * @brief Interactive TIVIDY driver: choose EXECUTION MODE (manual/automated),
 *        WORKFLOW TYPE (case/project/approval/document-management) and
 *        WORKFLOW STRUCTURE (rule-driven/state-machine/parallel/sequential),
 *        then run a scenario-specific instantiation of the generic engine.
 *
 * The engine itself is untouched. All scenario variation happens here.
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <sstream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <functional>
#include <cctype>
#include <algorithm>
#include <cstdlib>

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

// History / visitor
#include "AuditCaretaker.h"
#include "WorkflowMemento.h"
#include "WorkflowVisitor.h"
#include "MetricsReportVisitor.h"

// Documents
#include "Document.h"
#include "DocumentFactory.h"
#include "InformationalFactory.h"
#include "TransactionalFactory.h"

// Adapter
#include "ExternalSystemConnector.h"
#include "LegacyiHookAdapter.h"
#include "LegacyiHookAPI.h"

// =============================================================================
// Globals
// =============================================================================
static TividyEngineFacade gEngine;
static std::vector<EventObserver *> gObservers;

// =============================================================================
// Enums chosen by the user
// =============================================================================
enum class ExecMode { Manual, Automated };

enum class WorkflowType {
    Case,               // investigative / services
    Project,            // deliverables
    Approval,           // sign-off
    DocumentManagement  // publishing / lifecycle
};

enum class WorkflowStructure {
    Sequential,   // flat list of stages in order
    Parallel,     // two sibling stages meant to run concurrently
    RuleDriven,   // a gate stage routes based on data (value threshold)
    StateMachine  // chain of states: start -> in-progress -> end
};

// =============================================================================
// I/O helpers
// =============================================================================
static void rule() { std::cout << "------------------------------------------------------------\n"; }
static void heading(const std::string &s) { rule(); std::cout << s << "\n"; rule(); }

static std::string ask(const std::string &prompt)
{
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    if (!s.empty() && s[s.size() - 1] == '\r') s.erase(s.size() - 1);
    return s;
}

static std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool isNumericKey(const std::string &k)
{
    std::string lower;
    for (char c : k) lower.push_back((char)std::tolower((unsigned char)c));
    static const char* numericHints[] = {
        "amount","budget","credits","pages","score","severity",
        "value","price","cost","total","fee","threshold","age",
        "quantity","qty","count","size","duration","weight"
    };
    for (const char* h : numericHints)
        if (lower.find(h) != std::string::npos) return true;
    if (lower.size() >= 5 &&
        lower.compare(lower.size()-5, 5, "score") == 0) return true;
    return false;
}

static std::string askRequired(const std::string &prompt, size_t maxLen = 200)
{
    for (;;)
    {
        std::string s = trim(ask(prompt));
        if (s.empty()) { std::cout << "  ! This field is required.\n"; continue; }
        if (s.size() > maxLen) {
            std::cout << "  ! Too long (max " << maxLen << " chars).\n"; continue;
        }
        return s;
    }
}

static std::string askName(const std::string &prompt)
{
    for (;;)
    {
        std::string s = askRequired(prompt, 80);
        bool hasAlpha = false;
        bool ok = true;
        for (unsigned char c : s) {
            if (std::isalpha(c)) hasAlpha = true;
            if (!(std::isalnum(c) || std::isspace(c) || c=='-' || c=='_' ||
                  c=='\'' || c=='.' || c=='&' || c=='/')) {
                ok = false; break;
            }
        }
        if (!hasAlpha) { std::cout << "  ! Must contain at least one letter.\n"; continue; }
        if (!ok)       { std::cout << "  ! Invalid characters.\n"; continue; }
        return s;
    }
}

static double askPositiveDouble(const std::string &prompt, double maxVal = 1e12)
{
    for (;;)
    {
        std::string s = trim(ask(prompt));
        if (s.empty()) { std::cout << "  ! Value required.\n"; continue; }

        int dots = 0, signs = 0;
        bool ok = true;
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '.') { ++dots; if (dots > 1) { ok=false; break; } }
            else if (c == '-' || c == '+') { ++signs; if (i != 0 || signs > 1) { ok=false; break; } }
            else if (!std::isdigit((unsigned char)c)) { ok = false; break; }
        }
        if (!ok || dots > 1) { std::cout << "  ! Not a valid number.\n"; continue; }

        double v = 0.0;
        try { v = std::stod(s); } catch (...) { std::cout << "  ! Out of range.\n"; continue; }

        if (v <= 0)      { std::cout << "  ! Must be greater than zero.\n"; continue; }
        if (v > maxVal)  { std::cout << "  ! Too large (max " << maxVal << ").\n"; continue; }
        return v;
    }
}

static double askDoubleInRange(const std::string &prompt, double lo, double hi)
{
    for (;;)
    {
        double v = askPositiveDouble(prompt, hi * 2.0);
        if (v < lo || v > hi) {
            std::cout << "  ! Must be between " << lo << " and " << hi << ".\n";
            continue;
        }
        return v;
    }
}

static std::string askEnum(const std::string &prompt,
                           const std::vector<std::string> &allowed,
                           const std::string &defaultValue = "")
{
    for (;;)
    {
        std::string s = trim(ask(prompt));
        if (s.empty() && !defaultValue.empty()) return defaultValue;
        std::string up;
        for (char c : s) up.push_back((char)std::toupper((unsigned char)c));
        for (const auto &a : allowed) if (up == a) return a;
        std::cout << "  ! Choose one of:";
        for (const auto &a : allowed) std::cout << " " << a;
        std::cout << "\n";
    }
}

static std::map<std::string, std::string> validateDataValues(
    const std::map<std::string, std::string> &raw,
    const std::vector<std::string> &keys)
{
    std::map<std::string, std::string> out = raw;

    for (const auto &k : keys)
    {
        std::string v = out.count(k) ? trim(out[k]) : "";
        if (v.empty())
            throw std::runtime_error("missing required field: " + k);

        if (isNumericKey(k))
        {
            char *end = nullptr;
            double d = std::strtod(v.c_str(), &end);
            if (end == v.c_str() || (end && *end != '\0'))
                throw std::runtime_error("field '" + k + "' must be numeric (got \"" + v + "\")");
            if (d <= 0.0)
                throw std::runtime_error("field '" + k + "' must be > 0 (got " + v + ")");
            if (d > 1e12)
                throw std::runtime_error("field '" + k + "' is implausibly large");
        }
        else if (k == "creditScore")
        {
            int s = std::atoi(v.c_str());
            if (s < 300 || s > 850)
                throw std::runtime_error("creditScore must be 300..850 (got " + v + ")");
        }
        else if (k == "severity")
        {
            int s = std::atoi(v.c_str());
            if (s < 1 || s > 10)
                throw std::runtime_error("severity must be 1..10 (got " + v + ")");
        }
        else if (v.size() > 200)
        {
            throw std::runtime_error("field '" + k + "' is too long (max 200)");
        }
    }
    return out;
}


// availibles instance ID's
static void listInstances()
{
    heading("Active Workflow Instances");
    
    AuditCaretaker* care = gEngine.getAuditCaretaker();
    int n = care->size();
    std::map<std::string, std::string> uniqueInstances; 

    for (int i = 0; i < n; ++i)
    {
        WorkflowMemento* m = care->getSnapshot(i);
        if (!m) continue;

        std::map<std::string, std::string>* state = m->getState();
        if (state)
        {
            std::string id = "";
            // Check for context "ID" first, fallback to memento "instanceID"
            if (state->find("ID") != state->end()) id = (*state)["ID"];
            else if (state->find("instanceID") != state->end()) id = (*state)["instanceID"];

            if (id.empty()) continue;

            // Store the instance (map naturally overwrites older snapshots with newer ones)
            uniqueInstances[id] = state->count("scenario") ? (*state)["scenario"] : "Workflow Instance";
        }
    }

    if (uniqueInstances.empty())
    {
        std::cout << "  No active instances found. Start one from the main menu!\n";
        return;
    }

    std::cout << "  ID        | Scenario\n";
    rule();
    for (std::map<std::string, std::string>::iterator it = uniqueInstances.begin(); 
         it != uniqueInstances.end(); ++it)
    {
        std::cout << "  " << std::setw(9) << std::left << it->first 
                  << " | " << it->second << "\n";
    }
}

static void viewHistory()
{
    heading("View Workflow History");
    std::string iid = trim(ask("Enter instance id (e.g., INST-1): "));
    if (iid.empty()) { std::cout << "  No id entered.\n"; return; }
    for (unsigned char c : iid) {
        if (!(std::isalnum(c) || c == '-' || c == '_')) {
            std::cout << "  ! Invalid id characters.\n";
            return;
        }
    }

    AuditCaretaker* care = gEngine.getAuditCaretaker();
    int n = care->size();
    int found = 0;

    std::cout << "\nSnapshots for " << iid << ":\n";
    for (int i = 0; i < n; ++i)
    {
        WorkflowMemento* m = care->getSnapshot(i);
        if (!m) continue;

        std::map<std::string, std::string>* state = m->getState();
        if (state)
        {
            std::string id = "";
            if (state->find("ID") != state->end()) id = (*state)["ID"];
            else if (state->find("instanceID") != state->end()) id = (*state)["instanceID"];

            if (id == iid)
            {
                std::cout << "  [" << i << "] Timestamp: " << m->getTimestamp() << "\n";
                for (std::map<std::string, std::string>::iterator it = state->begin(); it != state->end(); ++it)
                {
                    if (it->first != "instanceID" && it->first != "state" && it->first != "eventName"
                        && it->first != "ID" && it->first != "State" && it->first != "Event")
                    {
                        std::cout << "      " << it->first << " = " << it->second << "\n";
                    }
                }
                found++;
            }
        }
    }

    if (found == 0)
        std::cout << "  No snapshots found for this instance.\n";
}


static int askInt(const std::string &prompt)
{
    std::string s = trim(ask(prompt));
    if (s.empty()) return -1;
    std::istringstream iss(s);
    int v = -1;
    if (!(iss >> v)) return -1;
    char extra;
    if (iss >> extra) return -1;
    return v;
}
static std::string execModeName(ExecMode m) { return m == ExecMode::Manual ? "Manual" : "Automated"; }

static std::string typeName(WorkflowType t)
{
    switch (t)
    {
    case WorkflowType::Case:               return "Case";
    case WorkflowType::Project:            return "Project";
    case WorkflowType::Approval:           return "Approval";
    case WorkflowType::DocumentManagement: return "Document Management";
    }
    return "?";
}

static std::string structureName(WorkflowStructure s)
{
    switch (s)
    {
    case WorkflowStructure::Sequential:   return "Sequential";
    case WorkflowStructure::Parallel:     return "Parallel";
    case WorkflowStructure::RuleDriven:   return "Rule-Driven";
    case WorkflowStructure::StateMachine: return "State Machine";
    }
    return "?";
}


// =============================================================================
// ScenarioSpec — everything that varies per industry / workflow type.
// To add a NEW industry application, fill one of these in and hand it to
// buildDefinition(). The engine never needs to change.
// =============================================================================
struct ScenarioSpec
{
    std::string defId;               // registry key (unique)
    std::string industry;            // e.g. "banking", "healthcare", "admissions"
    std::string scenarioName;        // e.g. "Home loan application"
    WorkflowType type;
    WorkflowStructure structure;

    // Data keys the user will fill at instance start.
    std::vector<std::string> dataKeys;

    // Threshold + role configuration for rule-driven routing.
    std::string thresholdKey;        // e.g. "amount", "aps", "severity"
    double      thresholdValue;
    std::string lowRole;
    std::string highRole;

    // Step id prefix (e.g. "loan"). Step ids become "<prefix>-intake" etc.
    std::string stepPrefix;

    // Display labels for the four standard stages (intake / assess / decide / close)
    std::string stageIntake;
    std::string stageAssess;
    std::string stageDecide;
    std::string stageClose;

    // Document kind chosen for document-management scenarios.
    std::string documentKind;        // "contract" or "guideline"
    bool        transactionalDocs;   // true -> TransactionalFactory
};

// =============================================================================
// Build a WorkflowDefinition from a ScenarioSpec + structure choice.
// =============================================================================
static WorkflowDefinition *buildDefinition(const ScenarioSpec &spec)
{
    const std::string &p = spec.stepPrefix;

    CompositeStage *root = new CompositeStage(spec.defId + "-root",
        "[" + spec.industry + "] " + spec.scenarioName + "  <" + structureName(spec.structure) + ">");

    // ----- helpers -----------------------------------------------------------
    auto makeLeaf = [&](const std::string &id, const std::string &label,
                        const std::string &role, WorkItemState *state,
                        RoutingStrategy *routing = nullptr,
                        ApprovalHandler<std::string> *handler = nullptr) -> LeafWorkItem *
    {
        LeafWorkItem *leaf = new LeafWorkItem(id, label);
        leaf->setState(state ? state : new ActiveState());
        if (routing) leaf->setRoutingStrategy(routing);
        else         leaf->setRoutingStrategy(new RoleBasedRouting(role));
        if (handler) leaf->setApprovalHandler(handler);
        return leaf;
    };

    auto stage = [&](const std::string &id, const std::string &label) {
        return new CompositeStage(id, label);
    };

    // ----- standard stage factory functions ---------------------------------
    auto buildIntake = [&]() -> CompositeStage *
    {
        CompositeStage *s = stage(spec.defId + "-intake", spec.stageIntake);
        s->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-submit", "Submit " + spec.scenarioName, "initiator", new ActiveState())));
        s->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-verify", "Verify supporting information", "verifier", new InactiveState())));
        return s;
    };

    auto buildAssess = [&]() -> CompositeStage *
    {
        CompositeStage *s = stage(spec.defId + "-assess", spec.stageAssess);

        // Rule-driven gate: routes on spec.thresholdKey.
        RoutingStrategy *gate = new ValueThresholdRouting(
            spec.thresholdKey, spec.thresholdValue, spec.lowRole, spec.highRole);

        HandlerA<std::string> *h1 = new HandlerA<std::string>();
        HandlerB<std::string> *h2 = new HandlerB<std::string>();
        h1->setNext(h2);

        s->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-assess-primary", "Primary assessment of " + spec.thresholdKey,
                     "assessor", new ActiveState(), gate, h1)));
        return s;
    };

    auto buildDecide = [&]() -> CompositeStage *
    {
        CompositeStage *s = stage(spec.defId + "-decide", spec.stageDecide);
        s->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-decide", "Record decision", "decision-maker",
                     new InactiveState(), nullptr, new HandlerA<std::string>())));
        return s;
    };

    auto buildClose = [&]() -> CompositeStage *
    {
        CompositeStage *s = stage(spec.defId + "-close", spec.stageClose);
        s->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-close", "Close and archive", "clerk", new InactiveState())));
        return s;
    };

    // ----- structure-specific assembly --------------------------------------
    switch (spec.structure)
    {
    case WorkflowStructure::Sequential:
    {
        root->add(std::shared_ptr<WorkflowComponent>(buildIntake()));
        root->add(std::shared_ptr<WorkflowComponent>(buildAssess()));
        root->add(std::shared_ptr<WorkflowComponent>(buildDecide()));
        root->add(std::shared_ptr<WorkflowComponent>(buildClose()));
        break;
    }
    case WorkflowStructure::Parallel:
    {
        // Two intake tracks run side-by-side, then merge into assess/decide.
        CompositeStage *par = stage(spec.defId + "-parallel", "Parallel intake tracks");
        par->add(std::shared_ptr<WorkflowComponent>(buildIntake()));
        CompositeStage *trackB = stage(spec.defId + "-track-b", "Parallel secondary track");
        trackB->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-secondary", "Secondary verification", "verifier",
                     new ActiveState(), nullptr, new HandlerA<std::string>())));
        par->add(std::shared_ptr<WorkflowComponent>(trackB));

        root->add(std::shared_ptr<WorkflowComponent>(par));
        root->add(std::shared_ptr<WorkflowComponent>(buildAssess()));
        root->add(std::shared_ptr<WorkflowComponent>(buildDecide()));
        root->add(std::shared_ptr<WorkflowComponent>(buildClose()));
        break;
    }
    case WorkflowStructure::RuleDriven:
    {
        // Only the rule gate + decision matter; assess is a gate, not a stage.
        CompositeStage *gate = stage(spec.defId + "-gate",
                                     "Rule gate on '" + spec.thresholdKey + "'");
        RoutingStrategy *gateStrategy = new ValueThresholdRouting(
            spec.thresholdKey, spec.thresholdValue, spec.lowRole, spec.highRole);

        HandlerA<std::string> *h1 = new HandlerA<std::string>();
        HandlerB<std::string> *h2 = new HandlerB<std::string>();
        h1->setNext(h2);

        gate->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-gate", "Apply rule: " + spec.thresholdKey,
                     "rule-engine", new ActiveState(), gateStrategy, h1)));

        root->add(std::shared_ptr<WorkflowComponent>(buildIntake()));
        root->add(std::shared_ptr<WorkflowComponent>(gate));
        root->add(std::shared_ptr<WorkflowComponent>(buildDecide()));
        root->add(std::shared_ptr<WorkflowComponent>(buildClose()));
        break;
    }
    case WorkflowStructure::StateMachine:
    {
        // Explicit state chain: submitted -> assessing -> pending-approval -> closed
        CompositeStage *sm = stage(spec.defId + "-states", "State machine");
        sm->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-sm-submitted", "State: Submitted", "initiator",
                     new ActiveState())));
        sm->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-sm-assessing", "State: Assessing", "assessor",
                     new InactiveState())));
        sm->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-sm-pending", "State: Pending Approval", "approver",
                     new InactiveState())));
        sm->add(std::shared_ptr<WorkflowComponent>(
            makeLeaf(p + "-sm-closed", "State: Closed", "clerk",
                     new InactiveState())));

        root->add(std::shared_ptr<WorkflowComponent>(sm));
        break;
    }
    }

    WorkflowDefinition *def = new WorkflowDefinition(spec.defId, spec.industry);
    def->setRootElement(root);
    return def;
}

// =============================================================================
// Collect all leaf ids in tree order — used by the automated driver.
// =============================================================================
static void collectLeafIds(WorkflowComponent *node, std::vector<std::string> &out)
{
    if (!node) return;
    CompositeStage *stage = dynamic_cast<CompositeStage *>(node);
    if (stage)
    {
        for (const auto &child : stage->getChildren())
            collectLeafIds(child.get(), out);
        return;
    }
    if (dynamic_cast<LeafWorkItem *>(node))
        out.push_back(node->getId());
}

// =============================================================================
// Seed a demo scenario for each (type, structure) combination so the user can
// try them all. In a real deployment you'd register only the ones you need.
// =============================================================================
static void seedScenarios()
{
    heading("Registering scenario definitions (generic engine, multiple industries)");

    struct Row { WorkflowType type; WorkflowStructure structure; ScenarioSpec spec; };

    std::vector<Row> rows = {
        // --- Approval ---
        { WorkflowType::Approval, WorkflowStructure::Sequential, {
            "approval-seq", "procurement", "Purchase order approval",
            WorkflowType::Approval, WorkflowStructure::Sequential,
            {"requester", "amount", "costCentre"},
            "amount", 10000.0, "team-lead", "finance-director",
            "po",
            "Intake", "Assessment", "Decision", "Close",
            "contract", true } },
        { WorkflowType::Approval, WorkflowStructure::RuleDriven, {
            "approval-rule", "banking", "Home loan application",
            WorkflowType::Approval, WorkflowStructure::RuleDriven,
            {"applicant", "amount", "creditScore"},
            "amount", 1500000.0, "loan-officer", "credit-committee",
            "loan",
            "Intake", "Rule Gate", "Decision", "Close",
            "contract", true } },

        // --- Case ---
        { WorkflowType::Case, WorkflowStructure::Sequential, {
            "case-seq", "healthcare", "Patient complaint case",
            WorkflowType::Case, WorkflowStructure::Sequential,
            {"patient", "severity", "ward"},
            "severity", 5.0, "ward-nurse", "clinical-lead",
            "case",
            "Intake", "Assessment", "Decision", "Close",
            "guideline", false } },
        { WorkflowType::Case, WorkflowStructure::StateMachine, {
            "case-sm", "municipal", "Service-delivery complaint",
            WorkflowType::Case, WorkflowStructure::StateMachine,
            {"resident", "severity", "suburb"},
            "severity", 3.0, "clerk", "supervisor",
            "svc",
            "Intake", "Assessment", "Decision", "Close",
            "guideline", false } },

        // --- Project ---
        { WorkflowType::Project, WorkflowStructure::Parallel, {
            "project-par", "construction", "Site handover project",
            WorkflowType::Project, WorkflowStructure::Parallel,
            {"projectManager", "budget", "site"},
            "budget", 5000000.0, "site-engineer", "project-director",
            "proj",
            "Initiation", "Parallel Assessment", "Decision", "Close-out",
            "guideline", false } },
        { WorkflowType::Project, WorkflowStructure::Sequential, {
            "project-seq", "education", "Course accreditation project",
            WorkflowType::Project, WorkflowStructure::Sequential,
            {"coordinator", "credits", "faculty"},
            "credits", 360.0, "faculty-officer", "senate",
            "acc",
            "Initiation", "Assessment", "Decision", "Close-out",
            "guideline", false } },

        // --- Document Management ---
        { WorkflowType::DocumentManagement, WorkflowStructure::RuleDriven, {
            "doc-rule", "legal", "Policy document publication",
            WorkflowType::DocumentManagement, WorkflowStructure::RuleDriven,
            {"author", "reviewScore", "classification"},
            "reviewScore", 80.0, "editor", "chief-legal-officer",
            "doc",
            "Draft Intake", "Review Gate", "Approval", "Publish",
            "contract", true } },
        { WorkflowType::DocumentManagement, WorkflowStructure::Sequential, {
            "doc-seq", "publishing", "Journal article lifecycle",
            WorkflowType::DocumentManagement, WorkflowStructure::Sequential,
            {"author", "pages", "journal"},
            "pages", 20.0, "copy-editor", "editor-in-chief",
            "art",
            "Submission", "Assessment", "Decision", "Publish",
            "guideline", false } },
    };

    for (const Row &r : rows)
    {
        // Structurally-tagged id, so a caller can pick (type, structure) directly.
        std::string registryId = r.spec.defId;
        ScenarioSpec copy = r.spec;
        // Structure/type already stored in the spec; rebuild registry key from them
        // so the menu can find it consistently.
        gEngine.registerDefinition(registryId, buildDefinition(copy));
    }

    std::cout << "Registered " << rows.size() << " scenario definitions across "
              << "4 workflow types and 4 structures.\n";
}

// =============================================================================
// Find a registered definition matching (type, structure).
// Returns its defId, or "" if none.
// =============================================================================
static std::string findDefinition(WorkflowType type, WorkflowStructure structure)
{
    static const std::map<std::pair<WorkflowType, WorkflowStructure>, std::string> table = {
        {{WorkflowType::Approval,           WorkflowStructure::Sequential},  "approval-seq"},
        {{WorkflowType::Approval,           WorkflowStructure::RuleDriven},  "approval-rule"},
        {{WorkflowType::Case,               WorkflowStructure::Sequential},  "case-seq"},
        {{WorkflowType::Case,               WorkflowStructure::StateMachine},"case-sm"},
        {{WorkflowType::Project,            WorkflowStructure::Parallel},    "project-par"},
        {{WorkflowType::Project,            WorkflowStructure::Sequential},  "project-seq"},
        {{WorkflowType::DocumentManagement, WorkflowStructure::RuleDriven},  "doc-rule"},
        {{WorkflowType::DocumentManagement, WorkflowStructure::Sequential},  "doc-seq"},
    };
    auto it = table.find({type, structure});
    return it == table.end() ? std::string() : it->second;
}

// =============================================================================
// Interactive chooser
// =============================================================================
static ExecMode chooseExecMode()
{
    heading("Step 1 — Execution mode");
    std::cout << "  1. Manual     (you execute each step and record each decision)\n"
              << "  2. Automated  (engine runs every step, decisions default to APPROVE)\n";
    for (;;)
    {
        int c = askInt("Choose: ");
        if (c == 1) return ExecMode::Manual;
        if (c == 2) return ExecMode::Automated;
        std::cout << "  ! Enter 1 or 2.\n";
    }
}
static WorkflowType chooseType()
{
    heading("Step 2 — Workflow type");
    std::cout << "  1. Case\n"
              << "  2. Project\n"
              << "  3. Approval\n"
              << "  4. Document Management\n";
    for (;;)
    {
        int c = askInt("Choose: ");
        switch (c) {
        case 1: return WorkflowType::Case;
        case 2: return WorkflowType::Project;
        case 3: return WorkflowType::Approval;
        case 4: return WorkflowType::DocumentManagement;
        default: std::cout << "  ! Enter 1-4.\n";
        }
    }
}

static WorkflowStructure chooseStructure()
{
    heading("Step 3 — Workflow structure");
    std::cout << "  1. Sequential\n"
              << "  2. Parallel\n"
              << "  3. Rule-Driven\n"
              << "  4. State Machine\n";
    for (;;)
    {
        int c = askInt("Choose: ");
        switch (c) {
        case 1: return WorkflowStructure::Sequential;
        case 2: return WorkflowStructure::Parallel;
        case 3: return WorkflowStructure::RuleDriven;
        case 4: return WorkflowStructure::StateMachine;
        default: std::cout << "  ! Enter 1-4.\n";
        }
    }
}

// =============================================================================
// Attach observers once per instance.
// =============================================================================
static void attachObservers(WorkflowInstance *inst)
{
    if (gObservers.empty())
    {
        gObservers.push_back(new NotificationObserver());
        gObservers.push_back(new StateObserver());
        gObservers.push_back(new AuditLoggerObserver());
    }
    for (auto *o : gObservers) inst->attach(o);
}

// =============================================================================
// Prompt user for the scenario's data fields.
// =============================================================================
static std::map<std::string, std::string> collectData(const ScenarioSpec &spec)
{
    std::map<std::string, std::string> data;
    heading("Enter scenario data");

    for (const auto &k : spec.dataKeys)
    {
        if (isNumericKey(k))
        {
            if (k == "creditScore")
            {
                double v = askDoubleInRange("  " + k + " (300-850): ", 300, 850);
                data[k] = std::to_string((int)v);
            }
            else if (k == "severity")
            {
                double v = askDoubleInRange("  " + k + " (1-10): ", 1, 10);
                data[k] = std::to_string((int)v);
            }
            else
            {
                double v = askPositiveDouble("  " + k + " (positive number): ");
                data[k] = std::to_string(v);
            }
        }
        else
        {
            data[k] = askName("  " + k + ": ");
        }
    }

    try
    {
        data = validateDataValues(data, spec.dataKeys);
    }
    catch (const std::exception &e)
    {
        std::cout << "  ! Validation failed: " << e.what() << "\n";
        throw;
    }

    data["State"] = "Submitted";
    data["Event"] = "Application submitted";
    data["ID"]    = "";
    return data;
}
// =============================================================================
// Manual driver — user chooses which step to execute.
// =============================================================================
static void runManual(const ScenarioSpec &spec, WorkflowInstance *inst,
                      WorkflowDefinition *def)
{
    heading("Manual execution — " + spec.scenarioName);

    std::vector<std::string> leaves;
    collectLeafIds(def->getRootElement(), leaves);

    std::string realId = "unknown";
    try { realId = inst->getContext()->getData("ID"); } catch (...) {}

    std::function<WorkflowComponent*(WorkflowComponent*, const std::string&)> finder =
        [&](WorkflowComponent* node, const std::string& id) -> WorkflowComponent* {
        if (!node) return nullptr;
        if (node->getId() == id) return node;
        CompositeStage* st = dynamic_cast<CompositeStage*>(node);
        if (st) {
            for (auto& c : st->getChildren()) {
                WorkflowComponent* f = finder(c.get(), id);
                if (f) return f;
            }
        }
        return nullptr;
    };

    for (;;)
    {
        rule();
        std::cout << "Instance " << realId
                  << "  |  state=" << inst->getState() << "\n";

        std::cout << "Available steps:\n";
        for (size_t i = 0; i < leaves.size(); ++i)
            std::cout << "  " << std::setw(2) << i << ". " << leaves[i] << "\n";
        std::cout << "  q  quit manual mode\n";

        std::string choice = ask("Execute step # or 'q': ");
        if (choice == "q" || choice == "Q") break;

        std::istringstream iss(choice);
        size_t idx;
        if (!(iss >> idx) || idx >= leaves.size())
        {
            std::cout << "  ! Invalid step number.\n";
            continue;
        }
        char extra;
        if (iss >> extra) { std::cout << "  ! Invalid step number.\n"; continue; }

        const std::string &sid = leaves[idx];
        WorkflowComponent* comp = finder(def->getRootElement(), sid);
        LeafWorkItem* item = dynamic_cast<LeafWorkItem*>(comp);

        try
        {
            gEngine.executeStep(realId, sid);
            std::cout << "  [Step executed successfully]\n";

            if (sid.find("decide") != std::string::npos ||
                sid.find("gate")   != std::string::npos ||
                sid.find("assess") != std::string::npos)
            {
                std::string d = trim(ask("  Record decision (APPROVE/REJECT/WAITLIST, blank=skip): "));
                if (!d.empty()) {
                    std::string up;
                    for (char c : d) up.push_back((char)std::toupper((unsigned char)c));
                    if (up != "APPROVE" && up != "REJECT" && up != "WAITLIST") {
                        std::cout << "  ! Invalid decision '" << d
                                  << "'. Must be APPROVE, REJECT, or WAITLIST. Skipping.\n";
                        continue;
                    }
                    gEngine.processApproval(realId, sid, up);
                    if (item) {
                        if (up == "REJECT") item->setState(new RejectedState());
                        else                item->setState(new CompletedState());
                    }
                }
            } else {
                if (item) item->setState(new CompletedState());
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "  Error: " << e.what() << "\n";
        }
    }
}
// =============================================================================
// Automated driver — runs every leaf with default decisions.
// =============================================================================
static void runAutomated(const ScenarioSpec &spec, WorkflowInstance *inst,
                         WorkflowDefinition *def)
{
    heading("Automated execution — " + spec.scenarioName);

    std::vector<std::string> leaves;
    collectLeafIds(def->getRootElement(), leaves);

    std::string realId = "unknown";
    try { realId = inst->getContext()->getData("ID"); } catch (...) {}

    int n = 0;
    for (const std::string &sid : leaves)
    {
        std::cout << "  [" << ++n << "/" << leaves.size() << "] "
                  << "execute " << sid << " ... ";
        try
        {
            gEngine.executeStep(realId, sid);
            std::cout << "ok\n";
        }
        catch (const std::exception &e)
        {
            std::cout << "error: " << e.what() << "\n";
            continue;
        }

        if (sid.find("decide") != std::string::npos ||
            sid.find("gate")   != std::string::npos ||
            sid.find("assess") != std::string::npos)
        {
            try
            {
                gEngine.processApproval(realId, sid, "APPROVE");
                std::cout << "      decision: APPROVE\n";
            }
            catch (const std::exception &e)
            {
                std::cout << "      decision failed: " << e.what() << "\n";
            }
        }
    }

    if (!spec.documentKind.empty())
    {
        DocumentFactory *f = spec.transactionalDocs
            ? static_cast<DocumentFactory *>(new TransactionalFactory())
            : static_cast<DocumentFactory *>(new InformationalFactory());
        Document *doc = f->createInstance(spec.documentKind);
        if (doc)
        {
            rule();
            std::cout << "Generated " << (spec.transactionalDocs ? "transactional" : "informational")
                      << " " << spec.documentKind << ":\n";
            doc->print();
        }
        delete doc;
        delete f;
    }

    try
    {
        LegacyiHookAPI legacy;
        LegacyiHookAdapter adapter(&legacy);
        std::map<std::string, std::string> payload;
        payload["instance"] = realId;
        payload["scenario"] = spec.scenarioName;
        payload["industry"] = spec.industry;
        for (const auto &k : spec.dataKeys)
        {
            try { payload[k] = inst->getContext()->getData(k); } catch (...) {}
        }
        rule();
        std::cout << "Syncing to legacy record system:\n";
        adapter.sendPayload(payload);
    }
    catch (const std::exception &e)
    {
        std::cout << "  Legacy sync warning: " << e.what() << "\n";
    }

    rule();
    MetricsReportVisitor visitor;
    gEngine.exportMetrics(&visitor);
    visitor.displayMetrics();
}
// =============================================================================
// Full pipeline for one run.
// =============================================================================
static void runScenario(const ScenarioSpec &spec, ExecMode mode)
{
    heading("Instantiating scenario");
    std::cout << "  industry  : " << spec.industry << "\n"
              << "  scenario  : " << spec.scenarioName << "\n"
              << "  type      : " << typeName(spec.type) << "\n"
              << "  structure : " << structureName(spec.structure) << "\n"
              << "  mode      : " << execModeName(mode) << "\n";

    WorkflowDefinition *def = gEngine.getDefinition(spec.defId);
    if (!def)
    {
        std::cout << "  ERROR: definition " << spec.defId
                  << " not registered. Register it before running.\n";
        return;
    }

    std::map<std::string, std::string> data;
    try
    {
        data = collectData(spec);
    }
    catch (const std::exception &e)
    {
        std::cout << "  Aborting — invalid data: " << e.what() << "\n";
        return;
    }

    if (!spec.thresholdKey.empty() &&
        std::find(spec.dataKeys.begin(), spec.dataKeys.end(), spec.thresholdKey)
            == spec.dataKeys.end())
    {
        std::cout << "  ! Warning: thresholdKey '" << spec.thresholdKey
                  << "' is not among the entered data fields.\n";
    }

    WorkflowInstance *inst = nullptr;
    try
    {
        inst = gEngine.startWorkflow(spec.defId, data);
        if (!inst) throw std::runtime_error("engine returned null instance");
        attachObservers(inst);
    }
    catch (const std::exception &e)
    {
        std::cout << "  Failed to start instance: " << e.what() << "\n";
        return;
    }

    std::cout << "\n>> Instance " << inst->getInstanceID() << " started.\n";

    if (mode == ExecMode::Manual)
        runManual(spec, inst, def);
    else
        runAutomated(spec, inst, def);

    rule();
    AuditCaretaker *care = gEngine.getAuditCaretaker();
    std::cout << "History snapshots: " << care->size() << "\n";
}

// =============================================================================
// Prompt the user to build a NEW industry scenario on the fly.
// This shows how extensible the engine is: no engine changes are required.
// =============================================================================
static ScenarioSpec promptCustomScenario()
{
    heading("Define a new industry application");

    ScenarioSpec s;
    s.industry     = askName("Industry (e.g. logistics, mining, retail): ");
    s.scenarioName = askName("Scenario name (e.g. Shipment release): ");
    s.defId        = "custom-" + s.industry + "-" + std::to_string(std::rand() % 10000);

    s.type      = chooseType();
    s.structure = chooseStructure();

    std::string prefix = trim(ask("Step id prefix (e.g. ship): "));
    if (prefix.empty()) prefix = "step";
    for (unsigned char c : prefix) {
        if (!(std::isalnum(c) || c == '-' || c == '_')) {
            std::cout << "  ! Prefix had invalid chars; using 'step'.\n";
            prefix = "step";
            break;
        }
    }
    s.stepPrefix = prefix;

    auto labelOrDefault = [](const std::string &p, const std::string &def) {
        std::string v = trim(ask(p));
        if (v.empty()) return def;
        if (v.size() > 60) {
            std::cout << "  ! Label too long (max 60); using default.\n";
            return def;
        }
        return v;
    };
    s.stageIntake = labelOrDefault("Intake stage label      [Intake]: ", "Intake");
    s.stageAssess = labelOrDefault("Assessment stage label  [Assessment]: ", "Assessment");
    s.stageDecide = labelOrDefault("Decision stage label    [Decision]: ", "Decision");
    s.stageClose  = labelOrDefault("Close-out stage label   [Close-out]: ", "Close-out");

    std::string tk = trim(ask("Rule/data key for routing (e.g. amount, severity): "));
    if (tk.empty()) tk = "amount";
    s.thresholdKey = tk;

    s.thresholdValue = askPositiveDouble("Threshold value (positive number): ");

    std::string low = trim(ask("Low-side role name  [clerk]: "));
    s.lowRole = low.empty() ? "clerk" : low;
    std::string hi  = trim(ask("High-side role name [manager]: "));
    s.highRole = hi.empty() ? "manager" : hi;

    for (;;)
    {
        std::string keys = trim(ask("Data fields, comma-separated [owner, amount]: "));
        if (keys.empty()) keys = "owner, amount";

        std::vector<std::string> parsed;
        std::string cur;
        bool bad = false;
        for (char c : keys) {
            if (c == ',') {
                cur = trim(cur);
                if (!cur.empty()) {
                    bool okKey = true;
                    for (unsigned char kc : cur)
                        if (!(std::isalnum(kc) || kc == '_' || kc == '-')) okKey = false;
                    if (!okKey) { bad = true; break; }
                    parsed.push_back(cur);
                }
                cur.clear();
            } else cur.push_back(c);
        }
        if (!bad) {
            cur = trim(cur);
            if (!cur.empty()) {
                bool okKey = true;
                for (unsigned char kc : cur)
                    if (!(std::isalnum(kc) || kc == '_' || kc == '-')) okKey = false;
                if (!okKey) bad = true;
                else parsed.push_back(cur);
            }
        }
        if (bad) { std::cout << "  ! Keys may use letters, digits, - and _ only.\n"; continue; }
        if (parsed.empty()) { std::cout << "  ! At least one data field is required.\n"; continue; }
        s.dataKeys = parsed;
        break;
    }

    s.documentKind = askEnum(
        "Document kind to produce (contract/guideline): ",
        {"CONTRACT", "GUIDELINE"}, "CONTRACT");
    for (auto &c : s.documentKind) c = (char)std::tolower((unsigned char)c);

    s.transactionalDocs = (askEnum("Transactional? (y/n) [n]: ",
                                   {"Y","N","YES","NO"}, "N")[0] == 'Y');

    return s;
}

// =============================================================================
// Top-level flow
// =============================================================================
static void runOnce()
{
    ExecMode mode        = chooseExecMode();
    WorkflowType type    = chooseType();
    WorkflowStructure st = chooseStructure();

    std::string defId = findDefinition(type, st);
    if (defId.empty())
    {
        std::cout << "\nNo pre-seeded scenario matches "
                  << typeName(type) << " + " << structureName(st) << ".\n"
                  << "You can define one yourself.\n";
        ScenarioSpec custom = promptCustomScenario();
        custom.type      = type;
        custom.structure = st;
        gEngine.registerDefinition(custom.defId, buildDefinition(custom));
        runScenario(custom, mode);
        return;
    }

    // We need the ScenarioSpec again — rebuild the one we registered.
    // Simplest: keep a lookup of specs by defId.
    static std::map<std::string, ScenarioSpec> specByDefId;
    if (specByDefId.empty())
    {
        // Mirrors seedScenarios().
        specByDefId["approval-seq"] = { "approval-seq", "procurement", "Purchase order approval",
            WorkflowType::Approval, WorkflowStructure::Sequential,
            {"requester","amount","costCentre"}, "amount", 10000.0,
            "team-lead","finance-director","po",
            "Intake","Assessment","Decision","Close","contract",true };
        specByDefId["approval-rule"] = { "approval-rule", "banking", "Home loan application",
            WorkflowType::Approval, WorkflowStructure::RuleDriven,
            {"applicant","amount","creditScore"}, "amount", 1500000.0,
            "loan-officer","credit-committee","loan",
            "Intake","Rule Gate","Decision","Close","contract",true };
        specByDefId["case-seq"] = { "case-seq", "healthcare", "Patient complaint case",
            WorkflowType::Case, WorkflowStructure::Sequential,
            {"patient","severity","ward"}, "severity", 5.0,
            "ward-nurse","clinical-lead","case",
            "Intake","Assessment","Decision","Close","guideline",false };
        specByDefId["case-sm"] = { "case-sm", "municipal", "Service-delivery complaint",
            WorkflowType::Case, WorkflowStructure::StateMachine,
            {"resident","severity","suburb"}, "severity", 3.0,
            "clerk","supervisor","svc",
            "Intake","Assessment","Decision","Close","guideline",false };
        specByDefId["project-par"] = { "project-par", "construction", "Site handover project",
            WorkflowType::Project, WorkflowStructure::Parallel,
            {"projectManager","budget","site"}, "budget", 5000000.0,
            "site-engineer","project-director","proj",
            "Initiation","Parallel Assessment","Decision","Close-out","guideline",false };
        specByDefId["project-seq"] = { "project-seq", "education", "Course accreditation project",
            WorkflowType::Project, WorkflowStructure::Sequential,
            {"coordinator","credits","faculty"}, "credits", 360.0,
            "faculty-officer","senate","acc",
            "Initiation","Assessment","Decision","Close-out","guideline",false };
        specByDefId["doc-rule"] = { "doc-rule", "legal", "Policy document publication",
            WorkflowType::DocumentManagement, WorkflowStructure::RuleDriven,
            {"author","reviewScore","classification"}, "reviewScore", 80.0,
            "editor","chief-legal-officer","doc",
            "Draft Intake","Review Gate","Approval","Publish","contract",true };
        specByDefId["doc-seq"] = { "doc-seq", "publishing", "Journal article lifecycle",
            WorkflowType::DocumentManagement, WorkflowStructure::Sequential,
            {"author","pages","journal"}, "pages", 20.0,
            "copy-editor","editor-in-chief","art",
            "Submission","Assessment","Decision","Publish","guideline",false };
    }

    runScenario(specByDefId.at(defId), mode);
}

// =============================================================================
// Menu
// =============================================================================
static void printMenu()
{
    rule();
    std::cout << "  TIVIDY Generic Workflow Driver\n";
    rule();
    std::cout << "  1. Run a workflow (choose mode/type/structure)\n"
              << "  2. Define a new industry application\n"
              << "  3. List registered definitions\n"
              << "  4. View workflow history (snapshots)\n"
              << "  5. List active instances\n"                 // <--- NEW
              << "  0. Exit\n";
    rule();
}

static void listDefinitions()
{
    heading("Registered definitions");
    const auto &all = gEngine.getAllDefinitions();
    int i = 0;
    for (const auto &kv : all)
        std::cout << "  " << ++i << ". " << kv.first
                  << "  (" << kv.second->getVersion() << ")\n";
}

int main()
{
    std::cout << "\nWelcome to the TIVIDY Generic Workflow Driver.\n"
              << "Four workflow types x four structures x two execution modes,\n"
              << "all driven by one unmodified engine.\n\n";

    seedScenarios();

    bool running = true;
    while (running)
    {
        printMenu();
        int c = askInt("Choice: ");
        switch (c)
        {
        case 1:
            runOnce();
            break;

        case 2:
        {
            ScenarioSpec s = promptCustomScenario();
            gEngine.registerDefinition(s.defId, buildDefinition(s));
            std::cout << "  Registered " << s.defId << "\n";
            ExecMode mode = chooseExecMode();
            runScenario(s, mode);
            break;
        }

        case 3:
            listDefinitions();
            break;

        case 4:
            viewHistory();
            break;

        case 5:
            listInstances();
            break;

        case 0:
            running = false;
            break;

        default:
            std::cout << "  ! Unknown option — enter 0-5.\n";
            break;
        }
    }

    for (auto *o : gObservers) delete o;
    gObservers.clear();

    std::cout << "Goodbye.\n";
    return 0;
}