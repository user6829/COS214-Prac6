#include "NotificationObserver.h"

void NotificationObserver::update(SubjectWorkFlowInstance& s)
{
    if (!verbose)
        return;
    std::cout<<"[Notification]"<<std::endl;
    std::cout<<"The event "<<s.getEventName()<<" with the ID of "<<s.getInstanceID()<<"."<<std::endl;
}