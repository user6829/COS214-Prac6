#ifndef NOTIFICATIONOBSERVER_H
#define NOTIFICATIONOBSERVER_H

#include "EventObserver.h"
#include <iostream>

    /**
    * @brief Notification system for the instance
    */

class NotificationObserver : public EventObserver
{
public:
    void update(SubjectWorkFlowInstance& s);
};

#endif