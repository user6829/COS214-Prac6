/**
 * @file Director.h
 * @brief Coordinates a builder to construct a document or workflow definition.
 */
#ifndef DIRECTOR_H
#define DIRECTOR_H

class Document;
class DocumentationBuilder;
class WorkflowDefinition;
class WorkflowBuilder;
class TividyEngineFacade;
#include <map>
#include <string>

/**
 * @class Director
 * @brief Coordinates a builder to construct a document or workflow definition.
 */
class Director {
private:
    DocumentationBuilder* docBuilder;
    WorkflowBuilder* wfBuilder;

public:
    Director() : docBuilder(nullptr), wfBuilder(nullptr) {}
    virtual ~Director() {}
    // Overloaded constructors for different builders
    explicit Director(DocumentationBuilder* b) : docBuilder(b), wfBuilder(nullptr) {}
    explicit Director(WorkflowBuilder* b) : docBuilder(nullptr), wfBuilder(b) {}
    
    Document* constructDocument();
    virtual WorkflowDefinition* constructWorkflow();
    /** Engine-gated document creation; returned products are owned by the caller. */
    static Document* constructUniversityDocument(TividyEngineFacade& engine, const std::string& instanceId, bool contract);
    static Document* constructProcurementDocument(TividyEngineFacade& engine, const std::string& instanceId);
    static Document* constructResultDocument(TividyEngineFacade& engine, const std::string& instanceId,
        const std::map<std::string, std::string>& metadata, bool contract, bool transactional);
};

#endif