#ifndef DIRECTOR_H
#define DIRECTOR_H

class Document;
class DocumentationBuilder;
class WorkflowDefinition;
class WorkflowBuilder;

class Director {
private:
    DocumentationBuilder* docBuilder;
    WorkflowBuilder* wfBuilder;

public:
    // Overloaded constructors for different builders
    explicit Director(DocumentationBuilder* b) : docBuilder(b), wfBuilder(nullptr) {}
    explicit Director(WorkflowBuilder* b) : docBuilder(nullptr), wfBuilder(b) {}
    
    Document* constructDocument();
    WorkflowDefinition* constructWorkflow();
};

#endif