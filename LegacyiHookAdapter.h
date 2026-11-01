/**
 * @file LegacyiHookAdapter.h
 * @brief Adapter translating the workflow payload into the legacy API format.
 */
#ifndef LEGACYIHOOKADAPTER_H
#define LEGACYIHOOKADAPTER_H

#include "ExternalSystemConnector.h"
#include "LegacyiHookAPI.h"

    /**
    * @brief New system used to represent the data(Adapter)
    */

class LegacyiHookAdapter : public ExternalSystemConnector
{
private:

    /**
    * @brief Object to use the legacy system to represent the data
    */
    LegacyiHookAPI* legacySystem;
public:
    /**
    * @brief Initializes the legacySystem variable
    * @param l The variable that will be used to set the legacySystem
    */
    LegacyiHookAdapter(LegacyiHookAPI* l);

    /**
    * @brief The new method to represent the data
    * @param payload The data that is passed in as a map that is to be converted to use the old system 
    */
    void sendPayload(const std::map<std::string,std::string>& payload);


    ~LegacyiHookAdapter(){};
};

#endif  