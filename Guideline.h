/**
 * @file Guideline.h
 * @brief Abstract document category for guideline-like workflow documents.
 */
#ifndef GUIDELINE_H
#define GUIDELINE_H

#include "Document.h"
#include <string>

/**
 * @class Guideline
 * @brief Abstract document category for guideline-like workflow documents.
 */
class Guideline : public Document {
public:
    Guideline() = default;
    ~Guideline() override = default;
};

#endif