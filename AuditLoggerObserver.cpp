#include "AuditLoggerObserver.h"

void AuditLoggerObserver::update(SubjectWorkFlowInstance& s)
{
    std::cout<<"[Audit]"<<std::endl;
    std::cout<<"The event "<<s.getEventName()<<" with the ID of "<<s.getInstanceID()<<"."<<std::endl;
}