#include "ProcurementWorkflowDirector.h"

#include "ActiveState.h"
#include "ApprovalHandler.h"
#include "ConcreteWorkflowBuilder.h"
#include "HandlerA.h"
#include "HandlerB.h"
#include "InactiveState.h"
#include "RoleBasedRouting.h"
#include "ValueThresholdRouting.h"
#include "ProcurementRuleHandler.h"
#include "WorkflowDefinition.h"
#include "CompositeStage.h"
#include "LeafWorkItem.h"

WorkflowDefinition* ProcurementWorkflowDirector::constructWorkflow()
{
    ConcreteWorkflowBuilder builder;
    builder.setDefinitionMetadata("construction-procurement", "construction-material-procurement")
           .buildStage("proc-root", "Construction Material Procurement")
           .buildStage("proc-request-stage", "Request and Specification")
           .setInitialState(new ActiveState())
           .setRoutingStrategy(new RoleBasedRouting("site-manager"))
           .buildWorkitem("proc-submit", "Submit material request")
           .setInitialState(new InactiveState())
           .setRoutingStrategy(new RoleBasedRouting("quantity-surveyor"))
           .buildWorkitem("proc-spec", "Verify material specification")
           .endStage()
           .buildStage("proc-sourcing-stage", "Supplier and Cost Review")
           .setInitialState(new ActiveState())
           .setRoutingStrategy(new ValueThresholdRouting(
               "amount", 100000.0, "procurement-officer", "procurement-manager"))
           .setApprovalHandler([this]() {
               ProcurementRuleHandler* first = new ProcurementRuleHandler("specification", ledger);
               ProcurementRuleHandler* supplier = new ProcurementRuleHandler("supplier", ledger);
               ProcurementRuleHandler* authority = new ProcurementRuleHandler("authority", ledger);
               first->setNext(supplier);
               supplier->setNext(authority);
               authority->setNext(new ProcurementRuleHandler("funding", ledger));
               return first;
           }())
           .buildWorkitem("proc-quote", "Review supplier quotation")
           .setInitialState(new InactiveState())
           .setRoutingStrategy(new RoleBasedRouting("finance-approver"))
           .setApprovalHandler([this]() {
               HandlerA<std::string>* first = new HandlerA<std::string>();
               ProcurementRuleHandler* specification =
                   new ProcurementRuleHandler("specification", ledger);
               ProcurementRuleHandler* supplier =
                   new ProcurementRuleHandler("supplier", ledger);
               ProcurementRuleHandler* authority =
                   new ProcurementRuleHandler("authority", ledger);
               ProcurementRuleHandler* funding =
                   new ProcurementRuleHandler("funding", ledger);
               specification->setNext(supplier);
               supplier->setNext(authority);
               authority->setNext(funding);
               first->setNext(specification);
               return first;
           }())
           .buildWorkitem("proc-approve", "Approve purchase order")
           .endStage()
           .buildStage("proc-delivery-stage", "Delivery and Inspection")
           .setInitialState(new InactiveState())
           .setRoutingStrategy(new RoleBasedRouting("site-inspector"))
           .buildWorkitem("proc-inspect", "Inspect delivered materials")
    ;
    WorkflowDefinition* definition = builder.getResult();
    definition->setSuccessResult("APPROVED");
    definition->setCompletionReversible(false);
    CompositeStage* root = static_cast<CompositeStage*>(definition->getRootElement());
    CompositeStage* delivery = static_cast<CompositeStage*>(root->getChildren().back().get());
    static_cast<LeafWorkItem*>(delivery->getChildren().back().get())->setDeferred(true);
    return definition;
}
