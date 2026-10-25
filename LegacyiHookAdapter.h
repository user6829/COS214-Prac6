#ifndef LEGACYIHOOKADAPTER_H
#define LEGACYIHOOKADAPTER_H

#include "ExternalSystemConnector.h"
#include "LegacyiHookAPI.h"

template <typename A, typename B>
class LegacyiHookAdapter : public ExternalSystemConnector
{
private:
    LegacyiHookAdapter* legacySystem;
public:
    bool sendPayload(const std::map<A,B>& payload);
    ~LegacyiHookAdapter();
};

#endif  