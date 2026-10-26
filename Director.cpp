#include "Director.h"
#include "DocumentationBuilder.h"
#include "Document.h"
#include "WorkflowBuilder.h"
#include "WorkflowDefinition.h"

Document* Director::constructDocument() {
    if (!docBuilder) return nullptr;
    docBuilder->buildPart();
    return docBuilder->getResult();
}

WorkflowDefinition* Director::constructWorkflow() {
    if (!wfBuilder) return nullptr;
    
    // Example of directing a complex build process:
    wfBuilder->reset();
    wfBuilder->buildStage("Approval")
             .buildWorkitem("Manager Sign-off")
             .buildWorkitem("HR Verification")
             .buildStage("Execution")
             .buildWorkitem("System Trigger");
             
    return wfBuilder->getResult();
}