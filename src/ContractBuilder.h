/**
 * @file ContractBuilder.h
 * @brief Builder that creates a contract document.
 */
#ifndef CONTRACTBUILDER_H
#define CONTRACTBUILDER_H

#include "DocumentationBuilder.h"
#include <map>

/**
 * @class ContractBuilder
 * @brief Builder that creates a contract document.
 */
class ContractBuilder : public DocumentationBuilder {
public:
    ContractBuilder(const ContractBuilder&) = delete;
    ContractBuilder& operator=(const ContractBuilder&) = delete;
    explicit ContractBuilder(bool transactional = false)
        : transactional(transactional) {}
    ~ContractBuilder() override;
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