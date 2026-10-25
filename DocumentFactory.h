#ifndef DOCUMENTFACTORY_H
#define DOCUMENTFACTORY_H
#include <string>

class DocumentFactory {
public:
DocumentFactory() = default;
virtual Document* createTransactionalInstance() = 0;
Virtual Document* createInformationalInstance() = 0;
virtual ~DocumentFactory() = default;
};

#endif 