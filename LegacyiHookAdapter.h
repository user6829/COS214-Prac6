#ifndef LEGACYIHOOKADAPTER_H
#define LEGACYIHOOKADAPTER_H

#include "ExternalSystemConnector.h"
#include "LegacyiHookAPI.h"

class LegacyiHookAdapter : public ExternalSystemConnector
{
private:
    LegacyiHookAdapter* legacySystem;
public:
    bool sendPayload(const std::map<std::string, std::string>& payload);
    ~LegacyiHookAdapter();
};

#endif  