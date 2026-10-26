#ifndef TRANSACTIONALFACTORY_H
#define TRANSACTIONALFACTORY_H

#include "DocumentFactory.h"
#include <string>

class TransactionalFactory : public DocumentFactory {
public:
    TransactionalFactory() = default;
    Document* createInstance(const std::string& type) override;
    ~TransactionalFactory() override = default;
    std::string getType();
};

#endif