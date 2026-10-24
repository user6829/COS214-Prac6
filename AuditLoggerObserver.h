#ifndef AUDITLOGGEROBSERVER_H
#define AUDITLOGGEROBSERVER_H

#include "EventObserver.h"

class AuditLoggerObserver : public EventObserver
{
public:
    void update(SubjectWorkFlowInstance& s);
};
#endif