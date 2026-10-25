#ifndef DOCUMENT_H
#define DOCUMENT_H
#include <string>

class Document {
public:
Document() = default;
virtual void print() = 0;
virtual ~Document() = default;
};

#endif 