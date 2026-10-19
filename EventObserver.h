#ifndef EVENTOBSERVER_H
#define EVENTOBSERVER_H

#include <string>

class EventObserver
{
public:
    virtual void onEvent(std::string eventName, std::string instaceID) = 0;
};

#endif