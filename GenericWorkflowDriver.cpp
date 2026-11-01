/**
 * @file GenericWorkflowDriver.cpp
 * @brief Guided, dynamic workflow definition and execution for launcher option 3.
 */

#include "GenericWorkflowDriver.h"

#include "ActiveState.h"
#include "AuditLoggerObserver.h"
#include "ApprovalHandler.h"
#include "AuditCaretaker.h"
#include "CompositeStage.h"
#include "ConcreteWorkflowBuilder.h"
#include "Document.h"
#include "DocumentFactory.h"
#include "EventObserver.h"
#include "HandlerA.h"
#include "InformationalFactory.h"
#include "LegacyiHookAPI.h"
#include "LegacyiHookAdapter.h"
#include "LeafWorkItem.h"
#include "MetricsReportVisitor.h"
#include "NotificationObserver.h"
#include "RoleBasedRouting.h"
#include "StateObserver.h"
#include "TividyEngineFacade.h"
#include "TransactionalFactory.h"
#include "ValueThresholdRouting.h"
#include "WorkflowContext.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkflowMemento.h"
#include "WorkflowVisitor.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
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

namespace GenericDriver
{
namespace
{
enum class ExecutionMethod { Manual, Automated };
enum class WorkflowType { Case, Project, Approval, DocumentManagement };
enum class WorkflowStructure { RuleDriven, StateMachine, Parallel, Sequential };
enum class FieldType { Text, Number };

struct FieldSpec
{
    std::string name;
    FieldType type;
};

struct WorkflowDefinitionSpec
{
    std::string id;
    std::string industry;
    std::string name;
    ExecutionMethod method;
    WorkflowType type;
    WorkflowStructure structure;
    std::vector<std::string> roles;
    std::vector<FieldSpec> fields;
    std::string ruleField;
    double threshold;
    std::string belowPath;
    std::string abovePath;
    std::vector<std::string> sequence;
};

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
    size_t nextStep;
    std::vector<std::string> completed;
    std::vector<std::string> actions;
    std::vector<HistoryEntry> history;
    std::string result;
    std::string documentPath;
    std::map<std::string, std::string> data;
};

std::string lower(std::string value);
std::string typeName(WorkflowType type);
std::string structureName(WorkflowStructure structure);

class WorkflowTemplate
{
public:
    virtual ~WorkflowTemplate() {}
    virtual std::vector<std::string> sequence() const = 0;
};

class TupleWorkflowTemplate : public WorkflowTemplate
{
private:
    WorkflowType type;
    WorkflowStructure structure;

public:
    TupleWorkflowTemplate(WorkflowType workflowType,
                          WorkflowStructure workflowStructure)
        : type(workflowType), structure(workflowStructure) {}

    std::vector<std::string> sequence() const override
    {
        const std::string prefix = lower(typeName(type)).substr(0, 3) + "-" +
                                   lower(structureName(structure)).substr(0, 3);
        std::vector<std::string> result;
        if (type == WorkflowType::Case && structure == WorkflowStructure::Sequential)
            result = {"Register", "Triage", "Investigate", "Resolve", "Close"};
        else if (type == WorkflowType::Case && structure == WorkflowStructure::Parallel)
            result = {"Register", "Triage", "Investigate Facts", "Gather Evidence",
                      "Join", "Findings Review", "Close"};
        else if (type == WorkflowType::Case && structure == WorkflowStructure::RuleDriven)
            result = {"Register", "Severity Gate", "Quick Resolve",
                      "Investigation and Escalated Review", "Close"};
        else if (type == WorkflowType::Case)
            result = {"Opened", "Triaged", "Investigating", "Resolved", "Closed"};
        else if (type == WorkflowType::Project && structure == WorkflowStructure::Sequential)
            result = {"Initiate", "Plan", "Execute", "Review", "Handover"};
        else if (type == WorkflowType::Project && structure == WorkflowStructure::Parallel)
            result = {"Initiate", "Plan", "Workstream A", "Workstream B", "Join",
                      "Integration Review", "Handover"};
        else if (type == WorkflowType::Project && structure == WorkflowStructure::RuleDriven)
            result = {"Initiate", "Budget Gate", "Lightweight Plan",
                      "Business Case", "Execute", "Handover"};
        else if (type == WorkflowType::Project)
            result = {"Proposed", "Planned", "In Progress", "Completed"};
        else if (type == WorkflowType::Approval && structure == WorkflowStructure::Sequential)
            result = {"Submit", "Validate", "Review", "Decide", "Notify"};
        else if (type == WorkflowType::Approval && structure == WorkflowStructure::Parallel)
            result = {"Submit", "Compliance Check", "Budget Check", "Join",
                      "Consolidated Decision", "Notify"};
        else if (type == WorkflowType::Approval && structure == WorkflowStructure::RuleDriven)
            result = {"Submit", "Value Gate", "Single Approval",
                      "Review and Director Approval", "Notify"};
        else if (type == WorkflowType::Approval)
            result = {"Submitted", "Under Review", "Approved", "Notify"};
        else if (structure == WorkflowStructure::Sequential)
            result = {"Draft", "Review", "Revise", "Approve", "Publish"};
        else if (structure == WorkflowStructure::Parallel)
            result = {"Draft", "Legal Review", "Technical Review", "Join",
                      "Merge Comments", "Approve", "Publish"};
        else if (structure == WorkflowStructure::RuleDriven)
            result = {"Draft", "Classification Gate", "Standard Review",
                      "Legal Review and Executive Sign-off", "Publish", "Archive"};
        else
            result = {"Draft", "In Review", "Approved", "Published", "Archived"};
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = prefix + "-" + result[i];
        return result;
    }
};

class WorkflowTemplateFactory
{
public:
    static std::unique_ptr<WorkflowTemplate> create(
        WorkflowType type, WorkflowStructure structure)
    {
        return std::unique_ptr<WorkflowTemplate>(
            new TupleWorkflowTemplate(type, structure));
    }
};

TividyEngineFacade gEngine;
std::map<std::string, WorkflowDefinitionSpec> gDefinitions;
std::map<std::string, InstanceRecord> gInstances;
std::vector<EventObserver*> gObservers;
unsigned long gDefinitionSequence = 1;
unsigned long gInstanceSequence = 1;

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
    std::cout << prompt;
    std::string value;
    if (!std::getline(std::cin, value))
        throw std::runtime_error("Input ended before the operation was completed.");
    return trim(value);
}

int askInt(const std::string& prompt, int minimum, int maximum)
{
    for (;;)
    {
        const std::string value = ask(prompt);
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
        if (end != value.c_str() && *end == '\0' && (!positive || result > 0.0))
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
        "id", "state", "event", "assignee", "workitem", "approval", "scenario"
    };
    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); ++i)
        if (key == reserved[i])
            return false;
    return true;
}

std::string nowText()
{
    return "session";
}

void addHistory(InstanceRecord& record, const std::string& state,
                const std::string& event, const std::string& note)
{
    record.history.push_back({nowText(), state, event, note});
}

std::vector<std::string> templateSequence(WorkflowType type,
                                          WorkflowStructure structure)
{
    return WorkflowTemplateFactory::create(type, structure)->sequence();
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

WorkflowDefinition* buildDefinition(const WorkflowDefinitionSpec& spec)
{
    // The existing Builder remains the construction entry point for the root.
    // The resulting tree is then enriched with configured routing and approval
    // policies because the legacy Builder interface has no role parameter.
    ConcreteWorkflowBuilder builder;
    builder.reset();
    builder.buildStage(spec.name);
    for (size_t i = 0; i < spec.sequence.size(); ++i)
        builder.buildWorkitem(spec.sequence[i]);
    builder.endStage();
    WorkflowDefinition* built = builder.getResult();
    CompositeStage* root = new CompositeStage(spec.id + "-root", spec.name);
    for (size_t i = 0; i < spec.sequence.size(); ++i)
    {
        const bool approval = (i + 1 == spec.sequence.size() ||
                               spec.type == WorkflowType::Approval ||
                               spec.sequence[i].find("Approval") != std::string::npos ||
                               spec.sequence[i].find("Decision") != std::string::npos ||
                               spec.sequence[i].find("Approve") != std::string::npos);
        LeafWorkItem* item = new LeafWorkItem(
            spec.id + "-" + std::to_string(i + 1), spec.sequence[i]);
        std::string role = spec.roles.empty() ? "process-owner" :
                           spec.roles[i % spec.roles.size()];
        if (spec.structure == WorkflowStructure::RuleDriven &&
            i == 1 && !spec.ruleField.empty())
            item->setRoutingStrategy(new ValueThresholdRouting(
                spec.ruleField, spec.threshold, spec.belowPath, spec.abovePath));
        else
            item->setRoutingStrategy(new RoleBasedRouting(role));
        item->setState(new ActiveState());
        if (approval)
            item->setApprovalHandler(new HandlerA<std::string>());
        root->add(std::shared_ptr<WorkflowComponent>(item));
    }
    delete built;
    WorkflowDefinition* definition = new WorkflowDefinition(spec.id, "1.0");
    definition->setRootElement(root);
    return definition;
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
        gObservers.push_back(new NotificationObserver());
        gObservers.push_back(new StateObserver());
        gObservers.push_back(new AuditLoggerObserver());
    }
    for (size_t i = 0; i < gObservers.size(); ++i)
        instance->attach(gObservers[i]);
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
    DocumentFactory* factory = spec.type == WorkflowType::DocumentManagement ||
                               spec.type == WorkflowType::Case
        ? static_cast<DocumentFactory*>(new InformationalFactory())
        : static_cast<DocumentFactory*>(new TransactionalFactory());
    Document* document = factory->createInstance(
        spec.type == WorkflowType::DocumentManagement ? "contract" : "guideline");
    delete document;
    delete factory;

    const std::string path = "documents/" + instance.reference + "_" +
                             lower(typeName(spec.type)) + "_result.txt";
    std::ofstream output(path.c_str());
    if (!output)
        throw std::runtime_error("Unable to create result document: " + path);
    output << "TIVIDY WORKFLOW RESULT\n"
           << "Reference: " << instance.reference << "\n"
           << "Workflow: " << spec.name << "\n"
           << "Industry: " << spec.industry << "\n"
           << "Tuple: " << tuple(spec) << "\n"
           << "Status: " << instance.status << "\n"
           << "Result: " << instance.result << "\n\n"
           << "Processing history:\n";
    for (size_t i = 0; i < instance.history.size(); ++i)
        output << instance.history[i].state << " | " << instance.history[i].event
               << " | " << instance.history[i].note << "\n";
    return path;
}

void printSequence(const WorkflowDefinitionSpec& spec)
{
    std::cout << "Workflow Sequence:\n";
    for (size_t i = 0; i < spec.sequence.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << spec.sequence[i]
                  << " - " << spec.roles[i % spec.roles.size()] << "\n";
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
              << "6. Cancel\n";
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
        const int action = askInt("Choose: ", 1, 6);
        if (action == 1)
            break;
        if (action == 2) spec.method = chooseMethod();
        else if (action == 3) spec.type = chooseType();
        else if (action == 4) spec.structure = chooseStructure();
        else if (action == 5) { spec.fields.clear(); collectDetails(spec); }
        else throw std::runtime_error("Workflow definition cancelled.");
        spec.sequence = templateSequence(spec.type, spec.structure);
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

void printProcessing(const WorkflowDefinitionSpec& spec, InstanceRecord& record,
                     WorkflowInstance* instance, WorkflowDefinition* definition)
{
    std::vector<LeafWorkItem*> leaves;
    collectLeaves(definition->getRootElement(), leaves);
    const std::string instanceId = instance->getInstanceID();
    std::string lifecycle = "Active";
    addHistory(record, lifecycle, "WORKFLOW_STARTED", "Workflow instance started");
    for (size_t i = record.nextStep; i < leaves.size(); ++i)
    {
        LeafWorkItem* item = leaves[i];
        std::string role;
        try { role = item->getRoutingStrategy()->determineAssignee(*item, *instance->getContext()); }
        catch (const std::exception& error) { role = "routing-error: " + std::string(error.what()); }
        std::cout << "\n  " << (i + 1) << ". " << item->getName() << "\n"
                  << "     Step ID: " << item->getId() << "\n"
                  << "     Assigned role: " << role << "\n"
                  << "     Routing reason: " << item->getRoutingStrategy()->describe() << "\n"
                  << "     Action: ";
        if (spec.method == ExecutionMethod::Manual)
        {
            std::cout << "1 Execute  2 Approve  3 Reject  4 Waitlist  5 Stop\n";
            const int action = askInt("     Choose action: ", 1, 5);
            if (action == 5)
            {
                record.status = "In Progress";
                record.nextStep = i;
                addHistory(record, "Active", "PAUSED", "Manual execution paused");
                return;
            }
            if (action == 3)
            {
                record.status = "Rejected";
                record.nextStep = i;
                record.result = "Rejected by " + role;
                addHistory(record, "Rejected", "REJECTED", "Rejected by " + role);
                std::cout << RED << "     Result: REJECTED\n" << RESET;
                return;
            }
            std::cout << "performed by " << role << "\n";
            record.actions.push_back("Manual action by " + role + " on " + item->getName());
        }
        else
        {
            std::cout << "automatic execution by " << role << "\n";
            record.actions.push_back("Automatic action by " + role + " on " + item->getName());
        }
        try
        {
            gEngine.executeStep(instanceId, item->getId());
            const std::string status = instance->getContext()->getData(
                "workitem:" + item->getId() + ":status");
            if (status == "PENDING_APPROVAL")
                gEngine.processApproval(instanceId, item->getId(), "APPROVE", role);
            record.completed.push_back(item->getName());
            record.nextStep = i + 1;
            addHistory(record, "Active", "WORK_ITEM_COMPLETED", item->getName() + " completed");
            if (spec.structure == WorkflowStructure::RuleDriven && i == 1)
                printRuleEvaluation(spec, record);
        }
        catch (const std::exception& error)
        {
            record.status = "Rejected";
            record.result = error.what();
            addHistory(record, "DefectResolution", "PROCESSING_ERROR", error.what());
            addHistory(record, "Rejected", "REJECTED", error.what());
            std::cout << RED << "     Result: FAIL - " << error.what() << "\n" << RESET;
            return;
        }
    }
    record.status = "Completed";
    record.result = typeName(spec.type) + " workflow completed";
    if (spec.structure == WorkflowStructure::Parallel)
        record.result += "; all branches completed and join satisfied";
    if (spec.structure == WorkflowStructure::StateMachine)
        record.result += "; transition trail recorded";
    addHistory(record, "Completed", "WORKFLOW_COMPLETED", record.result);
    std::cout << GREEN << "\n  Result: " << record.result << "\n" << RESET;
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
    record.reference = "INST-" + std::to_string(gInstanceSequence++);
    record.definitionId = definitionId;
    record.status = "Not Started";
    record.nextStep = 0;
    record.data = collectInstanceData(*spec);
    record.data["ID"] = record.reference;
    record.data["scenario"] = spec->name;
    WorkflowInstance* instance = gEngine.startWorkflow(
        definitionId, record.data);
    attachObservers(instance);
    record.status = "In Progress";
    gInstances[record.reference] = record;
    WorkflowDefinition* definition = gEngine.getDefinition(definitionId);
    heading("WORKFLOW PROCESSING - " + spec->name);
    std::cout << "Reference: " << record.reference << "\n"
              << "Tuple: " << tuple(*spec) << "\n";
    printProcessing(*spec, gInstances[record.reference], instance, definition);
    InstanceRecord& stored = gInstances[record.reference];
    if (stored.status == "Completed")
    {
        stored.documentPath = createDocument(*spec, stored);
        std::cout << "Result document: " << stored.documentPath << "\n";
        try
        {
            LegacyiHookAPI legacy;
            LegacyiHookAdapter adapter(&legacy);
            adapter.sendPayload(stored.data);
        }
        catch (const std::exception& error)
        {
            std::cout << "Legacy sync warning: " << error.what() << "\n";
        }
        MetricsReportVisitor visitor;
        gEngine.exportMetrics(&visitor);
        std::cout << "Visitor summary:\n";
        visitor.displayMetrics();
    }
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

void runGenericWorkflowDriver()
{
    verifyTemplates();
    bool running = true;
    while (running)
    {
        heading("GENERIC WORKFLOW DRIVER");
        std::cout << "1. Define a new workflow\n"
                  << "2. Run a defined workflow\n"
                  << "3. View defined workflows\n"
                  << "4. View workflow status\n"
                  << "5. View workflow history\n"
                  << "6. Generate workflow result document\n"
                  << "7. Choose a different workflow scenario\n"
                  << "8. Exit\n";
        try
        {
            const int choice = askInt("Choose: ", 1, 8);
            if (choice == 1)
            {
                WorkflowDefinitionSpec spec = defineWorkflow();
                gEngine.registerDefinition(spec.id, buildDefinition(spec));
                gDefinitions[spec.id] = spec;
                std::cout << "Workflow registered as " << spec.id << ".\n";
                if (lower(ask("Run it now? (y/n): ")) == "y")
                    runInstance(spec.id);
            }
            else if (choice == 2) chooseAndRun();
            else if (choice == 3) viewDefinitions();
            else if (choice == 4) viewStatus();
            else if (choice == 5) viewHistory();
            else if (choice == 6) generateDocument();
            else if (choice == 7) running = false;
            else running = false;
        }
        catch (const std::exception& error)
        {
            std::cout << "  " << error.what() << "\n";
        }
    }
}

void cleanup()
{
    for (size_t i = 0; i < gObservers.size(); ++i)
        delete gObservers[i];
    gObservers.clear();
}

} // namespace GenericDriver
