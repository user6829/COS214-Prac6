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

namespace
{
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
}

TividyEngineFacade::TividyEngineFacade()
    : auditCaretaker(new AuditCaretaker()), nextInstanceNumber(1)
{
}

TividyEngineFacade::~TividyEngineFacade()
{
    for (std::map<std::string, InstanceRecord>::iterator it = instances.begin();
         it != instances.end(); ++it)
    {
        delete it->second.instance;
        delete it->second.context;
    }
    for (std::map<std::string, WorkflowDefinition*>::iterator it = definitionRegistry.begin();
         it != definitionRegistry.end(); ++it)
    {
        delete it->second;
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
    auditCaretaker->addSnapshot(new WorkflowMemento(instance->createMemento()));
}

void TividyEngineFacade::registerDefinition(const std::string& defId, WorkflowDefinition* definition)
{
    if (!definition)
    {
        return;
    }
    std::map<std::string, WorkflowDefinition*>::iterator it = definitionRegistry.find(defId);
    if (it != definitionRegistry.end())
    {
        if (it->second != definition)
        {
            delete it->second;
        }
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
        context->setData(it->first, it->second);
    }

    std::string id = "INST-" + std::to_string(nextInstanceNumber++);
    context->setData("ID", id);

    WorkflowInstance* instance = new WorkflowInstance(context);
    instance->setID();

    InstanceRecord record;
    record.instance = instance;
    record.context = context;
    record.definition = def->second;
    instances[id] = record;

    snapshot(instance); // snapshot 0 = state at start
    return instance;
}

void TividyEngineFacade::executeStep(const std::string& instanceId, const std::string& stepId)
{
    InstanceRecord& record = requireInstance(instanceId);
    WorkflowComponent* step = findComponent(record.definition->getRootElement(), stepId);
    if (!step)
    {
        throw std::invalid_argument("Unknown step: " + stepId);
    }

    snapshot(record.instance); // so this step can be rolled back
    step->execute(record.context);
}

void TividyEngineFacade::processApproval(const std::string& instanceId, const std::string& stepId,
                                         const std::string& decision)
{
    InstanceRecord& record = requireInstance(instanceId);
    WorkflowComponent* step = findComponent(record.definition->getRootElement(), stepId);
    if (!step)
    {
        throw std::invalid_argument("Unknown step: " + stepId);
    }

    LeafWorkItem* item = dynamic_cast<LeafWorkItem*>(step);
    if (!item)
    {
        throw std::invalid_argument("Step is not a work item: " + stepId);
    }
    ApprovalHandler* handler = item->getApprovalHandler();
    if (!handler)
    {
        throw std::runtime_error("No approval handler set on step: " + stepId);
    }

    // The amount comes from the instance's data; getData throws if the key is missing.
    double amount = 0.0;
    try
    {
        amount = std::stod(record.context->getData("amount"));
    }
    catch (const std::exception&)
    {
        amount = 0.0;
    }

    snapshot(record.instance);
    record.context->setData("approval:" + stepId, decision);
    handler->processRequest(amount, decision);
}

void TividyEngineFacade::rollbackInstance(const std::string& instanceId, int snapshotIndex)
{
    InstanceRecord& record = requireInstance(instanceId);

    WorkflowMemento* saved = auditCaretaker->getSnapshot(snapshotIndex);
    if (!saved)
    {
        throw std::out_of_range("No snapshot at index " + std::to_string(snapshotIndex));
    }
    record.instance->restoreMemento(*saved);
}

void TividyEngineFacade::exportMetrics(WorkflowVisitor* visitor)
{
    if (!visitor)
    {
        return;
    }
    for (std::map<std::string, WorkflowDefinition*>::iterator it = definitionRegistry.begin();
         it != definitionRegistry.end(); ++it)
    {
        WorkflowComponent* root = it->second ? it->second->getRootElement() : nullptr;
        if (root)
        {
            root->accept(visitor);
        }
    }
}