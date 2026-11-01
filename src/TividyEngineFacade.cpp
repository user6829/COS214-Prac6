#include "TividyEngineFacade.h"
#include "AuditCaretaker.h"
#include "WorkflowMemento.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkflowContext.h"
#include "WorkflowComponent.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"
#include "ApprovalHandler.h"
#include "WorkflowVisitor.h"
#include <stdexcept>
#include <set>
#include <sstream>
#include "RoutingStrategy.h"
#include <memory>
#include "HandlerA.h"
#include "MetricsReportVisitor.h"
#include "DefectResolutionState.h"

namespace
{
    void collectWorkItems(WorkflowComponent* node, std::vector<LeafWorkItem*>& items)
    {
        if (!node) return;
        if (LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(node)) { items.push_back(leaf); return; }
        if (CompositeStage* stage = dynamic_cast<CompositeStage*>(node)) {
            const std::vector<std::shared_ptr<WorkflowComponent> >& children = stage->getChildren();
            for (size_t i = 0; i < children.size(); ++i) collectWorkItems(children[i].get(), items);
        }
    }

    void validateTree(WorkflowComponent* node, std::set<std::string>& ids)
    {
        if (!node || node->getId().empty() || !ids.insert(node->getId()).second)
            throw std::invalid_argument("Workflow components require unique nonempty IDs");
        if (CompositeStage* stage = dynamic_cast<CompositeStage*>(node))
            for (const auto& child : stage->getChildren()) validateTree(child.get(), ids);
    }

    // Depth-first search of the composite tree for a component with this id.
    WorkflowComponent* findComponent(WorkflowComponent* node, const std::string& id)
    {
        if (!node)
        {
            return nullptr;
        }
        if (node->getId() == id)
        {
            return node;
        }
        CompositeStage* stage = dynamic_cast<CompositeStage*>(node);
        if (stage)
        {
            const std::vector<std::shared_ptr<WorkflowComponent> >& children = stage->getChildren();
            for (size_t i = 0; i < children.size(); ++i)
            {
                WorkflowComponent* found = findComponent(children[i].get(), id);
                if (found)
                {
                    return found;
                }
            }
        }
        return nullptr;
    }
} // namespace

TividyEngineFacade::TividyEngineFacade()
    : auditCaretaker(new AuditCaretaker()), nextInstanceNumber(1)
{
}

TividyEngineFacade::~TividyEngineFacade()
{
    for (std::map<std::string, InstanceRecord>::iterator it = instances.begin();
         it != instances.end(); ++it)
    {
        // WorkflowInstance owns and deletes its WorkflowContext.
        delete it->second.instance;
    }
    for (std::map<std::string, WorkflowDefinition*>::iterator it = definitionRegistry.begin();
         it != definitionRegistry.end(); ++it)
    {
        delete it->second;
    }
    for (std::vector<WorkflowDefinition*>::iterator it = retiredDefinitions.begin();
         it != retiredDefinitions.end(); ++it)
    {
        delete *it;
    }
    delete auditCaretaker;
}

TividyEngineFacade::InstanceRecord& TividyEngineFacade::requireInstance(const std::string& instanceId)
{
    std::map<std::string, InstanceRecord>::iterator it = instances.find(instanceId);
    if (it == instances.end())
    {
        throw std::invalid_argument("Unknown workflow instance: " + instanceId);
    }
    return it->second;
}

// The caretaker keeps the pointer; it must delete its mementos.
void TividyEngineFacade::snapshot(WorkflowInstance* instance)
{
    auditCaretaker->addSnapshot(instance->createMemento());
}

void TividyEngineFacade::registerDefinition(const std::string& defId, WorkflowDefinition* definition)
{
    if (!definition)
    {
        return;
    }
    // Ownership transfers even if validation fails.
    std::unique_ptr<WorkflowDefinition> incoming;
    const auto existing = definitionRegistry.find(defId);
    if (existing != definitionRegistry.end() && existing->second == definition) return;
    for (const auto& registered : definitionRegistry)
        if (registered.second == definition) throw std::invalid_argument("Definition already owned under another key");
    for (WorkflowDefinition* retired : retiredDefinitions)
        if (retired == definition) throw std::invalid_argument("Cannot re-register a retired definition pointer");
    incoming.reset(definition);
    std::set<std::string> ids;
    validateTree(definition->getRootElement(), ids);
    std::vector<LeafWorkItem*> leaves;
    collectWorkItems(definition->getRootElement(), leaves);
    if (leaves.empty()) throw std::invalid_argument("Workflow requires work items");
    incoming.release();
    std::map<std::string, WorkflowDefinition*>::iterator it = definitionRegistry.find(defId);
    if (it != definitionRegistry.end())
    {
        if (it->second != definition)
            retiredDefinitions.push_back(it->second);
        it->second = definition;
        return;
    }
    definitionRegistry[defId] = definition;
}

WorkflowInstance* TividyEngineFacade::startWorkflow(const std::string& defId,
                                                    const std::map<std::string, std::string>& initialData)
{
    std::map<std::string, WorkflowDefinition*>::iterator def = definitionRegistry.find(defId);
    if (def == definitionRegistry.end())
    {
        throw std::invalid_argument("Unknown workflow definition: " + defId);
    }

    WorkflowContext* context = new WorkflowContext();
    for (std::map<std::string, std::string>::const_iterator it = initialData.begin();
         it != initialData.end(); ++it)
    {
        if (it->first.compare(0, 9, "workitem:") == 0 ||
            it->first.compare(0, 9, "assignee:") == 0 ||
            it->first.compare(0, 9, "approval:") == 0 ||
            it->first.compare(0, 5, "rule:") == 0 || it->first == "budgetCommitted") continue;
        context->setData(it->first, it->second);
    }

    std::string id = "INST-" + std::to_string(nextInstanceNumber++);
    context->setData("ID", id);

    WorkflowInstance* instance = new WorkflowInstance(context);
    context->setData("definitionId", defId);
    context->setData("Result", "");
    context->setData("State", "Inactive");
    context->setData("Event", "WORKFLOW_STARTED");
    instance->setID();
    instance->setState();
    instance->setEvent();

    InstanceRecord record;
    record.instance = instance;
    record.context = context;
    record.definition = def->second;
    instances[id] = record;

    snapshot(instance); // snapshot 0 = state at start
    return instance;
}

namespace {
std::string statusOf(WorkflowContext* context, const std::string& id)
{
    try { return context->getData("workitem:" + id + ":status"); }
    catch (const std::out_of_range&) { return "NEW"; }
}
bool finished(const std::string& status)
{
    return status == "COMPLETED" || status == "APPROVED" || status == "SKIPPED";
}
}

void TividyEngineFacade::recordEvent(const std::string& instanceId, const std::string& event)
{
    InstanceRecord& record = requireInstance(instanceId);
    record.instance->publish(record.instance->getState(), event);
    snapshot(record.instance);
}

LeafWorkItem* TividyEngineFacade::getNextWorkItem(const std::string& instanceId)
{
    InstanceRecord& record = requireInstance(instanceId);
    if (record.instance->getState() == "Rejected" ||
        record.instance->getState() == "Completed") return nullptr;
    std::vector<LeafWorkItem*> items;
    collectWorkItems(record.definition->getRootElement(), items);
    for (LeafWorkItem* item : items)
        if (!item->isDeferred() && item->isEnabled(*record.context) &&
            !finished(statusOf(record.context, item->getId()))) return item;
    return nullptr;
}

void TividyEngineFacade::updateOutcome(InstanceRecord& record, const std::string& event)
{
    std::vector<LeafWorkItem*> items;
    collectWorkItems(record.definition->getRootElement(), items);
    bool complete = true;
    for (LeafWorkItem* item : items) {
        if (item->isDeferred()) continue;
        if (!item->isEnabled(*record.context)) {
            record.instance->transitionWorkItem(item->getId(), "SKIPPED");
            continue;
        }
        const std::string status = statusOf(record.context, item->getId());
        if (status == "REJECTED") {
            record.context->setData("Result", "REJECTED");
            record.instance->publish("Rejected", "WORKFLOW_REJECTED:" + item->getId());
            return;
        }
        complete = complete && finished(status);
    }
    record.context->setData("Result", complete ? record.definition->getSuccessResult() : "");
    record.instance->publish(complete ? "Completed" : "Active",
                            complete ? "WORKFLOW_COMPLETED" : event);
}

void TividyEngineFacade::executeStep(const std::string& instanceId, const std::string& stepId)
{
    InstanceRecord& record = requireInstance(instanceId);
    LeafWorkItem* item = dynamic_cast<LeafWorkItem*>(
        findComponent(record.definition->getRootElement(), stepId));
    if (!item) throw std::invalid_argument("Unknown work item: " + stepId);
    if (item != getNextWorkItem(instanceId))
        throw std::runtime_error("Work item is not the next enabled prerequisite: " + stepId);
    if (!record.instance->getWorkItemState(stepId).canExecute())
        throw std::runtime_error("Work item cannot execute in its current state: " + stepId);

    if (record.context->getData("definitionId") == "undergraduate-admissions") {
        std::set<std::string> subjects;
        std::istringstream input(record.context->getData("subjects"));
        std::string subject;
        while (std::getline(input, subject, ',')) {
            const auto first = subject.find_first_not_of(" ");
            subject = first == std::string::npos ? "" : subject.substr(first, subject.find_last_not_of(" ") - first + 1);
            if (subject.empty() || !subjects.insert(subject).second)
                throw std::invalid_argument("Form validation: duplicate or empty school subject");
        }
        for (const auto& field : std::vector<std::pair<std::string, int> >{{"mathMark", 100}, {"aps", 42}, {"englishLevel", 7}}) {
            const std::string text = record.context->getData(field.first);
            std::size_t parsed = 0;
            const int value = std::stoi(text, &parsed);
            if (parsed != text.size() || value < (field.first == "englishLevel" ? 1 : 0) || value > field.second)
                throw std::invalid_argument("Form validation: invalid " + field.first);
        }
        if (subjects.size() != 7 || (subjects.count("Mathematics") + subjects.count("Mathematical Literacy")) != 1)
            throw std::invalid_argument("Form validation: seven distinct subjects and exactly one mathematics type required");
    }
    const std::map<std::string, std::string> before = record.context->getAll();
    snapshot(record.instance);
    try { item->execute(record.context); }
    catch (...) { record.context->replaceAll(before); throw; }
    const bool defect = item->getApprovalHandler() &&
        record.context->getData("approval:" + stepId + ":precheck") == "REJECT";
    record.instance->transitionWorkItem(stepId, defect ? "DEFECT_RESOLUTION" :
        (item->getApprovalHandler() ? "PENDING_APPROVAL" : "COMPLETED"));
    record.instance->getWorkItemState(stepId).handle(item);
    if (defect) record.instance->publish("DefectResolution", "RULES_FAILED:" + stepId);
    else updateOutcome(record, "WORK_ITEM_EXECUTED:" + stepId);
    if (item->getRoutingStrategy())
        recordEvent(instanceId, "ROUTING:" + stepId + ":" + item->getRoutingStrategy()->describe() +
                    ": assigned " + record.context->getData("assignee:" + stepId));
    if (item->getApprovalHandler()) {
        const int count = std::stoi(record.context->getData("rule:" + stepId + ":count"));
        for (int i = 1; i <= count; ++i) {
            const std::string prefix = "rule:" + stepId + ":" + std::to_string(i) + ":";
            recordEvent(instanceId, "SCREENING:" + record.context->getData(prefix + "name") + ":" +
                record.context->getData(prefix + "passed") + ":" + record.context->getData(prefix + "compared") +
                ":" + record.context->getData(prefix + "reason"));
        }
    }
    if (item->getApprovalHandler() && !defect)
        recordEvent(instanceId, "SCREENING_PASSED_PENDING_APPROVAL:" + stepId);
    snapshot(record.instance);
}

void TividyEngineFacade::processApproval(const std::string& instanceId,
    const std::string& stepId, const std::string& decision, const std::string& actorRole)
{
    InstanceRecord& record = requireInstance(instanceId);
    LeafWorkItem* item = dynamic_cast<LeafWorkItem*>(
        findComponent(record.definition->getRootElement(), stepId));
    if (!item || !item->getApprovalHandler())
        throw std::invalid_argument("No approval handler on work item: " + stepId);
    if (decision != "APPROVE" && decision != "REJECT" && decision != "WAITLIST")
        throw std::invalid_argument("Decision must be APPROVE, REJECT, or WAITLIST");
    if (item != getNextWorkItem(instanceId) ||
        !record.instance->getWorkItemState(stepId).canApprove())
        throw std::runtime_error("Work item is not awaiting a decision: " + stepId);
    if (record.context->getData("assignee:" + stepId) != actorRole)
        throw std::runtime_error("Decision denied: actor does not match routed assignee");
    const std::map<std::string, std::string> before = record.context->getAll();
    snapshot(record.instance);
    std::string result;
    try {
        record.context->setData("rule:" + stepId + ":count", "0");
        std::string amount = "0";
        try { amount = record.context->getData("amount"); } catch (const std::out_of_range&) {}
        result = item->getApprovalHandler()->processRequest(amount, decision, *record.context, stepId);
        if (result != "APPROVE" && result != "REJECT" && result != "WAITLIST")
            throw std::runtime_error("Handler returned an invalid decision");
    } catch (...) { record.context->replaceAll(before); throw; }
    record.context->setData("approval:" + stepId, result);
    record.instance->transitionWorkItem(stepId, result == "REJECT" ? "REJECTED" :
        (result == "APPROVE" ? "APPROVED" : "WAITLISTED"));
    record.instance->getWorkItemState(stepId).handle(item);
    if (result == "WAITLIST") record.instance->publish("WAITLISTED", "APPROVAL_WAITLIST:" + stepId);
    else updateOutcome(record, "APPROVAL_" + result + ":" + stepId);
    recordEvent(instanceId, "DECISION:" + stepId + ": actor " + actorRole + ": requested " + decision + ": result " + result);
    const int count = std::stoi(record.context->getData("rule:" + stepId + ":count"));
    for (int i = 1; i <= count; ++i) {
        const std::string prefix = "rule:" + stepId + ":" + std::to_string(i) + ":";
        if (record.context->getData(prefix + "passed") == "false")
            recordEvent(instanceId, "DECISION_RULE_FAILED:" + record.context->getData(prefix + "reason"));
    }
    snapshot(record.instance);
}

void TividyEngineFacade::advanceWorkflow(const std::string& instanceId, const std::string& decision)
{
    if (decision != "APPROVE" && decision != "REJECT" && decision != "WAITLIST")
        throw std::invalid_argument("Invalid decision");
    InstanceRecord& record = requireInstance(instanceId);
    LeafWorkItem* next = getNextWorkItem(instanceId);
    if (!next) throw std::runtime_error("Workflow is terminal; no work available");
    if (!next->getApprovalHandler() && decision != "APPROVE")
        throw std::runtime_error("This work item does not accept approval decisions");
    if (statusOf(record.context, next->getId()) == "NEW") executeStep(instanceId, next->getId());
    if (next->getApprovalHandler()) {
        const std::string selected = statusOf(record.context, next->getId()) == "DEFECT_RESOLUTION"
            ? "REJECT" : decision;
        processApproval(instanceId, next->getId(), selected,
                        record.context->getData("assignee:" + next->getId()));
    }
}

void TividyEngineFacade::runToCompletion(const std::string& instanceId, ProgressCallback progress)
{
    while (LeafWorkItem* next = getNextWorkItem(instanceId)) {
        if (requireInstance(instanceId).instance->getState() == "WAITLISTED") return;
        if (progress) progress(*next, true);
        advanceWorkflow(instanceId);
        if (progress) progress(*next, false);
    }
}

bool TividyEngineFacade::canGenerateDocuments(const std::string& instanceId)
{
    InstanceRecord& record = requireInstance(instanceId);
    const std::string state = record.instance->getState();
    if (state != "Completed" && state != "Rejected") return false;
    std::vector<LeafWorkItem*> items;
    collectWorkItems(record.definition->getRootElement(), items);
    bool rejected = false;
    for (LeafWorkItem* item : items) {
        if (item->isDeferred() || !item->isEnabled(*record.context)) continue;
        const std::string status = statusOf(record.context, item->getId());
        rejected = rejected || status == "REJECTED";
        if (state == "Completed" && !finished(status)) return false;
    }
    return state == "Completed" || rejected;
}

std::string TividyEngineFacade::getOutcome(const std::string& instanceId)
{
    return canGenerateDocuments(instanceId)
        ? requireInstance(instanceId).context->getData("Result") : "";
}

void TividyEngineFacade::rollbackInstance(const std::string& instanceId, int snapshotIndex)
{
    InstanceRecord& record = requireInstance(instanceId);
    if (record.instance->getState() == "Completed" && !record.definition->isCompletionReversible())
        throw std::runtime_error("Completed workflow has irreversible effects; compensation is required");
    WorkflowMemento* saved = auditCaretaker->getSnapshot(instanceId, snapshotIndex);
    if (!saved) throw std::out_of_range("No snapshot at index " + std::to_string(snapshotIndex));
    record.instance->restoreMemento(saved);
    record.instance->publish(record.instance->getState(), "ROLLBACK");
    snapshot(record.instance);
}

void TividyEngineFacade::exportMetrics(WorkflowVisitor* visitor)
{
    if (!visitor)
    {
        return;
    }
    MetricsReportVisitor* metrics = dynamic_cast<MetricsReportVisitor*>(visitor);
    if (metrics)
        metrics->reset();

    for (std::map<std::string, WorkflowDefinition*>::iterator it = definitionRegistry.begin();
         it != definitionRegistry.end(); ++it)
    {
        WorkflowComponent* root = it->second ? it->second->getRootElement() : nullptr;
        if (root)
        {
            root->accept(visitor);
        }
    }
    if (metrics)
    {
        for (std::map<std::string, InstanceRecord>::const_iterator it = instances.begin();
             it != instances.end(); ++it)
        {
            metrics->recordInstance(it->second.context->getAll());
        }
    }
}

WorkflowInstance* TividyEngineFacade::getInstance(const std::string& instanceId)
{
    std::map<std::string, InstanceRecord>::iterator it = instances.find(instanceId);
    if (it == instances.end()) return nullptr;
    return it->second.instance;
}

WorkflowDefinition* TividyEngineFacade::getDefinition(const std::string& defId)
{
    std::map<std::string, WorkflowDefinition*>::iterator it =
        definitionRegistry.find(defId);
    if (it == definitionRegistry.end()) return nullptr;
    return it->second;
}

const std::map<std::string, WorkflowDefinition*>&
TividyEngineFacade::getAllDefinitions() const
{
    return definitionRegistry;
}

AuditCaretaker* TividyEngineFacade::getAuditCaretaker() const
{
    return auditCaretaker;
}

std::vector<std::string> TividyEngineFacade::getInstanceIds() const
{
    std::vector<std::string> ids;
    for (const auto& entry : instances) ids.push_back(entry.first);
    return ids;
}
