#ifndef EXTERNALSYSTEMCONNECTOR_H
#define EXTERNALSYSTEMCONNECTOR_H

#include <map>
#include <string>

    /**
    * @brief The target interface to be used to transmit the data
    */


class ExternalSystemConnector
{
public:
    virtual void sendPayload(const std::map<std::string,std::string>& payload) = 0;
};

#endif  