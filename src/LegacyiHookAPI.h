/**
 * @file LegacyiHookAPI.h
 * @brief Legacy external-system API (Adaptee) with its original payload format.
 */
#ifndef LEGACYIHOOKAPI_H
#define LEGACYIHOOKAPI_H

#include <string>

    /**
    * @brief The legacy system used to transimit data (Adaptee)
    */

class LegacyiHookAPI
{
public:
    /**
    * @brief Show the data in the format of key--value
    * @param data the data that is passed into the function
    */
    void transmitRawData(std::string data);
};

#endif  