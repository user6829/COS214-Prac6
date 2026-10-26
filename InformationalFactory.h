#ifndef INFORMATIONALFACTORY_H
#define INFORMATIONALFACTORY_H

#include "DocumentFactory.h"
#include <string>

class InformationalFactory : public DocumentFactory {
public:
    InformationalFactory() = default;
    Document* createInstance(const std::string& type) override;
    ~InformationalFactory() override = default;
};

#endif