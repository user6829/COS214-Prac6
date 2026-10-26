#include "GuidelineBuilder.h"
#include "DocumentFactory.h"
#include "InformationalFactory.h"
#include "TransactionalFactory.h"

Document* GuidelineBuilder::buildPart() {
    DocumentFactory* factory = new InformationalFactory();
    result = factory->createInstance("guideline");
    delete factory;
    return result;
}

Document* GuidelineBuilder::getResult() {
    return result;
}