#ifndef INFORMATIONALFACTORY_H
#define INFORMATIONALFACTORY_H

#include "DocumentFactory"
#include <string>

class InformationalFactory : Public DocumentFactory {
public:
InformationalFactory() = default;
Virtual Document* createInformationalInstance();
virtual ~InformationalFactory() = default;
};

#endif 