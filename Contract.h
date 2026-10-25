#ifndef CONTRACT_H
#define CONTRACT_H

#include "Document.h"
#include <string>

class Contract : public Document {
public:
Contract() = default;
void print() = 0;
 ~Contract() = default;
};

#endif 