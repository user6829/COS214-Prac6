/**
 * @file InformationalContract.h
 * @brief Informational-industry contract document implementation.
 */
#ifndef INFORMATIONALCONTRACT_H
#define INFORMATIONALCONTRACT_H

#include "Contract.h"
#include <string>

/**
 * @class InformationalContract
 * @brief Informational-industry contract document implementation.
 */
class InformationalContract : public Contract {
public:
    InformationalContract() = default;
    void print() override;
    ~InformationalContract() override = default;
};

#endif