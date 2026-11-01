/**
 * @file ExternalSystemConnector.h
 * @brief Target interface used to send workflow payloads to an external system.
 */
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
    virtual ~ExternalSystemConnector() = default;
    virtual void sendPayload(const std::map<std::string,std::string>& payload) = 0;
};

#endif  