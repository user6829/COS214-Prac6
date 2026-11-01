/**
 * @file TransactionalFactory.h
 * @brief Factory producing documents for the transactional workflow context.
 */
#ifndef TRANSACTIONALFACTORY_H
#define TRANSACTIONALFACTORY_H

#include "DocumentFactory.h"
#include <string>

/**
 * @class TransactionalFactory
 * @brief Factory producing documents for the transactional workflow context.
 */
class TransactionalFactory : public DocumentFactory {
public:
    TransactionalFactory() = default;
    Document* createInstance(const std::string& type) override;
    ~TransactionalFactory() override = default;
    std::string getType();
};

#endif