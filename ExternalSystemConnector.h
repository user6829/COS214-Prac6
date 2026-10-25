#ifndef EXTERNALSYSTEMCONNECTOR_H
#define EXTERNALSYSTEMCONNECTOR_H

#include <map>
#include <string>

class ExternalSystemConnector
{
public:
    virtual void sendPayload(const std::map<std::string,std::string>& payload) = 0;
};

#endif  