#ifndef TRANSACTIONALCONTRACT_H
#define TRANSACTIONALCONTRACT_H

#include "Contract.h"
#include <string>

class TransactionalContract : public Contract {
public:
    TransactionalContract() = default;
    void print() override;
    ~TransactionalContract() override = default;
};

#endif