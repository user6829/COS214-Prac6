#ifndef NOTIFICATIONOBSERVER_H
#define NOTIFICATIONOBSERVER_H

#include "EventObserver.h"
#include <iostream>

class NotificationObserver : public EventObserver
{
public:
    void update(SubjectWorkFlowInstance& s);
};

#endif