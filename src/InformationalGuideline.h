/**
 * @file InformationalGuideline.h
 * @brief Guideline document for the informational workflow context.
 */
#ifndef INFORMATIONALGUIDELINE_H
#define INFORMATIONALGUIDELINE_H

#include "Guideline.h"
#include <string>

/**
 * @class InformationalGuideline
 * @brief Guideline document for the informational workflow context.
 */
class InformationalGuideline : public Guideline {
public:
    InformationalGuideline() = default;
    void print() override;
    ~InformationalGuideline() override = default;
};

#endif