#ifndef DOCUMENTATIONBUILDER_H
#define DOCUMENTATIONBUILDER_H

#include "Document.h"

class DocumentFactory;

class DocumentationBuilder {
public:
    virtual Document* buildPart() = 0;
    virtual Document* getResult() = 0;
    virtual ~DocumentationBuilder() = default;
};

#endif