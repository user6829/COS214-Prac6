#ifndef NOTIFICATIONOBSERVER_H
#define NOTIFICATIONOBSERVER_H

#include "EventObserver.h"

class NotificationObserver : public EventObserver
{
public:
    void onEvent(std::string eventName, std::string instanceID);
};

#endif