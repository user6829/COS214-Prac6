#include "StateObserver.h"

void StateObserver::update(SubjectWorkFlowInstance& s)
{
    std::cout<<"[State]"<<std::endl;
    std::cout<<"The current state of the "<<s.getEventName()<<" event is"<<s.getState()<<"."<<std::endl;
}