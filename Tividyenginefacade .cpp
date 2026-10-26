#include "TividyEngineFacade.h"
#include "WorkflowDefinitionRegistry.h"
#include "InstanceManager.h"
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

    InstanceRecord& requireRecord(InstanceManager* manager, const std::string& instanceId)
    {
        InstanceRecord* record = manager->find(instanceId);
        if (!record)
        {
            throw std::invalid_argument("Unknown workflow instance: " + instanceId);
        }
        return *record;
    }

    WorkflowComponent* requireStep(const InstanceRecord& record, const std::string& stepId)
    {
        WorkflowComponent* step = findComponent(record.definition->getRootElement(), stepId);
        if (!step)
        {
            throw std::invalid_argument("Unknown step: " + stepId);
        }
        return step;
    }

    // The caretaker keeps the pointer; it must delete its mementos.
    void takeSnapshot(AuditCaretaker* caretaker, WorkflowInstance* instance)
    {
        caretaker->addSnapshot(new WorkflowMemento(instance->createMemento()));
    }
}

TividyEngineFacade::TividyEngineFacade()
    : definitionRegistry(new WorkflowDefinitionRegistry()),
      instanceManager(new InstanceManager()),
      auditCaretaker(new AuditCaretaker())
{
}

TividyEngineFacade::~TividyEngineFacade()
{
    delete instanceManager;
    delete definitionRegistry;
    delete auditCaretaker;
}

void TividyEngineFacade::registerDefinition(const std::string& defId, WorkflowDefinition* definition)
{
    definitionRegistry->registerDefinition(defId, definition);
}

WorkflowInstance* TividyEngineFacade::startWorkflow(const std::string& defId,
                                                    const std::map<std::string, std::string>& initialData)
{
    WorkflowDefinition* definition = definitionRegistry->get(defId);
    if (!definition)
    {
        throw std::invalid_argument("Unknown workflow definition: " + defId);
    }

    WorkflowContext* context = new WorkflowContext();
    for (std::map<std::string, std::string>::const_iterator it = initialData.begin();
         it != initialData.end(); ++it)
    {
        context->setData(it->first, it->second);
    }

    WorkflowInstance* instance = new WorkflowInstance(context);
    std::string id = instanceManager->add(instance, context, definition);

    context->setData("ID", id);
    instance->setID();

    takeSnapshot(auditCaretaker, instance); // snapshot 0 = state at start
    return instance;
}

void TividyEngineFacade::executeStep(const std::string& instanceId, const std::string& stepId)
{
    InstanceRecord& record = requireRecord(instanceManager, instanceId);
    WorkflowComponent* step = requireStep(record, stepId);

    takeSnapshot(auditCaretaker, record.instance); // so this step can be rolled back
    step->execute(record.context);
}

void TividyEngineFacade::processApproval(const std::string& instanceId, const std::string& stepId,
                                         const std::string& decision)
{
    InstanceRecord& record = requireRecord(instanceManager, instanceId);
    WorkflowComponent* step = requireStep(record, stepId);

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

    takeSnapshot(auditCaretaker, record.instance);
    record.context->setData("approval:" + stepId, decision);
    handler->processRequest(amount, decision);
}

void TividyEngineFacade::rollbackInstance(const std::string& instanceId, int snapshotIndex)
{
    InstanceRecord& record = requireRecord(instanceManager, instanceId);

    WorkflowMemento* snapshot = auditCaretaker->getSnapshot(snapshotIndex);
    if (!snapshot)
    {
        throw std::out_of_range("No snapshot at index " + std::to_string(snapshotIndex));
    }
    record.instance->restoreMemento(*snapshot);
}

void TividyEngineFacade::exportMetrics(WorkflowVisitor* visitor)
{
    if (!visitor)
    {
        return;
    }
    const std::map<std::string, WorkflowDefinition*>& all = definitionRegistry->getAll();
    for (std::map<std::string, WorkflowDefinition*>::const_iterator it = all.begin();
         it != all.end(); ++it)
    {
        WorkflowComponent* root = it->second ? it->second->getRootElement() : nullptr;
        if (root)
        {
            root->accept(visitor);
        }
    }
}