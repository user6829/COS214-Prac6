#include "UniversityWorkflowDirector.h"

#include "ActiveState.h"
#include "ApprovalHandler.h"
#include "ConcreteWorkflowBuilder.h"
#include "HandlerA.h"
#include "HandlerB.h"
#include "InactiveState.h"
#include "RoleBasedRouting.h"
#include "ValueThresholdRouting.h"
#include "UniversityRuleHandler.h"
#include "WorkflowDefinition.h"

WorkflowDefinition* UniversityWorkflowDirector::constructWorkflow()
{
    ConcreteWorkflowBuilder builder;
    builder.setDefinitionMetadata("undergraduate-admissions", "informational");

    builder.buildStage("undergrad-root", "informational: undergrad admissions")
           .buildStage("undergrad-intake", "Intake")
           .setInitialState(new ActiveState())
           .setRoutingStrategy(new RoleBasedRouting("applicant"))
           .buildWorkitem("undergrad-submit", "Submit application form")
           .setInitialState(new InactiveState())
           .setRoutingStrategy(new RoleBasedRouting("fee-collector"))
           .setApprovalHandler(new HandlerA<std::string>())
           .buildWorkitem("undergrad-payfee", "Pay application fee")
           .endStage()
           .buildStage("undergrad-screening", "Academic Screening")
           .setInitialState(new ActiveState())
           .setRoutingStrategy(new ValueThresholdRouting(
               "aps", 35.0, "admissions-officer", "senior-reviewer"))
           .setApprovalHandler([]() {
               HandlerA<std::string>* first = new HandlerA<std::string>();
               HandlerB<std::string>* second = new HandlerB<std::string>();
               UniversityRuleHandler* aps = new UniversityRuleHandler("aps");
               UniversityRuleHandler* english = new UniversityRuleHandler("english");
               UniversityRuleHandler* subject = new UniversityRuleHandler("subject");
               UniversityRuleHandler* capacity = new UniversityRuleHandler("capacity");
               aps->setNext(english);
               english->setNext(subject);
               subject->setNext(capacity);
               second->setNext(aps);
               first->setNext(second);
               return first;
           }())
           .buildWorkitem("undergrad-aps", "Review APS and matric results")
           .setInitialState(new InactiveState())
           .setRoutingStrategy(new RoleBasedRouting("language-reviewer"))
           .buildWorkitem("undergrad-english",
                          "Verify NSC English achievement (level 4 minimum)")
           .endStage()
           .buildStage("undergrad-offer", "Offer Decision")
           .setInitialState(new InactiveState())
           .setRoutingStrategy(new RoleBasedRouting("faculty-board"))
           .setApprovalHandler([]() {
               HandlerA<std::string>* first = new HandlerA<std::string>();
               first->setNext(new HandlerB<std::string>());
               return first;
           }())
           .buildWorkitem("undergrad-issue-offer",
                          "Issue offer of admission or rejection")
           .endStage()
           .endStage();
    WorkflowDefinition* definition = builder.getResult();
    definition->setSuccessResult("ACCEPTED");
    return definition;
}
