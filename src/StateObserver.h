/**
 * @file StateObserver.h
 * @brief Observer that reports workflow-instance state changes.
 */
#ifndef STATEOBSERVER_H
#define STATEOBSERVER_H

#include "EventObserver.h"

    /**
    * @brief Observes the state of an instance
    */

class StateObserver : public EventObserver
{
private:
    bool verbose;

public:
    StateObserver() : verbose(true) {}
    void setVerbose(bool enabled) { verbose = enabled; }
    void update(SubjectWorkFlowInstance& s);
};
#endif