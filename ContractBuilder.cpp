#include "ContractBuilder.h"
#include "DocumentFactory.h"
#include "InformationalFactory.h"
#include "TransactionalFactory.h"

Document* ContractBuilder::buildPart() {
    // Choose a factory strategy; here we default to informational.
    // In a real system this would be injected/configured.
    DocumentFactory* factory = new InformationalFactory();
    result = factory->createInstance("contract");
    delete factory;
    return result;
}

Document* ContractBuilder::getResult() {
    return result;
}