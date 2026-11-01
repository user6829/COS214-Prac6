/**
 * @file NotificationObserver.h
 * @brief Observer that displays user-facing workflow notifications.
 */
#ifndef NOTIFICATIONOBSERVER_H
#define NOTIFICATIONOBSERVER_H

#include "EventObserver.h"
#include <iostream>

    /**
    * @brief Notification system for the instance
    */

class NotificationObserver : public EventObserver
{
private:
    bool verbose;

public:
    NotificationObserver() : verbose(true) {}
    void setVerbose(bool enabled) { verbose = enabled; }
    void update(SubjectWorkFlowInstance& s);
};

#endif