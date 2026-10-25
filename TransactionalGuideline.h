#ifndef TRANSACTIONALGUIDELINE_H
#define TRANSACTIONALGUIDELINE_H

#include "Guideline.h"
#include <string>

class TransactionalGuideline : public Guideline {
public:
TransactionalGuideline() = default;
void print();
~TransactionalGuideline() = default;
};

#endif 