#ifndef EXTERNALSYSTEMCONNECTOR_H
#define EXTERNALSYSTEMCONNECTOR_H

#include <map>
#include <string>

template <typename A, typename B>
class ExternalSystemConnector
{
public:
    virtual bool sendPayload(const std::map<A,B>& payload) = 0;
};

#endif  