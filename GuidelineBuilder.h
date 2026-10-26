#ifndef GUIDELINEBUILDER_H
#define GUIDELINEBUILDER_H

#include "DocumentationBuilder.h"

class GuidelineBuilder : public DocumentationBuilder {
public:
    Document* buildPart() override;
    Document* getResult() override;

private:
    Document* result = nullptr;
};

#endif