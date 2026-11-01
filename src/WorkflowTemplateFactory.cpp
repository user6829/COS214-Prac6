#include "WorkflowTemplateFactory.h"
#include "ConcreteWorkflowBuilder.h"
#include "WorkflowDefinition.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"
#include "ActiveState.h"
#include "RoleBasedRouting.h"
#include "ValueThresholdRouting.h"
#include "HandlerA.h"
#include <cctype>
#include <memory>
#include <stdexcept>
namespace GenericDriver {
namespace {
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

std::string lower(std::string value)
{
    for (size_t i = 0; i < value.size(); ++i)
        value[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
    return value;
}
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


}
std::vector<std::string> WorkflowTemplateFactory::sequence(WorkflowType type, WorkflowStructure structure)
{
    return TupleWorkflowTemplate(type, structure).sequence();
}
WorkflowDefinition* WorkflowTemplateFactory::createDefinition(const WorkflowDefinitionSpec& spec)
{
    if (spec.roles.empty() || spec.sequence.empty())
        throw std::invalid_argument("A workflow needs roles and tasks");
    if (spec.structure == WorkflowStructure::RuleDriven && spec.ruleField.empty())
        throw std::invalid_argument("Rule-driven workflows need a numeric rule");
    if ((!spec.stageNames.empty() && spec.stageNames.size() != spec.sequence.size()) ||
        (!spec.workItemRoles.empty() && spec.workItemRoles.size() != spec.sequence.size()))
        throw std::invalid_argument("Stage/role configuration must match the work items");
    if (spec.structure == WorkflowStructure::RuleDriven && spec.sequence.size() < 4)
        throw std::invalid_argument("Rule-driven workflows require a gate and two branch tasks");
    ConcreteWorkflowBuilder builder;
    builder.setDefinitionMetadata(spec.id, "1.0");
    builder.buildStage(spec.id + "-root", spec.name);
    std::string currentStage;
    bool stageOpen = false;
    for (std::size_t i = 0; i < spec.sequence.size(); ++i) {
        const std::string stage = spec.stageNames.empty() ? spec.sequence[i] : spec.stageNames[i];
        if (stage.empty() || spec.sequence[i].empty()) throw std::invalid_argument("Stage and work-item names cannot be empty");
        if (!stageOpen || stage != currentStage) {
            if (stageOpen) builder.endStage();
            builder.buildStage(spec.id + "-stage-" + std::to_string(i + 1), stage);
            currentStage = stage;
            stageOpen = true;
        }
        const std::string role = spec.workItemRoles.empty() ? spec.roles[i % spec.roles.size()] : spec.workItemRoles[i];
        if (!spec.ruleField.empty() && i == 1)
            builder.setRoutingStrategy(new ValueThresholdRouting(
                spec.ruleField, spec.threshold, spec.belowPath, spec.abovePath));
        else builder.setRoutingStrategy(new RoleBasedRouting(role));
        builder.setInitialState(new ActiveState())
               .setApprovalHandler(new HandlerA<std::string>())
               .buildWorkitem(spec.id + "-" + std::to_string(i + 1), spec.sequence[i]);
    }
    if (stageOpen) builder.endStage();
    builder.endStage();
    std::unique_ptr<WorkflowDefinition> result(builder.getResult());
    if (spec.structure == WorkflowStructure::RuleDriven) {
        CompositeStage* root = static_cast<CompositeStage*>(result->getRootElement());
        std::vector<LeafWorkItem*> leaves;
        for (const auto& child : root->getChildren()) {
            CompositeStage* stage = static_cast<CompositeStage*>(child.get());
            for (const auto& item : stage->getChildren()) leaves.push_back(static_cast<LeafWorkItem*>(item.get()));
        }
        for (std::size_t i = 2; i <= 3; ++i)
            leaves.at(i)->setCondition(spec.ruleField, spec.threshold, i == 3);
    }
    return result.release();
}
}
