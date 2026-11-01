#include "Director.h"
#include "DocumentationBuilder.h"
#include "Document.h"
#include "WorkflowBuilder.h"
#include "WorkflowDefinition.h"
#include "TividyEngineFacade.h"
#include "WorkflowInstance.h"
#include "ContractBuilder.h"
#include "GuidelineBuilder.h"
#include <memory>
#include <stdexcept>

Document* Director::constructDocument() {
    if (!docBuilder) return nullptr;
    docBuilder->buildPart();
    return docBuilder->getResult();
}

WorkflowDefinition* Director::constructWorkflow() {
    if (!wfBuilder) return nullptr;
    
    // Example of directing a complex build process:
    wfBuilder->reset();
    wfBuilder->buildStage("Workflow")
             .buildStage("Approval")
             .buildWorkitem("Manager Sign-off")
             .buildWorkitem("HR Verification")
             .endStage()
             .buildStage("Execution")
             .buildWorkitem("System Trigger");
             
    return wfBuilder->getResult();
}

namespace {
Document* buildWorkflowDocument(TividyEngineFacade& engine, const std::string& id,
    const std::string& format, const std::map<std::string, std::string>& metadata,
    bool contract, bool transactional)
{
    if (!engine.canGenerateDocuments(id)) throw std::runtime_error("Documents require a terminal workflow outcome");
    std::map<std::string, std::string> values = metadata;
    for (const auto& entry : engine.getInstance(id)->getContext()->getAll()) values[entry.first] = entry.second;
    values["document:format"] = format;
    std::unique_ptr<DocumentationBuilder> builder;
    if (contract) builder.reset(new ContractBuilder(transactional));
    else builder.reset(new GuidelineBuilder(transactional));
    builder->setWorkflowData(values);
    Director director(builder.get());
    return director.constructDocument();
}
}
Document* Director::constructUniversityDocument(TividyEngineFacade& engine, const std::string& id, bool contract)
{
    WorkflowInstance* instance = engine.getInstance(id);
    if (!instance || instance->getContext()->getData("definitionId") != "undergraduate-admissions")
        throw std::invalid_argument("University documents require an admissions instance");
    if (contract && engine.getOutcome(id) != "ACCEPTED")
        throw std::runtime_error("A university contract requires an approved application");
    return buildWorkflowDocument(engine, id, "university", {}, contract, contract);
}
Document* Director::constructProcurementDocument(TividyEngineFacade& engine, const std::string& id)
{
    WorkflowInstance* instance = engine.getInstance(id);
    if (!instance || instance->getContext()->getData("definitionId") != "construction-procurement")
        throw std::invalid_argument("Procurement documents require a procurement instance");
    return buildWorkflowDocument(engine, id, "procurement", {}, false, engine.getOutcome(id) == "APPROVED");
}
Document* Director::constructResultDocument(TividyEngineFacade& engine, const std::string& id,
    const std::map<std::string, std::string>& metadata, bool contract, bool transactional)
{
    return buildWorkflowDocument(engine, id, "result", metadata, contract, transactional);
}
