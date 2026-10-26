#ifndef CONTRACTBUILDER_H
#define CONTRACTBUILDER_H

#include "DocumentationBuilder.h"

class ContractBuilder : public DocumentationBuilder {
public:
    Document* buildPart() override;
    Document* getResult() override;

private:
    Document* result = nullptr;
};

#endif