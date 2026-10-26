#ifndef DIRECTOR_H
#define DIRECTOR_H

class Document;
class DocumentationBuilder;

class Director {
public:
    explicit Director(DocumentationBuilder* b) : builder(b) {}
    Document* construct();

private:
    DocumentationBuilder* builder;
};

#endif