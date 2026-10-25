#ifndef LEGACYIHOOKADAPTER_H
#define LEGACYIHOOKADAPTER_H

#include "ExternalSystemConnector.h"
#include "LegacyiHookAPI.h"

class LegacyiHookAdapter : public ExternalSystemConnector
{
private:
    LegacyiHookAPI* legacySystem;
public:
    LegacyiHookAdapter(LegacyiHookAPI*);
    void sendPayload(const std::map<std::string,std::string>& payload);
    ~LegacyiHookAdapter(){};
};

#endif  