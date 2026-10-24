#ifndef EVENTOBSERVER_H
#define EVENTOBSERVER_H

#include <string>
#include <iostream>
#include "SubjectWorkFlowInstance.h"

class EventObserver
{
public:
    virtual void update(SubjectWorkFlowInstance& s) = 0;
    virtual ~EventObserver(){};
};

#endif