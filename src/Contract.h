/**
 * @file Contract.h
 * @brief Abstract document category for contract-like workflow documents.
 */
#ifndef CONTRACT_H
#define CONTRACT_H

#include "Document.h"
#include <string>

/**
 * @class Contract
 * @brief Abstract document category for contract-like workflow documents.
 */
class Contract : public Document {
public:
Contract() = default;
 ~Contract() = default;
};

#endif 