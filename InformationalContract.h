#ifndef INFORMATIONALCONTRACT_H
#define INFORMATIONALCONTRACT_H

#include "Contract.h"
#include <string>

class InformationalContract : public Contract {
public:
InformationalContract() = default;
void print();
~InformationalContract() = default;
};

#endif 