/**
 * @file GuidelineBuilder.h
 * @brief Builder that creates a guideline document.
 */
#ifndef GUIDELINEBUILDER_H
#define GUIDELINEBUILDER_H

#include "DocumentationBuilder.h"
#include <map>

/**
 * @class GuidelineBuilder
 * @brief Builder that creates a guideline document.
 */
class GuidelineBuilder : public DocumentationBuilder {
public:
    GuidelineBuilder(const GuidelineBuilder&) = delete;
    GuidelineBuilder& operator=(const GuidelineBuilder&) = delete;
    explicit GuidelineBuilder(bool transactional = false)
        : transactional(transactional) {}
    ~GuidelineBuilder() override;
    Document* buildPart() override;
    Document* getResult() override;
    void setData(const std::map<std::string, std::string>& values) override;
    void setWorkflowData(const std::map<std::string, std::string>& values) override;

private:
    Document* result = nullptr;
    bool transactional;
    std::map<std::string, std::string> values;
};

#endif