#include "LegacyiHookAdapter.h"
#include <stdexcept>

LegacyiHookAdapter::LegacyiHookAdapter(LegacyiHookAPI* l){
    this->legacySystem = l;
    if (this->legacySystem == nullptr)
        throw std::invalid_argument("legacySystem must not be null");
}

void LegacyiHookAdapter::sendPayload(const std::map<std::string, std::string> &payload)
{
    std::string s;
    for (const auto& data : payload)
    {
        if(!s.empty()){
            s += ";\n";
        }
        s += data.first + "--" + data.second;
    }
    legacySystem->transmitRawData(s);
}