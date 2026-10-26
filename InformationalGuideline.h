#ifndef INFORMATIONALGUIDELINE_H
#define INFORMATIONALGUIDELINE_H

#include "Guideline.h"
#include <string>

class InformationalGuideline : public Guideline {
public:
    InformationalGuideline() = default;
    void print() override;
    ~InformationalGuideline() override = default;
};

#endif