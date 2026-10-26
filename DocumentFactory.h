#ifndef DOCUMENTFACTORY_H
#define DOCUMENTFACTORY_H

#include "Document.h"
#include <string>

class DocumentFactory {
public:
    DocumentFactory() = default;
    virtual Document* createInstance(const std::string& type) = 0;
    virtual ~DocumentFactory() = default;

private:
    std::string type;
};

#endif