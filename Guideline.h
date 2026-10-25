#ifndef GUIDELINE_H
#define GUIDELINE_H
#include "Document.h"
#include <string>

class Guideline : public Document {
public:
Guideline() = default;
void print() = 0;
~Guideline() = default;
};

#endif 