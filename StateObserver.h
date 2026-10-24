#ifndef STATEOBSERVER_H
#define STATEOBSERVER_H

#include "EventObserver.h"

class StateObserver : public EventObserver
{
public:
    void update(SubjectWorkFlowInstance& s);
};
#endif