/**
 * @file TransactionalGuideline.h
 * @brief Guideline document for the transactional workflow context.
 */
#ifndef TRANSACTIONALGUIDELINE_H
#define TRANSACTIONALGUIDELINE_H

#include "Guideline.h"
#include <string>

/**
 * @class TransactionalGuideline
 * @brief Guideline document for the transactional workflow context.
 */
class TransactionalGuideline : public Guideline {
public:
    TransactionalGuideline() = default;
    void print() override;
    ~TransactionalGuideline() override = default;
};

#endif