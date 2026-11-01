/**
 * @file Document.h
 * @brief Abstract interface shared by documents produced for workflow scenarios.
 */
#ifndef DOCUMENT_H
#define DOCUMENT_H
#include <string>
#include <map>

/**
 * @class Document
 * @brief Abstract interface shared by documents produced for workflow scenarios.
 */
class Document {
protected:
std::map<std::string, std::string> data;
public:
Document() = default;
virtual void print() = 0;
virtual std::string render() const
{
    std::map<std::string, std::string>::const_iterator it = data.find("content");
    return it == data.end() ? std::string() : it->second;
}
virtual void setData(const std::map<std::string, std::string>& values)
{
    data = values;
}
virtual ~Document() = default;
};

#endif 