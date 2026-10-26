#ifndef STATEOBSERVER_H
#define STATEOBSERVER_H

#include "EventObserver.h"

    /**
    * @brief Observes the state of an instance
    */

class StateObserver : public EventObserver
{
public:
    void update(SubjectWorkFlowInstance& s);
};
#endif