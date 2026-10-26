#ifndef AUDITLOGGEROBSERVER_H
#define AUDITLOGGEROBSERVER_H

#include "EventObserver.h"

    /**
    * @brief AuditLogger for an instance
    */


class AuditLoggerObserver : public EventObserver
{
public:
    void update(SubjectWorkFlowInstance& s);
};
#endif