/**
 * @file DocumentationBuilder.h
 * @brief Builder interface for constructing a document in steps.
 */
#ifndef DOCUMENTATIONBUILDER_H
#define DOCUMENTATIONBUILDER_H

#include "Document.h"
#include <map>

class DocumentFactory;

/**
 * @class DocumentationBuilder
 * @brief Builder interface for constructing a document in steps.
 */
class DocumentationBuilder {
public:
    virtual Document* buildPart() = 0;
    virtual Document* getResult() = 0;
    virtual void setData(const std::map<std::string, std::string>& values)
    {
        (void)values;
    }
    /** Assemble domain content from engine data; caller does not preformat a document. */
    virtual void setWorkflowData(const std::map<std::string, std::string>& values) { setData(values); }
    virtual ~DocumentationBuilder() = default;
protected:
    static std::string universityContent(const std::map<std::string, std::string>& values, bool contract);
    static std::string resultContent(const std::map<std::string, std::string>& values);
};

#endif