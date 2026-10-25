#ifndef TRANSACTIONALFACTORY_H
#define TRANSACTIONALFACTORY_H

#include "DocumentFactory"
#include <string>

class TransactionalFactory : Public DocumentFactory {
public:
TransactionalFactory() = default;
virtual Document* createTransactionalInstance();
virtual ~TransactionalFactory() = default;
};

#endif 