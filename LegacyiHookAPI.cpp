#include "LegacyiHookAPI.h"

#include <iostream>

void LegacyiHookAPI::transmitRawData(std::string data)
{
    std::cout<<"[Legacy] Transmitting: "<<std::endl;
    std::cout<<data<<std::endl;
}